#include "circe_worker.h"

#include <stdlib.h>
#include <string.h>

#include "circe_index.h"
#include "circe_daily.h"
#include "circe_patterns.h"
#include "circe_photo.h"
#include "circe_reflection.h"
#include "circe_save.h"
#include "circe_storage.h"
#include "circe_timeline.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "circe_worker";

#define CIRCE_WORKER_STACK 16384
#define CIRCE_WORKER_PRIO  4
#define CIRCE_WORKER_QUEUE 6

typedef struct {
    circe_worker_cmd_type_t type;
    circe_entry_t entry;
    bool editing_existing;
    char delete_id[CIRCE_MAX_ID];
    char load_entry_id[CIRCE_MAX_ID];
    circe_timeline_category_t timeline_category;
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
    s_busy = false;
    if (s_done_fn && result) {
        s_done_fn(result, s_done_ctx);
    }
    free(result);
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
        circe_reflection_clear_recent_context();
        snprintf(out->summary, sizeof(out->summary), "Save failed: %s", circe_save_result_name(out->save_result));
    }
}

static void load_reflection_context_after_save(const circe_worker_cmd_t *cmd, const circe_worker_completion_t *out)
{
    if (out->success && cmd->success_step == CIRCE_FLOW_REFLECTION && !cmd->editing_existing) {
        if (!circe_reflection_load_recent_context()) {
            circe_reflection_clear_recent_context();
        }
    } else {
        circe_reflection_clear_recent_context();
    }
}

static void run_delete_entry(const circe_worker_cmd_t *cmd, circe_worker_completion_t *out)
{
    circe_entry_t entry;
    if (circe_entry_load(cmd->delete_id, &entry)) {
        circe_photo_delete_file_for_entry(&entry);
    }
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

static void run_load_timeline(const circe_worker_cmd_t *cmd, circe_worker_completion_t *out)
{
    circe_timeline_cache_t cache = {0};
    out->timeline_category = cmd->timeline_category;
    out->timeline_ok = circe_timeline_load_category(cmd->timeline_category, &cache);
    out->timeline_index_error = cache.index_error;
    out->timeline_count = cache.count;
    out->timeline_empty = cache.count == 0;
    out->timeline_truncated = cache.truncated;
    out->success = out->timeline_ok && !cache.index_error;
    snprintf(out->summary, sizeof(out->summary), "Timeline %s: %d",
             circe_timeline_category_title(cmd->timeline_category), cache.count);
}

static void run_load_entry(const circe_worker_cmd_t *cmd, circe_worker_completion_t *out)
{
    out->success = cmd->load_entry_id[0] && circe_entry_load(cmd->load_entry_id, &out->entry);
    out->review_found = out->success;
    strncpy(out->entry_id, cmd->load_entry_id, sizeof(out->entry_id) - 1);
    out->entry_id[sizeof(out->entry_id) - 1] = '\0';
    snprintf(out->summary, sizeof(out->summary), out->success ? "Loaded %s" : "Load failed",
             cmd->load_entry_id);
}

static void run_load_patterns(circe_worker_completion_t *out)
{
    out->success = circe_patterns_scan(&out->patterns);
    if (out->patterns.storage_unavailable) {
        out->success = false;
        snprintf(out->summary, sizeof(out->summary), "Patterns: storage unavailable");
    } else if (out->patterns.index_error) {
        out->success = false;
        snprintf(out->summary, sizeof(out->summary), "Patterns: load failed");
    } else if (out->patterns.not_enough_entries) {
        snprintf(out->summary, sizeof(out->summary), "Patterns: %d entries", out->patterns.total_entries);
    } else if (out->patterns.no_patterns) {
        snprintf(out->summary, sizeof(out->summary), "Patterns: none");
    } else {
        snprintf(out->summary, sizeof(out->summary), "Patterns: %d found", out->patterns.count);
    }
}

static void run_photo_capture(const circe_worker_cmd_t *cmd, circe_worker_completion_t *out)
{
    circe_entry_t entry = cmd->entry;
    out->photo_result = circe_photo_capture_and_attach(&entry);
    out->entry = entry;
    out->success = out->photo_result == CIRCE_PHOTO_RESULT_SAVED;
    strncpy(out->entry_id, entry.id, sizeof(out->entry_id) - 1);
    out->entry_id[sizeof(out->entry_id) - 1] = '\0';
    switch (out->photo_result) {
    case CIRCE_PHOTO_RESULT_SAVED:
        snprintf(out->summary, sizeof(out->summary), "Photo saved for %s", out->entry_id);
        break;
    case CIRCE_PHOTO_RESULT_SAVE_FAILED:
        snprintf(out->summary, sizeof(out->summary), "Photo save failed");
        break;
    case CIRCE_PHOTO_RESULT_UNAVAILABLE:
    default:
        snprintf(out->summary, sizeof(out->summary), "Camera unavailable");
        break;
    }
}

static void run_load_daily_companion(circe_worker_completion_t *out)
{
    out->success = circe_daily_load(&out->daily);
    snprintf(out->summary, sizeof(out->summary), "Daily entries=%d reg=%d", out->daily.entries_today,
             out->daily.regulation_today);
}

static void run_health_check(circe_worker_completion_t *out)
{
    circe_storage_health_check(&out->health);
    out->storage_ready = out->health.storage_ready;
    out->success = true;
    snprintf(out->summary, sizeof(out->summary), "ready:%s entries:%d probe:%s",
             out->health.storage_ready ? "yes" : "no", out->health.entry_count,
             out->health.probe_passed ? "PASS" : "FAIL");
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
            load_reflection_context_after_save(&cmd, &result);
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
        case CIRCE_WORKER_LOAD_TIMELINE:
            run_load_timeline(&cmd, &result);
            break;
        case CIRCE_WORKER_LOAD_ENTRY:
            run_load_entry(&cmd, &result);
            break;
        case CIRCE_WORKER_LOAD_PATTERNS:
            run_load_patterns(&result);
            break;
        case CIRCE_WORKER_PHOTO_CAPTURE:
            run_photo_capture(&cmd, &result);
            break;
        case CIRCE_WORKER_HEALTH_CHECK:
        case CIRCE_WORKER_STORAGE_STATUS:
        case CIRCE_WORKER_DIAGNOSTICS_REFRESH:
            run_health_check(&result);
            break;
        case CIRCE_WORKER_LOAD_DAILY_COMPANION:
            run_load_daily_companion(&result);
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

bool circe_worker_post_load_timeline(circe_timeline_category_t category)
{
    circe_worker_cmd_t cmd = {0};
    cmd.type = CIRCE_WORKER_LOAD_TIMELINE;
    cmd.timeline_category = category;
    return post_cmd(&cmd);
}

bool circe_worker_post_load_entry(const char *id)
{
    if (!id || !id[0]) {
        return false;
    }
    circe_worker_cmd_t cmd = {0};
    cmd.type = CIRCE_WORKER_LOAD_ENTRY;
    strncpy(cmd.load_entry_id, id, sizeof(cmd.load_entry_id) - 1);
    cmd.load_entry_id[sizeof(cmd.load_entry_id) - 1] = '\0';
    return post_cmd(&cmd);
}

bool circe_worker_post_load_patterns(void)
{
    return post_simple(CIRCE_WORKER_LOAD_PATTERNS);
}

bool circe_worker_post_photo_capture(const circe_entry_t *entry)
{
    if (!entry) {
        return false;
    }
    circe_worker_cmd_t cmd = {0};
    cmd.type = CIRCE_WORKER_PHOTO_CAPTURE;
    cmd.entry = *entry;
    return post_cmd(&cmd);
}

bool circe_worker_post_health_check(void)
{
    return post_simple(CIRCE_WORKER_HEALTH_CHECK);
}

bool circe_worker_post_storage_status(void)
{
    return post_simple(CIRCE_WORKER_STORAGE_STATUS);
}

bool circe_worker_post_diagnostics_refresh(void)
{
    return post_simple(CIRCE_WORKER_DIAGNOSTICS_REFRESH);
}

bool circe_worker_post_load_daily_companion(void)
{
    return post_simple(CIRCE_WORKER_LOAD_DAILY_COMPANION);
}
