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

const char *circe_quick_colors[] = {"#808080", "#4A5568", "#68D391", "#9B59B6"};
const int circe_quick_color_count = 4;

const circe_quick_preset_t circe_quick_presets[CIRCE_QUICK_PRESET_COUNT] = {
    {"Chest tight", "chest", "tight", "#4A5568"},
    {"Overstim", "whole body", "overstimulated", "#9B59B6"},
    {"Numb", "whole body", "numb", "#808080"},
    {"Calm green", "chest", "pressure", "#68D391"},
};

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
        strncpy(entry->color_hex, p->color_hex, sizeof(entry->color_hex) - 1);
    }
}
