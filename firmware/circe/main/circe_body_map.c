#include "circe_body_map.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "circe_buf.h"
#include "circe_storage.h"
#include "circe_timeline.h"
#include "esp_log.h"

static const char *TAG = "circe_body_map";

typedef struct {
    char body_area[24];
    int count;
    int max_intensity;
    int intensity_sum;
    int high_count;
    int score;
} agg_row_t;

static bool item_is_regulation(const circe_timeline_item_t *item)
{
    return item && (item->entry_mode == CIRCE_ENTRY_MODE_REGULATION || item->has_regulation);
}

static bool item_has_body(const circe_timeline_item_t *item)
{
    return item && !item_is_regulation(item) && item->body_area[0] != '\0';
}

static int find_agg(agg_row_t *rows, int n, const char *area)
{
    for (int i = 0; i < n; i++) {
        if (strcasecmp(rows[i].body_area, area) == 0) {
            return i;
        }
    }
    return -1;
}

static void sort_rows(agg_row_t *rows, int n)
{
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            bool swap = false;
            if (rows[j].score > rows[i].score) {
                swap = true;
            } else if (rows[j].score == rows[i].score) {
                if (rows[j].count > rows[i].count) {
                    swap = true;
                } else if (rows[j].count == rows[i].count && rows[j].max_intensity > rows[i].max_intensity) {
                    swap = true;
                }
            }
            if (swap) {
                agg_row_t tmp = rows[i];
                rows[i] = rows[j];
                rows[j] = tmp;
            }
        }
    }
}

void circe_body_map_format_row(char *buf, size_t len, const circe_body_map_row_t *row, int max_score)
{
    if (!buf || len == 0 || !row) {
        return;
    }
    int bar_len = 0;
    if (max_score > 0 && row->score > 0) {
        bar_len = (row->score * 7) / max_score;
        if (bar_len < 1) {
            bar_len = 1;
        }
    }
    if (bar_len > 7) {
        bar_len = 7;
    }
    char bars[8];
    memset(bars, 0, sizeof(bars));
    for (int i = 0; i < bar_len; i++) {
        bars[i] = '#';
    }
    snprintf(buf, len, "%-10s %s", row->body_area, bars);
}

void circe_body_map_format_detail(char *buf, size_t len, const circe_body_map_row_t *row)
{
    if (!buf || len == 0 || !row || !row->body_area[0]) {
        return;
    }
    if (row->max_intensity >= 8) {
        snprintf(buf, len, "%s strongest signal: %d", row->body_area, row->max_intensity);
    } else {
        snprintf(buf, len, "%s appeared %d times", row->body_area, row->count);
    }
}

bool circe_body_map_load(circe_body_map_summary_t *out)
{
    if (!out) {
        return false;
    }
    memset(out, 0, sizeof(*out));

    if (!circe_storage_is_ready()) {
        out->state = CIRCE_BODY_MAP_STATE_STORAGE;
        out->valid = true;
        return false;
    }

    circe_timeline_item_t *items =
        circe_buf_alloc((size_t)CIRCE_BODY_MAP_SCAN_MAX * sizeof(circe_timeline_item_t));
    if (!items) {
        out->state = CIRCE_BODY_MAP_STATE_ERROR;
        out->valid = true;
        return false;
    }

    int count = 0;
    if (!circe_timeline_load_pattern_context(items, CIRCE_BODY_MAP_SCAN_MAX, &count)) {
        circe_buf_free(items);
        out->state = CIRCE_BODY_MAP_STATE_ERROR;
        out->valid = true;
        return false;
    }

    agg_row_t agg[CIRCE_BODY_MAP_MAX_ROWS * 2];
    int agg_n = 0;
    int usable = 0;

    for (int i = 0; i < count; i++) {
        if (!item_has_body(&items[i])) {
            continue;
        }
        usable++;
        int idx = find_agg(agg, agg_n, items[i].body_area);
        if (idx < 0) {
            if (agg_n >= (int)(sizeof(agg) / sizeof(agg[0]))) {
                out->truncated = true;
                continue;
            }
            idx = agg_n++;
            memset(&agg[idx], 0, sizeof(agg[idx]));
            strncpy(agg[idx].body_area, items[i].body_area, sizeof(agg[idx].body_area) - 1);
        }
        agg[idx].count++;
        agg[idx].intensity_sum += items[i].intensity;
        if (items[i].intensity > agg[idx].max_intensity) {
            agg[idx].max_intensity = items[i].intensity;
        }
        if (items[i].intensity >= 8) {
            agg[idx].high_count++;
        }
        agg[idx].score = agg[idx].count + agg[idx].high_count;
    }

    circe_buf_free(items);
    out->total_entries = usable;

    if (usable < CIRCE_BODY_MAP_MIN_ENTRIES) {
        out->state = CIRCE_BODY_MAP_STATE_EMPTY;
        out->valid = true;
        ESP_LOGI(TAG, "body map: only %d usable entries", usable);
        return true;
    }

    sort_rows(agg, agg_n);
    out->row_count = agg_n < CIRCE_BODY_MAP_MAX_ROWS ? agg_n : CIRCE_BODY_MAP_MAX_ROWS;
    for (int i = 0; i < out->row_count; i++) {
        out->rows[i].count = agg[i].count;
        out->rows[i].max_intensity = agg[i].max_intensity;
        out->rows[i].score = agg[i].score;
        out->rows[i].avg_intensity = agg[i].count > 0 ? agg[i].intensity_sum / agg[i].count : 0;
        strncpy(out->rows[i].body_area, agg[i].body_area, sizeof(out->rows[i].body_area) - 1);
    }

    circe_body_map_format_detail(out->detail_line, sizeof(out->detail_line), &out->rows[0]);
    out->state = CIRCE_BODY_MAP_STATE_OK;
    out->valid = true;
    ESP_LOGI(TAG, "body map: %d rows from %d entries", out->row_count, usable);
    return true;
}
