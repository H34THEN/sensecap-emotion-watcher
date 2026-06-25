#pragma once

#include "circe_entry.h"

typedef struct {
    const char *label;
    const char *area;
    const char *sensation;
    const char *color_hex;
} circe_quick_preset_t;

typedef struct {
    const char *label;
    const char *value;
} circe_emotion_tone_option_t;

typedef struct {
    const char *label;
    const char *hex;
} circe_color_preset_t;

#define CIRCE_QUICK_PRESET_COUNT 5

extern const circe_quick_preset_t circe_quick_presets[CIRCE_QUICK_PRESET_COUNT];

extern const char *circe_body_areas[];
extern const int circe_body_area_count;

extern const char *circe_body_sensations[];
extern const int circe_body_sensation_count;

extern const circe_emotion_tone_option_t circe_emotion_tones[];
extern const int circe_emotion_tone_count;

extern const circe_color_preset_t circe_color_presets[];
extern const int circe_color_preset_count;

void circe_entry_modes_apply_tone(circe_entry_t *entry, const char *label, const char *value, bool skipped);
void circe_entry_modes_apply_color_preset(circe_entry_t *entry, const char *label, const char *hex);
void circe_entry_modes_apply_color_touch(circe_entry_t *entry, const char *hex);
void circe_entry_modes_apply_color_skipped(circe_entry_t *entry);
void circe_entry_modes_apply_quick_preset(circe_entry_t *entry, int preset_index);
