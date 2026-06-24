#pragma once

#include "circe_entry.h"

typedef struct {
    const char *label;
    const char *area;
    const char *sensation;
    const char *color_hex;
} circe_quick_preset_t;

#define CIRCE_QUICK_PRESET_COUNT 5

extern const circe_quick_preset_t circe_quick_presets[CIRCE_QUICK_PRESET_COUNT];

void circe_entry_modes_apply_quick_preset(circe_entry_t *entry, int preset_index);

extern const char *circe_body_areas[];
extern const int circe_body_area_count;

extern const char *circe_body_sensations[];
extern const int circe_body_sensation_count;

extern const char *circe_quick_colors[];
extern const char *circe_quick_color_labels[];
extern const int circe_quick_color_count;
