#include "circe_daily.h"

#include <stdio.h>
#include <string.h>

#include "circe_copy.h"
#include "circe_storage.h"
#include "circe_time.h"
#include "circe_timeline.h"
#include "esp_log.h"

static const char *TAG = "circe_daily";

static bool item_is_regulation(const circe_timeline_item_t *item)
{
    return item && (item->has_regulation || item->entry_mode == CIRCE_ENTRY_MODE_REGULATION);
}

static int count_body_area(const circe_timeline_item_t *items, int n, const char *area)
{
    int c = 0;
    for (int i = 0; i < n; i++) {
        if (item_is_regulation(&items[i])) {
            continue;
        }
        if (area && items[i].body_area[0] && strcasecmp(items[i].body_area, area) == 0) {
            c++;
        }
    }
    return c;
}

static bool find_repeated_body_area(const circe_timeline_item_t *items, int n, char *area, size_t area_len,
                                    int *out_count)
{
    int best = 0;
    area[0] = '\0';
    if (out_count) {
        *out_count = 0;
    }
    for (int i = 0; i < n; i++) {
        if (item_is_regulation(&items[i]) || !items[i].body_area[0]) {
            continue;
        }
        bool seen = false;
        for (int j = 0; j < i; j++) {
            if (items[j].body_area[0] && strcasecmp(items[j].body_area, items[i].body_area) == 0) {
                seen = true;
                break;
            }
        }
        if (seen) {
            continue;
        }
        int c = count_body_area(items, n, items[i].body_area);
        if (c > best) {
            best = c;
            strncpy(area, items[i].body_area, area_len - 1);
            area[area_len - 1] = '\0';
        }
    }
    if (best >= 2 && out_count) {
        *out_count = best;
    }
    return best >= 2;
}

circe_daily_period_t circe_daily_period_from_hour(int hour)
{
    if (hour >= 5 && hour <= 11) {
        return CIRCE_DAILY_PERIOD_MORNING;
    }
    if (hour >= 12 && hour <= 16) {
        return CIRCE_DAILY_PERIOD_AFTERNOON;
    }
    if (hour >= 17 && hour <= 21) {
        return CIRCE_DAILY_PERIOD_EVENING;
    }
    return CIRCE_DAILY_PERIOD_NIGHT;
}

static const char *period_greeting(circe_daily_period_t period)
{
    switch (period) {
    case CIRCE_DAILY_PERIOD_MORNING:
        return circe_copy_get(CIRCE_PATTERN_DAILY_GOOD_MORNING);
    case CIRCE_DAILY_PERIOD_AFTERNOON:
        return circe_copy_get(CIRCE_PATTERN_DAILY_GOOD_AFTERNOON);
    case CIRCE_DAILY_PERIOD_EVENING:
        return circe_copy_get(CIRCE_PATTERN_DAILY_GOOD_EVENING);
    case CIRCE_DAILY_PERIOD_NIGHT:
        return circe_copy_get(CIRCE_PATTERN_DAILY_GOOD_NIGHT);
    case CIRCE_DAILY_PERIOD_NEUTRAL:
    default:
        return circe_copy_get(CIRCE_PATTERN_DAILY_READY);
    }
}

void circe_daily_build_copy(circe_daily_summary_t *summary, bool index_error, bool storage_unavailable)
{
    if (!summary) {
        return;
    }

    summary->primary_line[0] = '\0';
    summary->subline[0] = '\0';

    if (storage_unavailable || !circe_storage_is_ready()) {
        strncpy(summary->primary_line, circe_copy_get(CIRCE_PATTERN_DAILY_MEMORY_UNAVAILABLE),
                sizeof(summary->primary_line) - 1);
        strncpy(summary->subline, circe_copy_get(CIRCE_PATTERN_STORAGE_UNAVAILABLE_2), sizeof(summary->subline) - 1);
        summary->valid = true;
        return;
    }

    if (index_error) {
        strncpy(summary->primary_line, circe_copy_get(CIRCE_PATTERN_DAILY_MEMORY_LOADING),
                sizeof(summary->primary_line) - 1);
        strncpy(summary->subline, circe_copy_get(CIRCE_PATTERN_DAILY_START_BODY), sizeof(summary->subline) - 1);
        summary->valid = true;
        return;
    }

    if (!summary->time_set) {
        strncpy(summary->primary_line, circe_copy_get(CIRCE_PATTERN_DAILY_TIME_UNSET), sizeof(summary->primary_line) - 1);
        strncpy(summary->subline, circe_copy_get(CIRCE_PATTERN_DAILY_TIME_UNSET_SUB), sizeof(summary->subline) - 1);
        summary->valid = true;
        return;
    }

    circe_daily_period_t period = circe_daily_period_from_hour(summary->local_hour);

    if (summary->entries_today <= 0 && summary->regulation_today <= 0) {
        if (period == CIRCE_DAILY_PERIOD_MORNING || period == CIRCE_DAILY_PERIOD_EVENING ||
            period == CIRCE_DAILY_PERIOD_NIGHT) {
            strncpy(summary->primary_line, period_greeting(period), sizeof(summary->primary_line) - 1);
            strncpy(summary->subline, circe_copy_get(CIRCE_PATTERN_DAILY_START_BODY), sizeof(summary->subline) - 1);
        } else {
            strncpy(summary->primary_line, circe_copy_get(CIRCE_PATTERN_DAILY_NO_ENTRY_TODAY),
                    sizeof(summary->primary_line) - 1);
            strncpy(summary->subline, circe_copy_get(CIRCE_PATTERN_DAILY_QUIET_ALLOWED), sizeof(summary->subline) - 1);
        }
        summary->valid = true;
        return;
    }

    if (summary->entries_today <= 0 && summary->regulation_today > 0) {
        strncpy(summary->primary_line, circe_copy_get(CIRCE_PATTERN_DAILY_REGULATION_RECORDED),
                sizeof(summary->primary_line) - 1);
        strncpy(summary->subline, circe_copy_get(CIRCE_PATTERN_DAILY_REGULATION_SUB), sizeof(summary->subline) - 1);
        summary->valid = true;
        return;
    }

    snprintf(summary->primary_line, sizeof(summary->primary_line), "%d %s", summary->entries_today,
             summary->entries_today == 1 ? "ENTRY TODAY" : "ENTRIES TODAY");

    if (summary->repeated_body_count >= 2 && summary->repeated_body_area[0]) {
        snprintf(summary->subline, sizeof(summary->subline), "%s appeared %d times.", summary->repeated_body_area,
                 summary->repeated_body_count);
    } else if (summary->regulation_today > 0) {
        strncpy(summary->subline, circe_copy_get(CIRCE_PATTERN_DAILY_REGULATION_SUB), sizeof(summary->subline) - 1);
    } else {
        strncpy(summary->subline, circe_copy_get(CIRCE_PATTERN_DAILY_BODY_HEARD), sizeof(summary->subline) - 1);
    }

    summary->valid = true;
}

bool circe_daily_load(circe_daily_summary_t *out)
{
    if (!out) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    out->time_set = circe_time_is_set();
    if (out->time_set) {
        int year = 0;
        int month = 0;
        int day = 0;
        int minute = 0;
        circe_time_get_local(&year, &month, &day, &out->local_hour, &minute);
    }

    if (!circe_storage_is_ready()) {
        circe_daily_build_copy(out, false, true);
        return false;
    }

    circe_timeline_cache_t cache = {0};
    if (!circe_timeline_load_category(CIRCE_TIMELINE_CAT_TODAY, &cache)) {
        circe_daily_build_copy(out, true, false);
        return false;
    }

    for (int i = 0; i < cache.count; i++) {
        const circe_timeline_item_t *item = &cache.items[i];
        if (item_is_regulation(item)) {
            out->regulation_today++;
            continue;
        }
        out->entries_today++;
        if (item->intensity >= 8) {
            out->high_intensity_today++;
        }
    }

    find_repeated_body_area(cache.items, cache.count, out->repeated_body_area, sizeof(out->repeated_body_area),
                            &out->repeated_body_count);

    circe_daily_build_copy(out, false, false);
    ESP_LOGI(TAG, "daily summary entries=%d regulation=%d high=%d area=%s x%d", out->entries_today,
             out->regulation_today, out->high_intensity_today, out->repeated_body_area, out->repeated_body_count);
    return true;
}
