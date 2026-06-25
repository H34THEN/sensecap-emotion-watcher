#include "circe_reflection.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "circe_time.h"
#include "esp_log.h"

static const char *TAG = "circe_reflection";

static circe_timeline_item_t s_recent[CIRCE_TIMELINE_MAX_ITEMS];
static int s_recent_count;
static bool s_recent_loaded;

void circe_reflection_clear(circe_reflection_t *reflection)
{
    if (!reflection) {
        return;
    }
    memset(reflection, 0, sizeof(*reflection));
}

void circe_reflection_clear_recent_context(void)
{
    s_recent_loaded = false;
    s_recent_count = 0;
}

bool circe_reflection_load_recent_context(void)
{
    circe_timeline_cache_t cache = {0};
    bool ok;
    if (circe_time_is_set()) {
        ok = circe_timeline_load_category(CIRCE_TIMELINE_CAT_THIS_WEEK, &cache);
    } else {
        ok = circe_timeline_load_category(CIRCE_TIMELINE_CAT_ALL, &cache);
    }
    if (!ok || cache.index_error || cache.count <= 0) {
        circe_reflection_clear_recent_context();
        ESP_LOGW(TAG, "recent context unavailable");
        return false;
    }
    s_recent_count = cache.count;
    if (s_recent_count > CIRCE_TIMELINE_MAX_ITEMS) {
        s_recent_count = CIRCE_TIMELINE_MAX_ITEMS;
    }
    memcpy(s_recent, cache.items, (size_t)s_recent_count * sizeof(s_recent[0]));
    s_recent_loaded = true;
    ESP_LOGI(TAG, "recent context: %d entries", s_recent_count);
    return true;
}

static bool tone_is_unknown(const circe_entry_t *entry)
{
    if (!entry) {
        return true;
    }
    if (entry->emotion_skipped) {
        return true;
    }
    if (entry->emotion_label[0] == '\0') {
        return true;
    }
    return strcasecmp(entry->emotion_label, "UNKNOWN") == 0;
}

static bool item_is_regulation(const circe_timeline_item_t *item)
{
    return item && (item->entry_mode == CIRCE_ENTRY_MODE_REGULATION || item->has_regulation);
}

static void copy_main(circe_reflection_t *out, const char *text)
{
    if (!out || !text) {
        return;
    }
    strncpy(out->main_text, text, sizeof(out->main_text) - 1);
    out->main_text[sizeof(out->main_text) - 1] = '\0';
}

static void copy_sub(circe_reflection_t *out, const char *text)
{
    if (!out || !text) {
        return;
    }
    strncpy(out->subline, text, sizeof(out->subline) - 1);
    out->subline[sizeof(out->subline) - 1] = '\0';
}

static bool areas_match(const char *a, const char *b)
{
    if (!a || !b || !a[0] || !b[0]) {
        return false;
    }
    return strcasecmp(a, b) == 0;
}

static bool tones_match(const char *a, const char *b)
{
    if (!a || !b || !a[0] || !b[0]) {
        return false;
    }
    if (strcasecmp(a, "UNKNOWN") == 0 || strcasecmp(b, "UNKNOWN") == 0) {
        return false;
    }
    return strcasecmp(a, b) == 0;
}

static int count_high_intensity(void)
{
    int n = 0;
    for (int i = 0; i < s_recent_count; i++) {
        if (s_recent[i].intensity >= 8) {
            n++;
        }
    }
    return n;
}

static int count_body_area(const char *area)
{
    int n = 0;
    for (int i = 0; i < s_recent_count; i++) {
        if (areas_match(s_recent[i].body_area, area)) {
            n++;
        }
    }
    return n;
}

static int count_tone(const char *tone)
{
    int n = 0;
    for (int i = 0; i < s_recent_count; i++) {
        if (tones_match(s_recent[i].emotion_label, tone)) {
            n++;
        }
    }
    return n;
}

static int count_regulation_recent(void)
{
    int n = 0;
    for (int i = 0; i < s_recent_count; i++) {
        if (item_is_regulation(&s_recent[i])) {
            n++;
        }
    }
    return n;
}

typedef enum {
    COLOR_FAM_UNKNOWN = 0,
    COLOR_FAM_COOL,
    COLOR_FAM_WARM,
    COLOR_FAM_DARK,
    COLOR_FAM_BRIGHT,
} color_family_t;

static bool hex_to_rgb(const char *hex, float *r, float *g, float *b)
{
    if (!hex || hex[0] != '#') {
        return false;
    }
    unsigned ir = 0;
    unsigned ig = 0;
    unsigned ib = 0;
    if (sscanf(hex + 1, "%2x%2x%2x", &ir, &ig, &ib) != 3) {
        return false;
    }
    *r = (float)ir / 255.0f;
    *g = (float)ig / 255.0f;
    *b = (float)ib / 255.0f;
    return true;
}

static color_family_t color_family_of(const char *hex)
{
    float r, g, b;
    if (!hex_to_rgb(hex, &r, &g, &b)) {
        return COLOR_FAM_UNKNOWN;
    }
    float maxc = r;
    if (g > maxc) {
        maxc = g;
    }
    if (b > maxc) {
        maxc = b;
    }
    float minc = r;
    if (g < minc) {
        minc = g;
    }
    if (b < minc) {
        minc = b;
    }
    float v = maxc;
    float d = maxc - minc;
    float s = maxc > 0.0f ? d / maxc : 0.0f;
    if (v < 0.35f) {
        return COLOR_FAM_DARK;
    }
    if (v > 0.72f && s < 0.35f) {
        return COLOR_FAM_BRIGHT;
    }
    if (s < 0.12f) {
        return COLOR_FAM_UNKNOWN;
    }
    float h = 0.0f;
    if (d > 0.0f) {
        if (maxc == r) {
            h = 60.0f * fmodf(((g - b) / d), 6.0f);
        } else if (maxc == g) {
            h = 60.0f * (((b - r) / d) + 2.0f);
        } else {
            h = 60.0f * (((r - g) / d) + 4.0f);
        }
        if (h < 0.0f) {
            h += 360.0f;
        }
    }
    if (h >= 120.0f && h <= 270.0f) {
        return COLOR_FAM_COOL;
    }
    return COLOR_FAM_WARM;
}

static int count_color_family(color_family_t fam)
{
    int n = 0;
    for (int i = 0; i < s_recent_count; i++) {
        if (s_recent[i].color_skipped || s_recent[i].color_hex[0] != '#') {
            continue;
        }
        if (color_family_of(s_recent[i].color_hex) == fam) {
            n++;
        }
    }
    return n;
}

static bool try_regulation_pattern(const circe_entry_t *entry, circe_reflection_t *out)
{
    if (!s_recent_loaded || s_recent_count < 2) {
        return false;
    }
    if (count_regulation_recent() >= 2) {
        copy_main(out, "This regulation session joins your recent grounding record.");
        copy_sub(out, "We can return to this later.");
        out->is_regulation = true;
        out->is_recent_pattern = true;
        return true;
    }
    (void)entry;
    return false;
}

static bool try_body_patterns(const circe_entry_t *entry, circe_reflection_t *out)
{
    if (!s_recent_loaded || s_recent_count < 2) {
        return false;
    }

    if (entry->intensity >= 8 && count_high_intensity() >= 2) {
        copy_main(out, "Strong signals have appeared more than once recently.");
        copy_sub(out, "Would grounding help?");
        out->suggest_regulate = true;
        out->is_recent_pattern = true;
        return true;
    }

    if (entry->body_area_count > 0 && entry->body_areas[0][0] &&
        count_body_area(entry->body_areas[0]) >= 2) {
        char line[CIRCE_REFLECTION_MAIN_MAX];
        snprintf(line, sizeof(line), "Your %s has appeared more than once recently.", entry->body_areas[0]);
        copy_main(out, line);
        copy_sub(out, "This is only an observation.");
        out->is_recent_pattern = true;
        return true;
    }

    if (!tone_is_unknown(entry) && count_tone(entry->emotion_label) >= 2) {
        char line[CIRCE_REFLECTION_MAIN_MAX];
        snprintf(line, sizeof(line), "%s has shown up before.", entry->emotion_label);
        copy_main(out, line);
        copy_sub(out, "No need to decide what it means yet.");
        out->is_recent_pattern = true;
        return true;
    }

    if (count_regulation_recent() >= 1) {
        copy_main(out, "You have used regulation recently. I can keep that thread.");
        copy_sub(out, "We can return to this later.");
        out->is_recent_pattern = true;
        return true;
    }

    if (!entry->color_skipped && entry->color_hex[0] == '#') {
        color_family_t fam = color_family_of(entry->color_hex);
        if (fam != COLOR_FAM_UNKNOWN && count_color_family(fam) >= 2) {
            if (fam == COLOR_FAM_COOL) {
                copy_main(out, "Your recent colors have been mostly cool.");
            } else if (fam == COLOR_FAM_WARM) {
                copy_main(out, "Your recent colors have stayed near this range.");
            } else {
                copy_main(out, "Your recent colors have stayed near this range.");
            }
            copy_sub(out, "This is only an observation.");
            out->is_recent_pattern = true;
            return true;
        }
    }

    return false;
}

static bool generate_regulation(const circe_entry_t *entry, circe_reflection_t *out)
{
    out->is_regulation = true;
    out->suggest_regulate = false;
    if (entry->regulation_session_completed) {
        copy_main(out, "Session complete. I saved this regulation entry.");
    } else {
        char line[CIRCE_REFLECTION_MAIN_MAX];
        snprintf(line, sizeof(line), "Session saved. You stayed with it for %d seconds.",
                 entry->regulation_duration_seconds > 0 ? entry->regulation_duration_seconds : 0);
        copy_main(out, line);
    }
    copy_sub(out, "We can return to this later.");
    return true;
}

static bool generate_body_reflection(const circe_entry_t *entry, circe_reflection_t *out)
{
    char line[CIRCE_REFLECTION_MAIN_MAX];

    if (entry->body_area_count > 0 && entry->body_areas[0][0]) {
        snprintf(line, sizeof(line), "I noticed your %s carried this entry.", entry->body_areas[0]);
        copy_main(out, line);
    } else if (!tone_is_unknown(entry)) {
        snprintf(line, sizeof(line), "I saved this as %s.", entry->emotion_label);
        copy_main(out, line);
    } else if (!entry->color_skipped && entry->color_hex[0] == '#' &&
               strcmp(entry->color_source, "touch_picker") == 0) {
        snprintf(line, sizeof(line), "I saved the color you chose: %s.", entry->color_hex);
        copy_main(out, line);
    } else if (!entry->color_skipped && entry->color_hex[0] == '#' && strcmp(entry->color_source, "preset") == 0 &&
               entry->color_label[0]) {
        snprintf(line, sizeof(line), "I saved this color as %s %s.", entry->color_label, entry->color_hex);
        copy_main(out, line);
    } else if (entry->intensity >= 8) {
        copy_main(out, "That signal was strong. I saved it.");
    } else if (tone_is_unknown(entry)) {
        copy_main(out, "Not knowing is allowed. I saved what you noticed.");
    } else {
        copy_main(out, "Saved. I can remember this with you.");
    }

    if (entry->intensity >= 8) {
        copy_sub(out, "Would a grounding sequence help?");
        out->suggest_regulate = true;
    } else if (entry->body_area_count > 0 && !tone_is_unknown(entry)) {
        copy_sub(out, "We can return to this later.");
    } else if (!entry->color_skipped && entry->color_hex[0] == '#') {
        copy_sub(out, "We can return to this later.");
    } else {
        copy_sub(out, "");
    }

    return out->main_text[0] != '\0';
}

bool circe_reflection_generate(const circe_entry_t *entry, circe_reflection_t *out)
{
    if (!out) {
        return false;
    }
    circe_reflection_clear(out);
    if (!entry) {
        copy_main(out, "Saved. I can remember this with you.");
        ESP_LOGW(TAG, "reflection fallback: null entry");
        return true;
    }

    bool ok = false;
    if (entry->entry_mode == CIRCE_ENTRY_MODE_REGULATION || entry->has_regulation) {
        if (try_regulation_pattern(entry, out)) {
            ok = true;
        } else {
            ok = generate_regulation(entry, out);
        }
    } else if (try_body_patterns(entry, out)) {
        ok = true;
    } else {
        ok = generate_body_reflection(entry, out);
    }

    if (!ok || out->main_text[0] == '\0') {
        copy_main(out, "Saved. I can remember this with you.");
        ok = true;
    }

    ESP_LOGI(TAG, "reflection%s: %s", out->is_recent_pattern ? " (pattern)" : "", out->main_text);
    return ok;
}
