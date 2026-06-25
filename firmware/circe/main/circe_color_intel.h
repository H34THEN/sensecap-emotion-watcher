#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "circe_entry.h"
#include "circe_timeline.h"

#define CIRCE_COLOR_GRAY_SAT_THRESHOLD 0.15f

typedef struct {
    bool valid;
    float hue;
    float saturation;
    float value;
    char family[16];
    char temperature[12];
    char brightness_label[12];
    char saturation_label[12];
} circe_color_intel_t;

bool circe_color_parse_hex(const char *hex, uint8_t *r, uint8_t *g, uint8_t *b);
bool circe_color_rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v);
const char *circe_color_family_from_hue(float h, float s);
const char *circe_color_temperature_from_hue(float h, float s);
const char *circe_color_brightness_label(float v);
const char *circe_color_saturation_label(float s);

bool circe_color_intel_from_hex(const char *hex, circe_color_intel_t *out);
void circe_color_intel_apply_to_entry(circe_entry_t *entry);
void circe_color_intel_clear_entry(circe_entry_t *entry);
bool circe_color_intel_from_entry(const circe_entry_t *entry, circe_color_intel_t *out);
bool circe_color_intel_from_timeline_item(const circe_timeline_item_t *item, circe_color_intel_t *out);

void circe_color_intel_format_review_traits(const circe_entry_t *entry, char *buf, size_t len);
bool circe_color_intel_format_observation(const circe_entry_t *entry, char *buf, size_t len);
