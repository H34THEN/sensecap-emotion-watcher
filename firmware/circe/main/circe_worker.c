#include "circe_worker.h"

#include <stdlib.h>
#include <string.h>

#include "circe_index.h"
#include "circe_save.h"
#include "circe_storage.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "circe_worker";

#define CIRCE_WORKER_STACK 12288
#define CIRCE_WORKER_PRIO  4
#define CIRCE_WORKER_QUEUE 6

typedef struct {
    circe_worker_cmd_type_t type;
    circe_entry_t entry;
    bool editing_existing;
    char delete_id[CIRCE_MAX_ID];
    circe_flow_step_t success_step;
    int success_message;
    bool show_quick_subline;
} circe_worker_cmd_t;

static QueueHandle_t s_queue;
static TaskHandle_t s_task;
static circe_worker_done_fn s_done_fn;
static void *s_done_ctx;
static volatile bool s_busy;

static void log_worker_resources(const char *label)
{
    UBaseType_t hw = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "worker stack %s: %u words free (~%u bytes)", label, (unsigned)hw,
             (unsigned)(hw * sizeof(StackType_t)));
    ESP_LOGI(TAG, "heap free %s: %u", label, (unsigned)esp_get_free_heap_size());
}

static void async_deliver(void *user_data)
{
    circe_worker_completion_t *result = user_data;
    if (s_done_fn && result) {
        s_done_fn(result, s_done_ctx);
    }
    free(result);
    s_busy = false;
}

static void dispatch_completion_safe(circe_worker_completion_t *result)
{
    circe_worker_completion_t *copy = malloc(sizeof(*copy));
    if (!copy) {
        ESP_LOGE(TAG, "completion alloc failed");
        s_busy = false;
        return;
    }
    *copy = *result;
    if (lv_async_call(async_deliver, copy) != LV_RES_OK) {
        ESP_LOGE(TAG, "lv_async_call failed");
        free(copy);
        s_busy = false;
    }
}

static bool post_cmd(const circe_worker_cmd_t *cmd)
{
    if (!s_queue || !cmd) {
        return false;
    }
    if (s_busy) {
        ESP_LOGW(TAG, "worker busy, reject type=%d", (int)cmd->type);
        return false;
    }
    s_busy = true;
    if (xQueueSend(s_queue, cmd, 0) != pdTRUE) {
        s_busy = false;
        return false;
    }
    return true;
}

static void run_test_save(circe_worker_completion_t *out)
{
    out->success = circe_storage_run_save_self_test(&out->test_save);
    snprintf(out->summary, sizeof(out->summary), "%s", out->test_save.summary);
}

static void run_save_entry(const circe_worker_cmd_t *cmd, circe_worker_completion_t *out)
{
    circe_entry_t entry = cmd->entry;
    out->save_result = circe_save_entry_report(&entry, cmd->editing_existing, &out->save_report);
    out->success = circe_save_result_is_success(out->save_result);
    out->success_step = cmd->success_step;
    out->success_message = cmd->success_message;
    out->show_quick_subline = cmd->show_quick_subline;
    strncpy(out->entry_id, entry.id, sizeof(out->entry_id) - 1);
    out->entry_id[sizeof(out->entry_id) - 1] = '\0';
    strncpy(out->saved_color, entry.color_hex, sizeof(out->saved_color) - 1);
    out->saved_color[sizeof(out->saved_color) - 1] = '\0';
    out->entry = entry;
    if (out->success) {
        snprintf(out->summary, sizeof(out->summary), "Saved %s", out->entry_id);
    } else {
        snprintf(out->summary, sizeof(out->summary), "Save failed: %s", circe_save_result_name(out->save_result));
    }
}

static void run_delete_entry(const circe_worker_cmd_t *cmd, circe_worker_completion_t *out)
{
    out->success = cmd->delete_id[0] && circe_entry_delete_hard(cmd->delete_id);
    strncpy(out->entry_id, cmd->delete_id, sizeof(out->entry_id) - 1);
    out->entry_id[sizeof(out->entry_id) - 1] = '\0';
    snprintf(out->summary, sizeof(out->summary), out->success ? "Deleted" : "Delete failed");
}

static void run_rebuild_index(circe_worker_completion_t *out)
{
    out->success = circe_rebuild_index_from_json(&out->rebuild_count);
    snprintf(out->summary, sizeof(out->summary), "Rebuild: %d entries", out->rebuild_count);
}

static void run_storage_probe(circe_worker_completion_t *out)
{
    circe_storage_probe_result_t probe = {0};
    out->success = circe_storage_run_probe(&probe);
    snprintf(out->summary, sizeof(out->summary), out->success ? "Probe PASS" : "Probe FAIL");
}

static void run_reinit_storage(circe_worker_completion_t *out)
{
    out->storage_ready = circe_storage_reinit();
    out->success = out->storage_ready;
    snprintf(out->summary, sizeof(out->summary), out->success ? "Storage reinitialized" : "Reinit failed");
}

static void run_load_review(circe_worker_completion_t *out)
{
    circe_storage_rebuild_index_if_dirty(NULL);
    char id[CIRCE_MAX_ID] = {0};
    out->review_found = circe_storage_get_latest_entry_id(id, sizeof(id));
    if (out->review_found) {
        out->success = circe_entry_load(id, &out->entry);
        out->review_found = out->success;
        strncpy(out->entry_id, id, sizeof(out->entry_id) - 1);
        out->entry_id[sizeof(out->entry_id) - 1] = '\0';
        snprintf(out->summary, sizeof(out->summary), "Loaded %s", id);
    } else {
        out->success = false;
        snprintf(out->summary, sizeof(out->summary), "No entries yet.");
    }
}

static void worker_task(void *arg)
{
    (void)arg;
    circe_worker_cmd_t cmd;
    while (1) {
        if (xQueueReceive(s_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        ESP_LOGI(TAG, "run type=%d", (int)cmd.type);
        log_worker_resources("before");
        circe_worker_completion_t result = {0};
        result.type = cmd.type;

        switch (cmd.type) {
        case CIRCE_WORKER_TEST_SAVE:
            run_test_save(&result);
            break;
        case CIRCE_WORKER_SAVE_ENTRY:
            run_save_entry(&cmd, &result);
            break;
        case CIRCE_WORKER_DELETE_ENTRY:
            run_delete_entry(&cmd, &result);
            break;
        case CIRCE_WORKER_REBUILD_INDEX:
            run_rebuild_index(&result);
            break;
        case CIRCE_WORKER_STORAGE_PROBE:
            run_storage_probe(&result);
            break;
        case CIRCE_WORKER_REINIT_STORAGE:
            run_reinit_storage(&result);
            break;
        case CIRCE_WORKER_LOAD_REVIEW:
            run_load_review(&result);
            break;
        default:
            result.success = false;
            snprintf(result.summary, sizeof(result.summary), "Unknown worker cmd");
            break;
        }

        log_worker_resources("after");
        dispatch_completion_safe(&result);
    }
}

void circe_worker_init(circe_worker_done_fn on_done, void *user_data)
{
    s_done_fn = on_done;
    s_done_ctx = user_data;
    if (s_queue) {
        return;
    }
    s_queue = xQueueCreate(CIRCE_WORKER_QUEUE, sizeof(circe_worker_cmd_t));
    if (!s_queue) {
        ESP_LOGE(TAG, "queue create failed");
        return;
    }
    BaseType_t ok = xTaskCreate(worker_task, "circe_worker", CIRCE_WORKER_STACK, NULL, CIRCE_WORKER_PRIO, &s_task);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "task create failed");
    } else {
        ESP_LOGI(TAG, "worker task started stack=%d", CIRCE_WORKER_STACK);
    }
}

bool circe_worker_is_busy(void)
{
    return s_busy;
}

static bool post_simple(circe_worker_cmd_type_t type)
{
    circe_worker_cmd_t cmd = {0};
    cmd.type = type;
    return post_cmd(&cmd);
}

bool circe_worker_post_test_save(void)
{
    return post_simple(CIRCE_WORKER_TEST_SAVE);
}

bool circe_worker_post_save_entry(const circe_entry_t *entry, bool editing_existing, circe_flow_step_t success_step,
                                  int success_message, bool show_quick_subline)
{
    if (!entry) {
        return false;
    }
    circe_worker_cmd_t cmd = {0};
    cmd.type = CIRCE_WORKER_SAVE_ENTRY;
    cmd.entry = *entry;
    cmd.editing_existing = editing_existing;
    cmd.success_step = success_step;
    cmd.success_message = success_message;
    cmd.show_quick_subline = show_quick_subline;
    return post_cmd(&cmd);
}

bool circe_worker_post_delete_entry(const char *id)
{
    if (!id || !id[0]) {
        return false;
    }
    circe_worker_cmd_t cmd = {0};
    cmd.type = CIRCE_WORKER_DELETE_ENTRY;
    strncpy(cmd.delete_id, id, sizeof(cmd.delete_id) - 1);
    cmd.delete_id[sizeof(cmd.delete_id) - 1] = '\0';
    return post_cmd(&cmd);
}

bool circe_worker_post_rebuild_index(void)
{
    return post_simple(CIRCE_WORKER_REBUILD_INDEX);
}

bool circe_worker_post_storage_probe(void)
{
    return post_simple(CIRCE_WORKER_STORAGE_PROBE);
}

bool circe_worker_post_reinit_storage(void)
{
    return post_simple(CIRCE_WORKER_REINIT_STORAGE);
}

bool circe_worker_post_load_review(void)
{
    return post_simple(CIRCE_WORKER_LOAD_REVIEW);
}
