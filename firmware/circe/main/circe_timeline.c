#include "circe_timeline.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "circe_index.h"
#include "circe_buf.h"
#include "circe_storage.h"
#include "circe_terminal.h"
#include "circe_time.h"
#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "circe_timeline";

static circe_timeline_cache_t s_cache;

typedef struct {
    const char *date;
    bool allow_unset;
} filter_date_ctx_t;

typedef struct {
    const char *since_date;
} filter_since_ctx_t;

static bool filter_exact_date(const circe_index_row_t *row, void *ctx)
{
    filter_date_ctx_t *f = ctx;
    if (!row) {
        return false;
    }
    if (f->allow_unset && row->local_date[0] == '\0') {
        return true;
    }
    return row->local_date[0] && f->date && strcmp(row->local_date, f->date) == 0;
}

static bool filter_since_date(const circe_index_row_t *row, void *ctx)
{
    filter_since_ctx_t *f = ctx;
    if (!row || !f || !f->since_date || !f->since_date[0]) {
        return true;
    }
    if (row->local_date[0]) {
        return strcmp(row->local_date, f->since_date) >= 0;
    }
    if (row->created_at[0] && strcmp(row->created_at, "unset") != 0) {
        char day[CIRCE_MAX_DATE];
        strncpy(day, row->created_at, sizeof(day) - 1);
        day[sizeof(day) - 1] = '\0';
        char *t = strchr(day, 'T');
        if (t) {
            *t = '\0';
        }
        return day[0] && strcmp(day, f->since_date) >= 0;
    }
    return false;
}

static bool filter_all(const circe_index_row_t *row, void *ctx)
{
    (void)row;
    (void)ctx;
    return true;
}

static void time_label_from_created_at(const char *created_at, char *out, size_t len)
{
    if (!out || len == 0) {
        return;
    }
    snprintf(out, len, "--:--");
    if (!created_at || created_at[0] == '\0' || strcmp(created_at, "unset") == 0) {
        return;
    }
    const char *t = strchr(created_at, 'T');
    if (t && strlen(t) >= 6) {
        snprintf(out, len, "%02d:%02d", (t[1] - '0') * 10 + (t[2] - '0'), (t[4] - '0') * 10 + (t[5] - '0'));
    }
}

static void upper_field(char *dest, size_t len, const char *src)
{
    if (!dest || len == 0) {
        return;
    }
    circe_terminal_to_upper(dest, len, src ? src : "");
}

static bool item_from_entry(const circe_index_row_t *row, const circe_entry_t *entry, circe_timeline_item_t *item)
{
    if (!row || !entry || !item) {
        return false;
    }
    memset(item, 0, sizeof(*item));
    strncpy(item->entry_id, row->id, sizeof(item->entry_id) - 1);
    time_label_from_created_at(row->created_at, item->time_label, sizeof(item->time_label));
    if (entry->local_date[0]) {
        strncpy(item->date_label, entry->local_date, sizeof(item->date_label) - 1);
    } else if (row->local_date[0]) {
        strncpy(item->date_label, row->local_date, sizeof(item->date_label) - 1);
    }
    item->entry_mode = entry->entry_mode;
    item->intensity = entry->intensity;
    item->color_skipped = entry->color_skipped;
    item->emotion_skipped = entry->emotion_skipped;
    item->has_regulation = entry->has_regulation;
    item->regulation_duration_seconds = entry->regulation_duration_seconds;
    item->regulation_rounds_completed = entry->regulation_rounds_completed;
    item->regulation_session_completed = entry->regulation_session_completed;
    if (entry->body_area_count > 0) {
        upper_field(item->body_area, sizeof(item->body_area), entry->body_areas[0]);
    }
    if (entry->body_sensation_count > 0) {
        upper_field(item->body_sensation, sizeof(item->body_sensation), entry->body_sensations[0]);
    }
    upper_field(item->emotion_label, sizeof(item->emotion_label),
                entry->emotion_label[0] ? entry->emotion_label : "UNKNOWN");
    strncpy(item->color_hex, entry->color_hex, sizeof(item->color_hex) - 1);
    upper_field(item->color_label, sizeof(item->color_label), entry->color_label);
    upper_field(item->regulation_type, sizeof(item->regulation_type),
                entry->regulation_type[0] ? entry->regulation_type : "SESSION");
    return true;
}

const char *circe_timeline_category_title(circe_timeline_category_t category)
{
    switch (category) {
    case CIRCE_TIMELINE_CAT_TODAY:
        return "today";
    case CIRCE_TIMELINE_CAT_YESTERDAY:
        return "yesterday";
    case CIRCE_TIMELINE_CAT_THIS_WEEK:
        return "this week";
    case CIRCE_TIMELINE_CAT_ALL:
        return "all entries";
    default:
        return "memory";
    }
}

void circe_timeline_empty_copy(circe_timeline_category_t category, char *line1, size_t l1, char *line2, size_t l2)
{
    if (line1 && l1) {
        switch (category) {
        case CIRCE_TIMELINE_CAT_TODAY:
            snprintf(line1, l1, "no entries recorded today");
            break;
        case CIRCE_TIMELINE_CAT_YESTERDAY:
            snprintf(line1, l1, "no entry yesterday");
            break;
        case CIRCE_TIMELINE_CAT_THIS_WEEK:
            snprintf(line1, l1, "not enough memory yet");
            break;
        case CIRCE_TIMELINE_CAT_ALL:
            snprintf(line1, l1, "no entries found");
            break;
        default:
            snprintf(line1, l1, "no entries here");
            break;
        }
    }
    if (line2 && l2) {
        switch (category) {
        case CIRCE_TIMELINE_CAT_TODAY:
            if (!circe_time_is_set()) {
                snprintf(line2, l2, "time unset — check UNSET folder");
            } else {
                snprintf(line2, l2, "start with the body when ready");
            }
            break;
        case CIRCE_TIMELINE_CAT_YESTERDAY:
            snprintf(line2, l2, "quiet days are allowed");
            break;
        case CIRCE_TIMELINE_CAT_THIS_WEEK:
            snprintf(line2, l2, "check-ins will appear here");
            break;
        case CIRCE_TIMELINE_CAT_ALL:
            snprintf(line2, l2, "storage is ready");
            break;
        default:
            line2[0] = '\0';
            break;
        }
    }
}

const circe_timeline_cache_t *circe_timeline_get_cache(void)
{
    return &s_cache;
}

static circe_entry_mode_t mode_from_json(cJSON *root)
{
    cJSON *mode = cJSON_GetObjectItem(root, "entry_mode");
    if (!cJSON_IsString(mode) || !mode->valuestring) {
        return CIRCE_ENTRY_MODE_BODY_ONLY;
    }
    if (strcmp(mode->valuestring, "quick") == 0) {
        return CIRCE_ENTRY_MODE_QUICK;
    }
    if (strcmp(mode->valuestring, "regulation") == 0) {
        return CIRCE_ENTRY_MODE_REGULATION;
    }
    return CIRCE_ENTRY_MODE_BODY_ONLY;
}

static bool item_from_json_path(const circe_index_row_t *row, circe_timeline_item_t *item)
{
    if (!row || !item || !row->json_path[0]) {
        return false;
    }
    char *json = NULL;
    if (!circe_json_buf_alloc(&json, CIRCE_JSON_BUF_SIZE)) {
        return false;
    }
    FILE *f = fopen(row->json_path, "r");
    if (!f) {
        circe_json_buf_free(json);
        return false;
    }
    size_t n = fread(json, 1, CIRCE_JSON_BUF_SIZE - 1, f);
    json[n] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(json);
    circe_json_buf_free(json);
    if (!root) {
        return false;
    }

    memset(item, 0, sizeof(*item));
    strncpy(item->entry_id, row->id, sizeof(item->entry_id) - 1);
    time_label_from_created_at(row->created_at, item->time_label, sizeof(item->time_label));
    if (row->local_date[0]) {
        strncpy(item->date_label, row->local_date, sizeof(item->date_label) - 1);
    }

    item->entry_mode = mode_from_json(root);
    if (item->entry_mode == CIRCE_ENTRY_MODE_REGULATION) {
        item->has_regulation = true;
    }
    if (cJSON_GetObjectItem(root, "regulation_type")) {
        item->has_regulation = true;
        cJSON *rt = cJSON_GetObjectItem(root, "regulation_type");
        if (cJSON_IsString(rt) && rt->valuestring) {
            upper_field(item->regulation_type, sizeof(item->regulation_type), rt->valuestring);
        }
        cJSON *dur = cJSON_GetObjectItem(root, "duration_seconds");
        if (cJSON_IsNumber(dur)) {
            item->regulation_duration_seconds = dur->valueint;
        }
        cJSON *rounds = cJSON_GetObjectItem(root, "rounds_completed");
        if (cJSON_IsNumber(rounds)) {
            item->regulation_rounds_completed = rounds->valueint;
        }
        cJSON *done = cJSON_GetObjectItem(root, "session_completed");
        if (cJSON_IsBool(done)) {
            item->regulation_session_completed = cJSON_IsTrue(done);
        }
    }

    cJSON *areas = cJSON_GetObjectItem(root, "body_areas");
    if (cJSON_IsArray(areas) && cJSON_GetArraySize(areas) > 0) {
        cJSON *a0 = cJSON_GetArrayItem(areas, 0);
        if (cJSON_IsString(a0) && a0->valuestring) {
            upper_field(item->body_area, sizeof(item->body_area), a0->valuestring);
        }
    }

    cJSON *intensity = cJSON_GetObjectItem(root, "intensity");
    if (cJSON_IsNumber(intensity)) {
        item->intensity = intensity->valueint;
    }

    cJSON *el = cJSON_GetObjectItem(root, "emotion_label");
    if (cJSON_IsString(el) && el->valuestring) {
        upper_field(item->emotion_label, sizeof(item->emotion_label), el->valuestring);
    }
    cJSON *es = cJSON_GetObjectItem(root, "emotion_skipped");
    if (cJSON_IsBool(es)) {
        item->emotion_skipped = cJSON_IsTrue(es);
    }

    cJSON *cs = cJSON_GetObjectItem(root, "color_skipped");
    if (cJSON_IsBool(cs)) {
        item->color_skipped = cJSON_IsTrue(cs);
    }
    cJSON *hex = cJSON_GetObjectItem(root, "color_hex");
    if (cJSON_IsString(hex) && hex->valuestring && hex->valuestring[0] == '#') {
        strncpy(item->color_hex, hex->valuestring, sizeof(item->color_hex) - 1);
    }
    cJSON *cl = cJSON_GetObjectItem(root, "color_label");
    if (cJSON_IsString(cl) && cl->valuestring) {
        upper_field(item->color_label, sizeof(item->color_label), cl->valuestring);
    }

    cJSON_Delete(root);
    return true;
}

bool circe_timeline_load_pattern_context(circe_timeline_item_t *items, int max_items, int *out_count)
{
    if (!items || max_items <= 0 || !out_count) {
        return false;
    }
    *out_count = 0;

    if (!circe_storage_is_ready()) {
        return false;
    }
    circe_storage_rebuild_index_if_dirty(NULL);

    circe_index_row_t *rows = circe_buf_alloc(sizeof(circe_index_row_t) * CIRCE_TIMELINE_MAX_ITEMS);
    if (!rows) {
        return false;
    }

    int row_count = 0;
    bool more = false;
    bool ok = true;
    char today[CIRCE_MAX_DATE] = {0};
    char week_start[CIRCE_MAX_DATE] = {0};
    circe_time_format_date(today, sizeof(today));
    circe_time_offset_date(-6, week_start, sizeof(week_start));

    if (circe_time_is_set()) {
        filter_since_ctx_t fctx = {.since_date = week_start[0] ? week_start : today};
        ok = circe_index_list_collect(rows, CIRCE_TIMELINE_MAX_ITEMS, &row_count, &more, filter_since_date, &fctx);
    } else {
        ok = circe_index_list_collect(rows, CIRCE_TIMELINE_MAX_ITEMS, &row_count, &more, filter_all, NULL);
    }

    if (!ok) {
        circe_buf_free(rows);
        return false;
    }

    int limit = row_count < max_items ? row_count : max_items;
    for (int i = 0; i < limit; i++) {
        if (!item_from_json_path(&rows[i], &items[*out_count])) {
            continue;
        }
        (*out_count)++;
    }

    circe_buf_free(rows);
    ESP_LOGI(TAG, "pattern context: %d items", *out_count);
    return *out_count > 0;
}

bool circe_timeline_load_category(circe_timeline_category_t category, circe_timeline_cache_t *cache)
{
    if (!cache) {
        return false;
    }
    memset(cache, 0, sizeof(*cache));
    cache->category = category;

    if (!circe_storage_is_ready()) {
        snprintf(cache->status_msg, sizeof(cache->status_msg), "storage unavailable");
        cache->index_error = true;
        s_cache = *cache;
        return false;
    }

    circe_storage_rebuild_index_if_dirty(NULL);

    circe_index_row_t rows[CIRCE_TIMELINE_MAX_ITEMS];
    int row_count = 0;
    bool more = false;
    bool ok = true;

    char today[CIRCE_MAX_DATE] = {0};
    char yesterday[CIRCE_MAX_DATE] = {0};
    char week_start[CIRCE_MAX_DATE] = {0};
    circe_time_format_date(today, sizeof(today));
    circe_time_offset_date(-1, yesterday, sizeof(yesterday));
    circe_time_offset_date(-6, week_start, sizeof(week_start));

    if (!circe_time_is_set()) {
        cache->time_unset_note = true;
    }

    switch (category) {
    case CIRCE_TIMELINE_CAT_TODAY: {
        filter_date_ctx_t fctx = {.date = today, .allow_unset = !circe_time_is_set()};
        if (today[0] == '\0' || strcmp(today, "--") == 0) {
            fctx.date = NULL;
        }
        ok = circe_index_list_collect(rows, CIRCE_TIMELINE_MAX_ITEMS, &row_count, &more, filter_exact_date, &fctx);
        break;
    }
    case CIRCE_TIMELINE_CAT_YESTERDAY: {
        if (!yesterday[0]) {
            row_count = 0;
            break;
        }
        filter_date_ctx_t fctx = {.date = yesterday, .allow_unset = false};
        ok = circe_index_list_collect(rows, CIRCE_TIMELINE_MAX_ITEMS, &row_count, &more, filter_exact_date, &fctx);
        break;
    }
    case CIRCE_TIMELINE_CAT_THIS_WEEK: {
        filter_since_ctx_t fctx = {.since_date = week_start[0] ? week_start : today};
        ok = circe_index_list_collect(rows, CIRCE_TIMELINE_MAX_ITEMS, &row_count, &more, filter_since_date, &fctx);
        break;
    }
    case CIRCE_TIMELINE_CAT_ALL:
        ok = circe_index_list_collect(rows, CIRCE_TIMELINE_MAX_ITEMS, &row_count, &more, filter_all, NULL);
        break;
    default:
        ok = false;
        break;
    }

    if (!ok) {
        snprintf(cache->status_msg, sizeof(cache->status_msg), "memory index needs repair");
        cache->index_error = true;
        s_cache = *cache;
        return false;
    }

    cache->truncated = more;
    for (int i = 0; i < row_count; i++) {
        circe_entry_t entry;
        if (!circe_entry_load(rows[i].id, &entry)) {
            ESP_LOGW(TAG, "skip missing entry id=%s", rows[i].id);
            continue;
        }
        if (cache->count >= CIRCE_TIMELINE_MAX_ITEMS) {
            cache->truncated = true;
            break;
        }
        item_from_entry(&rows[i], &entry, &cache->items[cache->count]);
        cache->count++;
    }

    if (cache->truncated) {
        snprintf(cache->status_msg, sizeof(cache->status_msg), "showing recent %d", cache->count);
    }

    s_cache = *cache;
    ESP_LOGI(TAG, "timeline %s: %d items truncated=%d", circe_timeline_category_title(category), cache->count,
             cache->truncated ? 1 : 0);
    return true;
}

void circe_timeline_item_format_lines(const circe_timeline_item_t *item, char *line1, size_t l1, char *line2,
                                      size_t l2, char *line3, size_t l3, char *line4, size_t l4)
{
    if (!item) {
        return;
    }
    if (line1 && l1) {
        snprintf(line1, l1, "%s", item->time_label);
    }
    if (item->entry_mode == CIRCE_ENTRY_MODE_REGULATION || item->has_regulation) {
        if (line2 && l2) {
            snprintf(line2, l2, "REGULATION %s", item->regulation_type[0] ? item->regulation_type : "SESSION");
        }
        if (line3 && l3) {
            snprintf(line3, l3, "%ds / %d rounds", item->regulation_duration_seconds,
                     item->regulation_rounds_completed);
        }
        if (line4 && l4) {
            snprintf(line4, l4, "COMPLETED %s", item->regulation_session_completed ? "YES" : "NO");
        }
        return;
    }
    if (line2 && l2) {
        if (item->body_area[0] && item->body_sensation[0]) {
            snprintf(line2, l2, "%s / %s / %d", item->body_area, item->body_sensation, item->intensity);
        } else if (item->body_area[0]) {
            snprintf(line2, l2, "%s / %d", item->body_area, item->intensity);
        } else {
            snprintf(line2, l2, "ENTRY SAVED / %d", item->intensity);
        }
    }
    if (line3 && l3) {
        if (item->emotion_skipped || !item->emotion_label[0] ||
            strcasecmp(item->emotion_label, "UNKNOWN") == 0) {
            snprintf(line3, l3, "TONE UNKNOWN");
        } else {
            snprintf(line3, l3, "TONE %s", item->emotion_label);
        }
    }
    if (line4 && l4) {
        if (item->color_skipped || item->color_hex[0] != '#') {
            snprintf(line4, l4, "COLOR SKIPPED");
        } else if (strcasecmp(item->color_label, "CUSTOM") == 0 || item->color_label[0] == '\0') {
            snprintf(line4, l4, "COLOR %s", item->color_hex);
        } else {
            snprintf(line4, l4, "COLOR %s %s", item->color_label, item->color_hex);
        }
    }
}
