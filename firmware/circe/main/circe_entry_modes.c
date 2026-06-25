#include "circe_entry_modes.h"

#include <string.h>

const char *circe_body_areas[] = {
    "head",   "eyes",   "jaw",    "throat", "chest",  "stomach", "shoulders", "back",
    "arms",   "hands",  "hips",   "legs",   "feet",   "skin",    "whole body",
};
const int circe_body_area_count = sizeof(circe_body_areas) / sizeof(circe_body_areas[0]);

const char *circe_body_sensations[] = {
    "tight",            "pressure",         "heavy",           "shaky",           "buzzing",
    "tingling",         "hot",              "cold",              "numb",            "floaty",
    "nauseous",         "racing heart",     "shallow breathing", "skin crawling",   "overstimulated",
    "shutdown feeling", "meltdown warning", "pain spike",
};
const int circe_body_sensation_count = sizeof(circe_body_sensations) / sizeof(circe_body_sensations[0]);

const circe_emotion_tone_option_t circe_emotion_tones[] = {
    {"UNKNOWN", "unknown"},
    {"CALM", "calm"},
    {"FOCUSED", "focused"},
    {"TIRED", "tired"},
    {"STRESSED", "stressed"},
    {"TENSE", "tense"},
    {"OVERWHELMED", "overwhelmed"},
    {"NUMB", "numb"},
    {"SAD", "sad"},
    {"ANGRY", "angry"},
    {"HOPEFUL", "hopeful"},
    {"SAFE", "safe"},
    {"UNSETTLED", "unsettled"},
    {"CUSTOM LATER", "custom_later"},
};
const int circe_emotion_tone_count = sizeof(circe_emotion_tones) / sizeof(circe_emotion_tones[0]);

const circe_color_preset_t circe_color_presets[] = {
    {"CYAN", "#7DF9FF"},
    {"GREEN", "#68D391"},
    {"RED", "#F56565"},
    {"GRAY", "#A0AEC0"},
    {"YELLOW", "#F6E05E"},
    {"PINK", "#F687B3"},
    {"PURPLE", "#B794F4"},
    {"SLATE", "#4A5568"},
};
const int circe_color_preset_count = sizeof(circe_color_presets) / sizeof(circe_color_presets[0]);

const circe_quick_preset_t circe_quick_presets[CIRCE_QUICK_PRESET_COUNT] = {
    {"Chest tight", "chest", "tight", "#4A5568"},
    {"Overstim", "whole body", "overstimulated", "#9B59B6"},
    {"Numb", "whole body", "numb", "#808080"},
    {"Calm green", "chest", "pressure", "#68D391"},
    {"Soft gray", "", "", "#808080"},
};

void circe_entry_modes_apply_tone(circe_entry_t *entry, const char *label, const char *value, bool skipped)
{
    if (!entry) {
        return;
    }
    entry->emotion_skipped = skipped;
    if (skipped || !value || !value[0]) {
        strncpy(entry->emotion, "unknown", sizeof(entry->emotion) - 1);
        strncpy(entry->emotion_label, "UNKNOWN", sizeof(entry->emotion_label) - 1);
        strncpy(entry->emotional_tone, "unknown", sizeof(entry->emotional_tone) - 1);
        strncpy(entry->emotion_family, "unknown", sizeof(entry->emotion_family) - 1);
        return;
    }
    strncpy(entry->emotion, value, sizeof(entry->emotion) - 1);
    entry->emotion[sizeof(entry->emotion) - 1] = '\0';
    if (label && label[0]) {
        strncpy(entry->emotion_label, label, sizeof(entry->emotion_label) - 1);
        entry->emotion_label[sizeof(entry->emotion_label) - 1] = '\0';
        strncpy(entry->emotional_tone, label, sizeof(entry->emotional_tone) - 1);
        entry->emotional_tone[sizeof(entry->emotional_tone) - 1] = '\0';
        strncpy(entry->emotion_family, label, sizeof(entry->emotion_family) - 1);
        entry->emotion_family[sizeof(entry->emotion_family) - 1] = '\0';
    }
}

void circe_entry_modes_apply_color_preset(circe_entry_t *entry, const char *label, const char *hex)
{
    if (!entry || !hex) {
        return;
    }
    entry->color_skipped = false;
    strncpy(entry->color_hex, hex, sizeof(entry->color_hex) - 1);
    entry->color_hex[sizeof(entry->color_hex) - 1] = '\0';
    strncpy(entry->color_label, label ? label : "PRESET", sizeof(entry->color_label) - 1);
    entry->color_label[sizeof(entry->color_label) - 1] = '\0';
    strncpy(entry->color_source, "preset", sizeof(entry->color_source) - 1);
    entry->color_source[sizeof(entry->color_source) - 1] = '\0';
}

void circe_entry_modes_apply_color_touch(circe_entry_t *entry, const char *hex)
{
    if (!entry || !hex) {
        return;
    }
    entry->color_skipped = false;
    strncpy(entry->color_hex, hex, sizeof(entry->color_hex) - 1);
    entry->color_hex[sizeof(entry->color_hex) - 1] = '\0';
    strncpy(entry->color_label, "CUSTOM", sizeof(entry->color_label) - 1);
    entry->color_label[sizeof(entry->color_label) - 1] = '\0';
    strncpy(entry->color_source, "touch_picker", sizeof(entry->color_source) - 1);
    entry->color_source[sizeof(entry->color_source) - 1] = '\0';
}

void circe_entry_modes_apply_color_skipped(circe_entry_t *entry)
{
    if (!entry) {
        return;
    }
    entry->color_skipped = true;
    entry->color_hex[0] = '\0';
    strncpy(entry->color_label, "SKIPPED", sizeof(entry->color_label) - 1);
    entry->color_label[sizeof(entry->color_label) - 1] = '\0';
    strncpy(entry->color_source, "skipped", sizeof(entry->color_source) - 1);
    entry->color_source[sizeof(entry->color_source) - 1] = '\0';
}

void circe_entry_modes_apply_quick_preset(circe_entry_t *entry, int preset_index)
{
    if (preset_index < 0 || preset_index >= CIRCE_QUICK_PRESET_COUNT) {
        return;
    }
    const circe_quick_preset_t *p = &circe_quick_presets[preset_index];
    entry->body_area_count = 0;
    entry->body_sensation_count = 0;
    if (p->area[0]) {
        strncpy(entry->body_areas[0], p->area, sizeof(entry->body_areas[0]) - 1);
        entry->body_area_count = 1;
    }
    if (p->sensation[0]) {
        strncpy(entry->body_sensations[0], p->sensation, sizeof(entry->body_sensations[0]) - 1);
        entry->body_sensation_count = 1;
    }
    if (p->color_hex[0]) {
        circe_entry_modes_apply_color_preset(entry, "QUICK", p->color_hex);
    }
    entry->emotion_skipped = true;
    circe_entry_modes_apply_tone(entry, "UNKNOWN", "unknown", true);
}
