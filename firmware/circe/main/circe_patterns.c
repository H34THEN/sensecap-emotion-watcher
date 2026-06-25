#include "circe_patterns.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "circe_buf.h"
#include "circe_color_intel.h"
#include "circe_copy.h"
#include "circe_storage.h"
#include "circe_timeline.h"
#include "esp_log.h"

static const char *TAG = "circe_patterns";

static circe_patterns_result_t s_cache;

static bool tone_is_unknown(const char *tone)
{
    return !tone || !tone[0] || strcasecmp(tone, "UNKNOWN") == 0;
}

static bool item_is_regulation(const circe_timeline_item_t *item)
{
    return item && (item->entry_mode == CIRCE_ENTRY_MODE_REGULATION || item->has_regulation);
}

static int count_color_trait(const circe_timeline_item_t *items, int n, const char *trait, const char *value)
{
    int c = 0;
    circe_color_intel_t intel;
    for (int i = 0; i < n; i++) {
        if (items[i].color_skipped || items[i].color_hex[0] != '#') {
            continue;
        }
        if (!circe_color_intel_from_timeline_item(&items[i], &intel)) {
            continue;
        }
        const char *candidate = NULL;
        if (strcmp(trait, "temperature") == 0) {
            candidate = intel.temperature;
        } else if (strcmp(trait, "saturation_label") == 0) {
            candidate = intel.saturation_label;
        } else if (strcmp(trait, "brightness_label") == 0) {
            candidate = intel.brightness_label;
        } else if (strcmp(trait, "family") == 0) {
            candidate = intel.family;
        }
        if (candidate && value && strcasecmp(candidate, value) == 0) {
            c++;
        }
    }
    return c;
}

static int count_area(const circe_timeline_item_t *items, int n, const char *area)
{
    int c = 0;
    for (int i = 0; i < n; i++) {
        if (area && area[0] && items[i].body_area[0] && strcasecmp(items[i].body_area, area) == 0) {
            c++;
        }
    }
    return c;
}

static int count_sensation(const circe_timeline_item_t *items, int n, const char *sensation)
{
    int c = 0;
    for (int i = 0; i < n; i++) {
        if (sensation && sensation[0] && items[i].body_sensation[0] &&
            strcasecmp(items[i].body_sensation, sensation) == 0) {
            c++;
        }
    }
    return c;
}

static int count_tone(const circe_timeline_item_t *items, int n, const char *tone)
{
    if (tone_is_unknown(tone)) {
        return 0;
    }
    int c = 0;
    for (int i = 0; i < n; i++) {
        if (!tone_is_unknown(items[i].emotion_label) && strcasecmp(items[i].emotion_label, tone) == 0) {
            c++;
        }
    }
    return c;
}

static int count_high_intensity(const circe_timeline_item_t *items, int n)
{
    int c = 0;
    for (int i = 0; i < n; i++) {
        if (items[i].intensity >= 8) {
            c++;
        }
    }
    return c;
}

static int count_regulation(const circe_timeline_item_t *items, int n)
{
    int c = 0;
    for (int i = 0; i < n; i++) {
        if (item_is_regulation(&items[i])) {
            c++;
        }
    }
    return c;
}

static bool find_best_label(const circe_timeline_item_t *items, int n, char *label, size_t label_len,
                            int (*counter)(const circe_timeline_item_t *, int, const char *),
                            const char *(*getter)(const circe_timeline_item_t *))
{
    int best = 0;
    label[0] = '\0';
    for (int i = 0; i < n; i++) {
        const char *candidate = getter(&items[i]);
        if (!candidate || !candidate[0]) {
            continue;
        }
        bool seen = false;
        for (int j = 0; j < i; j++) {
            const char *prev = getter(&items[j]);
            if (prev && strcasecmp(prev, candidate) == 0) {
                seen = true;
                break;
            }
        }
        if (seen) {
            continue;
        }
        int c = counter(items, n, candidate);
        if (c > best) {
            best = c;
            strncpy(label, candidate, label_len - 1);
            label[label_len - 1] = '\0';
        }
    }
    return best > 0;
}

static const char *get_body_area(const circe_timeline_item_t *item)
{
    return item ? item->body_area : NULL;
}

static const char *get_sensation(const circe_timeline_item_t *item)
{
    return item ? item->body_sensation : NULL;
}

static const char *get_tone(const circe_timeline_item_t *item)
{
    return item && !tone_is_unknown(item->emotion_label) ? item->emotion_label : NULL;
}

static void set_observation_subline(circe_pattern_summary_t *p)
{
    strncpy(p->subline, circe_copy_get(CIRCE_PATTERN_PATTERNS_OBSERVATION_SUBLINE), sizeof(p->subline) - 1);
    p->subline[sizeof(p->subline) - 1] = '\0';
}

static void push_pattern(circe_pattern_summary_t *detected, int *detected_count, const circe_pattern_summary_t *src)
{
    if (!detected || !detected_count || !src || *detected_count >= CIRCE_PATTERNS_DETECT_MAX) {
        return;
    }
    for (int i = 0; i < *detected_count; i++) {
        if (detected[i].kind == src->kind) {
            return;
        }
    }
    detected[*detected_count] = *src;
    (*detected_count)++;
}

static void maybe_add_color_pattern(circe_pattern_summary_t *detected, int *detected_count, const circe_timeline_item_t *items,
                                    int count, const char *trait, const char *value, const char *primary)
{
    int n = count_color_trait(items, count, trait, value);
    if (n < 3) {
        return;
    }
    circe_pattern_summary_t p = {0};
    p.kind = CIRCE_PATTERN_KIND_COLOR_FAMILY;
    p.count = n;
    p.total_entries = count;
    snprintf(p.primary, sizeof(p.primary), "%s", primary);
    set_observation_subline(&p);
    push_pattern(detected, detected_count, &p);
}

static void trim_to_display(const circe_pattern_summary_t *detected, int detected_count, circe_patterns_result_t *out)
{
    out->count = detected_count > CIRCE_PATTERNS_DISPLAY_MAX ? CIRCE_PATTERNS_DISPLAY_MAX : detected_count;
    for (int i = 0; i < out->count; i++) {
        out->items[i] = detected[i];
    }
}

const circe_patterns_result_t *circe_patterns_get_cache(void)
{
    return &s_cache;
}

bool circe_patterns_any_suggest_regulate(const circe_patterns_result_t *result)
{
    if (!result) {
        return false;
    }
    for (int i = 0; i < result->count; i++) {
        if (result->items[i].suggest_regulate) {
            return true;
        }
    }
    return false;
}

bool circe_patterns_scan(circe_patterns_result_t *out)
{
    if (!out) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    s_cache = *out;

    if (!circe_storage_is_ready()) {
        out->storage_unavailable = true;
        s_cache = *out;
        ESP_LOGW(TAG, "scan: storage unavailable");
        return false;
    }

    circe_timeline_item_t *items =
        circe_buf_alloc((size_t)CIRCE_PATTERNS_SCAN_MAX * sizeof(circe_timeline_item_t));
    if (!items) {
        out->index_error = true;
        s_cache = *out;
        ESP_LOGE(TAG, "scan: alloc failed");
        return false;
    }

    int count = 0;
    if (!circe_timeline_load_pattern_context(items, CIRCE_PATTERNS_SCAN_MAX, &count)) {
        circe_buf_free(items);
        out->index_error = true;
        s_cache = *out;
        ESP_LOGW(TAG, "scan: load failed");
        return false;
    }

    out->total_entries = count;
    out->scan_ok = true;

    if (count < CIRCE_PATTERNS_MIN_ENTRIES) {
        out->not_enough_entries = true;
        circe_buf_free(items);
        s_cache = *out;
        ESP_LOGI(TAG, "scan: only %d entries", count);
        return true;
    }

    circe_pattern_summary_t detected[CIRCE_PATTERNS_DETECT_MAX];
    int detected_count = 0;

    char label[32];
    int n = 0;

    if (find_best_label(items, count, label, sizeof(label), count_area, get_body_area)) {
        n = count_area(items, count, label);
        if (n >= 3) {
            circe_pattern_summary_t p = {0};
            p.kind = CIRCE_PATTERN_KIND_BODY_AREA;
            p.count = n;
            p.total_entries = count;
            strncpy(p.label, label, sizeof(p.label) - 1);
            snprintf(p.primary, sizeof(p.primary), "%s has appeared often recently.", label);
            set_observation_subline(&p);
            push_pattern(detected, &detected_count, &p);
        }
    }

    if (find_best_label(items, count, label, sizeof(label), count_sensation, get_sensation)) {
        n = count_sensation(items, count, label);
        if (n >= 3) {
            circe_pattern_summary_t p = {0};
            p.kind = CIRCE_PATTERN_KIND_SENSATION;
            p.count = n;
            p.total_entries = count;
            strncpy(p.label, label, sizeof(p.label) - 1);
            snprintf(p.primary, sizeof(p.primary), "%s has appeared more than once.", label);
            set_observation_subline(&p);
            push_pattern(detected, &detected_count, &p);
        }
    }

    if (find_best_label(items, count, label, sizeof(label), count_tone, get_tone)) {
        n = count_tone(items, count, label);
        if (n >= 3) {
            circe_pattern_summary_t p = {0};
            p.kind = CIRCE_PATTERN_KIND_TONE;
            p.count = n;
            p.total_entries = count;
            strncpy(p.label, label, sizeof(p.label) - 1);
            snprintf(p.primary, sizeof(p.primary), "%s has shown up recently.", label);
            set_observation_subline(&p);
            push_pattern(detected, &detected_count, &p);
        }
    }

    n = count_high_intensity(items, count);
    if (n >= 2) {
        circe_pattern_summary_t p = {0};
        p.kind = CIRCE_PATTERN_KIND_HIGH_INTENSITY;
        p.count = n;
        p.total_entries = count;
        snprintf(p.primary, sizeof(p.primary), "Strong body signals appeared more than once.");
        strncpy(p.subline, circe_copy_get(CIRCE_PATTERN_PATTERNS_GROUNDING_SUBLINE), sizeof(p.subline) - 1);
        p.suggest_regulate = true;
        push_pattern(detected, &detected_count, &p);
    }

    n = count_regulation(items, count);
    if (n >= 2) {
        circe_pattern_summary_t p = {0};
        p.kind = CIRCE_PATTERN_KIND_REGULATION;
        p.count = n;
        p.total_entries = count;
        snprintf(p.primary, sizeof(p.primary), "You have returned to regulation recently.");
        set_observation_subline(&p);
        push_pattern(detected, &detected_count, &p);
    }

    maybe_add_color_pattern(detected, &detected_count, items, count, "temperature", "cool",
                            "Your recent colors have stayed mostly cool.");
    maybe_add_color_pattern(detected, &detected_count, items, count, "temperature", "warm",
                            "Your recent colors have stayed mostly warm.");
    maybe_add_color_pattern(detected, &detected_count, items, count, "saturation_label", "muted",
                            "Your recent colors have stayed mostly muted.");
    maybe_add_color_pattern(detected, &detected_count, items, count, "brightness_label", "dark",
                            "Dark colors have appeared often recently.");
    maybe_add_color_pattern(detected, &detected_count, items, count, "brightness_label", "bright",
                            "Bright colors have appeared often recently.");

    circe_buf_free(items);

    if (detected_count == 0) {
        out->no_patterns = true;
    } else {
        trim_to_display(detected, detected_count, out);
    }

    s_cache = *out;
    ESP_LOGI(TAG, "scan: %d entries, %d patterns", count, out->count);
    return true;
}
