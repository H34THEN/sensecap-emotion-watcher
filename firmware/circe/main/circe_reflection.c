#include "circe_reflection.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "esp_log.h"

static const char *TAG = "circe_reflection";

void circe_reflection_clear(circe_reflection_t *reflection)
{
    if (!reflection) {
        return;
    }
    memset(reflection, 0, sizeof(*reflection));
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
        ok = generate_regulation(entry, out);
    } else {
        ok = generate_body_reflection(entry, out);
    }

    if (!ok || out->main_text[0] == '\0') {
        copy_main(out, "Saved. I can remember this with you.");
        ok = true;
    }

    ESP_LOGI(TAG, "reflection: %s", out->main_text);
    return ok;
}
