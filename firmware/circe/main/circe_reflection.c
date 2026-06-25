#include "circe_reflection.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "circe_color_intel.h"
#include "circe_copy.h"
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
    int count = 0;
    if (!circe_timeline_load_pattern_context(s_recent, CIRCE_PATTERN_CONTEXT_MAX, &count) || count <= 0) {
        circe_reflection_clear_recent_context();
        ESP_LOGW(TAG, "recent context unavailable");
        return false;
    }
    s_recent_count = count;
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

static int count_color_trait(const char *trait, const char *value)
{
    int n = 0;
    circe_color_intel_t intel;
    for (int i = 0; i < s_recent_count; i++) {
        if (s_recent[i].color_skipped || s_recent[i].color_hex[0] != '#') {
            continue;
        }
        if (!circe_color_intel_from_timeline_item(&s_recent[i], &intel)) {
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
        circe_color_intel_t intel = {0};
        if (circe_color_intel_from_entry(entry, &intel)) {
            if (count_color_trait("saturation_label", "muted") >= 2 &&
                strcmp(intel.saturation_label, "muted") == 0) {
                copy_main(out, "Your recent colors have stayed mostly muted.");
                copy_sub(out, "This is only an observation.");
                out->is_recent_pattern = true;
                return true;
            }
            if (count_color_trait("temperature", intel.temperature) >= 2 &&
                strcmp(intel.temperature, "cool") == 0) {
                copy_main(out, "Your recent colors have been mostly cool.");
                copy_sub(out, "This is only an observation.");
                out->is_recent_pattern = true;
                return true;
            }
            if (count_color_trait("temperature", intel.temperature) >= 2 &&
                strcmp(intel.temperature, "warm") == 0) {
                copy_main(out, "Your recent colors have stayed near this range.");
                copy_sub(out, "This is only an observation.");
                out->is_recent_pattern = true;
                return true;
            }
        }
    }

    return false;
}

static bool generate_regulation(const circe_entry_t *entry, circe_reflection_t *out)
{
    out->is_regulation = true;
    out->suggest_regulate = false;
    char line[CIRCE_REFLECTION_MAIN_MAX];
    if (strcmp(entry->regulation_type, "grounding_54321") == 0) {
        copy_main(out, "Session saved. You stayed with the 5-4-3-2-1 sequence.");
    } else if (strcmp(entry->regulation_type, "sensory_reset") == 0) {
        copy_main(out, "Session saved. You stayed with sensory reset.");
    } else if (strcmp(entry->regulation_type, "bilateral_tap") == 0) {
        snprintf(line, sizeof(line), "Session saved. You stayed with bilateral tap for %d seconds.",
                 entry->regulation_duration_seconds > 0 ? entry->regulation_duration_seconds : 0);
        copy_main(out, line);
    } else if (entry->regulation_session_completed) {
        copy_main(out, "Session complete. I saved this regulation entry.");
    } else {
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
    } else if (circe_color_intel_format_observation(entry, line, sizeof(line))) {
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
    } else if (entry->has_color_intel || (!entry->color_skipped && entry->color_hex[0] == '#')) {
        copy_sub(out, circe_copy_get(CIRCE_PATTERN_COLOR_INTEL_ONLY_YOU));
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
