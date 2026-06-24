#include "circe_save.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "circe_storage.h"
#include "esp_log.h"

static const char *TAG = "circe_save";

const char *circe_save_result_name(circe_save_result_t result)
{
    switch (result) {
    case CIRCE_SAVE_OK:
        return "OK";
    case CIRCE_SAVE_OK_INDEX_WARN:
        return "INDEX_APPEND_FAILED";
    case CIRCE_SAVE_STORAGE_NOT_READY:
        return "STORAGE_NOT_READY";
    case CIRCE_SAVE_JSON_WRITE_FAILED:
        return "JSON_WRITE_FAILED";
    case CIRCE_SAVE_INVALID_COLOR:
        return "INVALID_COLOR";
    case CIRCE_SAVE_INVALID_ENTRY_STATE:
        return "INVALID_ENTRY_STATE";
    case CIRCE_SAVE_INDEX_APPEND_FAILED:
        return "INDEX_APPEND_FAILED";
    case CIRCE_SAVE_MEMORY_ALLOCATION_FAILED:
        return "MEMORY_ALLOCATION_FAILED";
    case CIRCE_SAVE_OK_NON_ATOMIC:
        return "JSON_OK_NON_ATOMIC";
    case CIRCE_SAVE_UNKNOWN_FAILURE:
    default:
        return "UNKNOWN_SAVE_FAILURE";
    }
}

bool circe_save_result_is_success(circe_save_result_t result)
{
    return result == CIRCE_SAVE_OK || result == CIRCE_SAVE_OK_INDEX_WARN || result == CIRCE_SAVE_OK_NON_ATOMIC;
}

bool circe_entry_validate_color_hex(const char *hex)
{
    if (!hex || hex[0] != '#' || strlen(hex) != 7) {
        return false;
    }
    for (int i = 1; i < 7; i++) {
        if (!isxdigit((unsigned char)hex[i])) {
            return false;
        }
    }
    return true;
}

bool circe_entry_normalize_color_hex(char *hex, size_t hex_len)
{
    if (!hex || hex_len < 8) {
        return false;
    }
    if (circe_entry_validate_color_hex(hex)) {
        return true;
    }
    ESP_LOGW(TAG, "invalid color '%s' — fallback to #808080", hex ? hex : "(null)");
    snprintf(hex, hex_len, "#808080");
    return false;
}

bool circe_entry_prepare_for_save(circe_entry_t *entry)
{
    if (!entry) {
        return false;
    }

    if (entry->id[0] == '\0') {
        circe_entry_generate_id(entry);
    }
    if (entry->created_at[0] == '\0' || entry->local_date[0] == '\0') {
        circe_entry_set_timestamp_now(entry);
    } else {
        circe_entry_touch_updated(entry);
    }

    entry->training_ok = false;
    entry->private_locked = true;
    strncpy(entry->emotion, CIRCE_EMOTION_UNKNOWN, sizeof(entry->emotion) - 1);
    entry->emotion[sizeof(entry->emotion) - 1] = '\0';

    circe_entry_normalize_color_hex(entry->color_hex, sizeof(entry->color_hex));

    if (entry->entry_mode == CIRCE_ENTRY_MODE_BODY_ONLY) {
        if (entry->body_area_count <= 0 || entry->body_sensation_count <= 0) {
            ESP_LOGE(TAG, "invalid body entry: areas=%d sensations=%d", entry->body_area_count,
                     entry->body_sensation_count);
            return false;
        }
    }

    if (entry->intensity < 1) {
        entry->intensity = 1;
    }
    if (entry->intensity > 10) {
        entry->intensity = 10;
    }

    return true;
}

static circe_save_result_t finish_save(circe_entry_t *entry, bool json_ok, bool index_ok, circe_save_report_t *report)
{
    if (report) {
        report->json_ok = json_ok;
        report->index_ok = index_ok;
    }

    if (!json_ok) {
        if (report) {
            report->result = CIRCE_SAVE_JSON_WRITE_FAILED;
        }
        return CIRCE_SAVE_JSON_WRITE_FAILED;
    }

    if (!index_ok) {
        ESP_LOGW(TAG, "save json ok, index warn id=%s color=%s", entry->id, entry->color_hex);
        if (report) {
            report->result = CIRCE_SAVE_OK_INDEX_WARN;
        }
        return CIRCE_SAVE_OK_INDEX_WARN;
    }

    if (circe_storage_last_save_non_atomic()) {
        ESP_LOGW(TAG, "save json ok non-atomic id=%s color=%s", entry->id, entry->color_hex);
        if (report) {
            report->result = CIRCE_SAVE_OK_NON_ATOMIC;
        }
        return CIRCE_SAVE_OK_NON_ATOMIC;
    }

    ESP_LOGI(TAG, "save ok id=%s color=%s", entry->id, entry->color_hex);
    if (report) {
        report->result = CIRCE_SAVE_OK;
    }
    return CIRCE_SAVE_OK;
}

static circe_save_result_t map_json_write_failure(void)
{
    const char *err = circe_storage_get_last_error();
    if (err && strstr(err, "MEMORY_ALLOCATION_FAILED")) {
        return CIRCE_SAVE_MEMORY_ALLOCATION_FAILED;
    }
    return CIRCE_SAVE_JSON_WRITE_FAILED;
}

circe_save_result_t circe_save_entry_report(circe_entry_t *entry, bool editing_existing, circe_save_report_t *report)
{
    if (report) {
        report->result = CIRCE_SAVE_UNKNOWN_FAILURE;
        report->json_ok = false;
        report->index_ok = false;
    }

    if (!entry) {
        ESP_LOGE(TAG, "save failed: null entry");
        return CIRCE_SAVE_UNKNOWN_FAILURE;
    }

    bool ready = circe_storage_ensure_ready();
    char json_path[128] = {0};
    circe_storage_entry_json_path(entry, json_path, sizeof(json_path));
    circe_storage_health_t health;
    circe_storage_health_check(&health);

    ESP_LOGI(TAG,
             "save attempt storage_ready=%s probe=%s base=%s json_path=%s color=%s sensations=%d areas=%d editing=%d",
             ready ? "yes" : "no", health.probe_passed ? "yes" : "no", health.base_path, json_path, entry->color_hex,
             entry->body_sensation_count, entry->body_area_count, editing_existing ? 1 : 0);

    if (!ready) {
        circe_storage_set_last_error(health.last_error[0] ? health.last_error : "Storage not ready");
        ESP_LOGE(TAG, "save failed: STORAGE_NOT_READY mount=%d probe=%d s_ready=%d path=%s", health.sd_mounted,
                 health.probe_passed, health.storage_ready, json_path);
        if (report) {
            report->result = CIRCE_SAVE_STORAGE_NOT_READY;
        }
        return CIRCE_SAVE_STORAGE_NOT_READY;
    }

    if (!circe_entry_prepare_for_save(entry)) {
        circe_storage_set_last_error("Invalid entry state");
        ESP_LOGE(TAG, "save failed: INVALID_ENTRY_STATE id=%s areas=%d sensations=%d", entry->id,
                 entry->body_area_count, entry->body_sensation_count);
        if (report) {
            report->result = CIRCE_SAVE_INVALID_ENTRY_STATE;
        }
        return CIRCE_SAVE_INVALID_ENTRY_STATE;
    }

    circe_storage_entry_json_path(entry, json_path, sizeof(json_path));
    ESP_LOGI(TAG, "save start id=%s mode=%s color=%s areas=%d sensations=%d intensity=%d path=%s", entry->id,
             circe_entry_mode_str(entry->entry_mode), entry->color_hex, entry->body_area_count,
             entry->body_sensation_count, entry->intensity, json_path);

    if (editing_existing) {
        if (!circe_entry_save_json_atomic(entry)) {
            circe_save_result_t fail = map_json_write_failure();
            const char *err = circe_storage_get_last_error();
            ESP_LOGE(TAG, "save failed: %s (update) err=%s path=%s", circe_save_result_name(fail),
                     err && err[0] ? err : "none", json_path);
            if (report) {
                report->result = fail;
            }
            return fail;
        }
        bool index_ok = circe_entry_index_insert_best_effort(entry);
        ESP_LOGI(TAG, "save update json=OK index=%s final=%s", index_ok ? "OK" : "WARN",
                 index_ok ? "OK" : "OK_INDEX_WARN");
        return finish_save(entry, true, index_ok, report);
    }

    bool json_ok = circe_entry_save_json_atomic(entry);
    bool index_ok = false;
    if (json_ok) {
        index_ok = circe_entry_index_insert_best_effort(entry);
    } else {
        circe_save_result_t fail = map_json_write_failure();
        const char *err = circe_storage_get_last_error();
        ESP_LOGE(TAG, "save failed: %s err=%s path=%s", circe_save_result_name(fail),
                 err && err[0] ? err : "none", json_path);
        if (report) {
            report->result = fail;
        }
        return fail;
    }

    ESP_LOGI(TAG, "save result json=OK index=%s user=%s", index_ok ? "OK" : "WARN",
             index_ok ? "OK" : "OK_INDEX_WARN");

    return finish_save(entry, json_ok, index_ok, report);
}

circe_save_result_t circe_save_entry(circe_entry_t *entry, bool editing_existing)
{
    return circe_save_entry_report(entry, editing_existing, NULL);
}
