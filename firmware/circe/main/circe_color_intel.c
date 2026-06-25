#include "circe_color_intel.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void copy_label(char *dest, size_t dest_len, const char *src)
{
    if (!dest || dest_len == 0) {
        return;
    }
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_len - 1);
    dest[dest_len - 1] = '\0';
}

bool circe_color_parse_hex(const char *hex, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (!hex || hex[0] != '#' || !r || !g || !b) {
        return false;
    }
    unsigned ir = 0;
    unsigned ig = 0;
    unsigned ib = 0;
    if (sscanf(hex + 1, "%2x%2x%2x", &ir, &ig, &ib) != 3) {
        return false;
    }
    *r = (uint8_t)ir;
    *g = (uint8_t)ig;
    *b = (uint8_t)ib;
    return true;
}

bool circe_color_rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v)
{
    if (!h || !s || !v) {
        return false;
    }
    float rf = (float)r / 255.0f;
    float gf = (float)g / 255.0f;
    float bf = (float)b / 255.0f;

    float maxc = rf;
    if (gf > maxc) {
        maxc = gf;
    }
    if (bf > maxc) {
        maxc = bf;
    }
    float minc = rf;
    if (gf < minc) {
        minc = gf;
    }
    if (bf < minc) {
        minc = bf;
    }

    float delta = maxc - minc;
    *v = maxc;
    *s = maxc > 0.0f ? delta / maxc : 0.0f;
    *h = 0.0f;

    if (delta > 0.0f) {
        if (maxc == rf) {
            *h = 60.0f * fmodf(((gf - bf) / delta), 6.0f);
        } else if (maxc == gf) {
            *h = 60.0f * (((bf - rf) / delta) + 2.0f);
        } else {
            *h = 60.0f * (((rf - gf) / delta) + 4.0f);
        }
        if (*h < 0.0f) {
            *h += 360.0f;
        }
    }
    return true;
}

const char *circe_color_family_from_hue(float h, float s)
{
    if (s < CIRCE_COLOR_GRAY_SAT_THRESHOLD) {
        return "gray";
    }
    if (h < 15.0f || h >= 345.0f) {
        return "red";
    }
    if (h < 45.0f) {
        return "orange";
    }
    if (h < 70.0f) {
        return "yellow";
    }
    if (h < 160.0f) {
        return "green";
    }
    if (h < 200.0f) {
        return "cyan";
    }
    if (h < 260.0f) {
        return "blue";
    }
    if (h < 290.0f) {
        return "purple";
    }
    if (h < 345.0f) {
        return "pink";
    }
    return "unknown";
}

const char *circe_color_temperature_from_hue(float h, float s)
{
    if (s < CIRCE_COLOR_GRAY_SAT_THRESHOLD) {
        return "neutral";
    }
    const char *family = circe_color_family_from_hue(h, s);
    if (strcmp(family, "red") == 0 || strcmp(family, "orange") == 0 || strcmp(family, "yellow") == 0 ||
        strcmp(family, "pink") == 0) {
        return "warm";
    }
    if (strcmp(family, "green") == 0 || strcmp(family, "cyan") == 0 || strcmp(family, "blue") == 0 ||
        strcmp(family, "purple") == 0) {
        return "cool";
    }
    if (strcmp(family, "gray") == 0) {
        return "neutral";
    }
    return "unknown";
}

const char *circe_color_brightness_label(float v)
{
    if (v < 0.25f) {
        return "dark";
    }
    if (v < 0.50f) {
        return "dim";
    }
    if (v < 0.75f) {
        return "balanced";
    }
    return "bright";
}

const char *circe_color_saturation_label(float s)
{
    if (s < 0.25f) {
        return "muted";
    }
    if (s < 0.65f) {
        return "soft";
    }
    return "vivid";
}

bool circe_color_intel_from_hex(const char *hex, circe_color_intel_t *out)
{
    if (!out) {
        return false;
    }
    memset(out, 0, sizeof(*out));

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    if (!circe_color_parse_hex(hex, &r, &g, &b)) {
        return false;
    }
    if (!circe_color_rgb_to_hsv(r, g, b, &out->hue, &out->saturation, &out->value)) {
        return false;
    }

    copy_label(out->family, sizeof(out->family), circe_color_family_from_hue(out->hue, out->saturation));
    copy_label(out->temperature, sizeof(out->temperature),
               circe_color_temperature_from_hue(out->hue, out->saturation));
    copy_label(out->brightness_label, sizeof(out->brightness_label), circe_color_brightness_label(out->value));
    copy_label(out->saturation_label, sizeof(out->saturation_label), circe_color_saturation_label(out->saturation));
    out->valid = true;
    return true;
}

void circe_color_intel_clear_entry(circe_entry_t *entry)
{
    if (!entry) {
        return;
    }
    entry->has_color_intel = false;
    entry->color_hue = 0.0f;
    entry->color_saturation = 0.0f;
    entry->color_value = 0.0f;
    entry->color_family[0] = '\0';
    entry->color_temperature[0] = '\0';
    entry->color_brightness_label[0] = '\0';
    entry->color_saturation_label[0] = '\0';
}

void circe_color_intel_apply_to_entry(circe_entry_t *entry)
{
    if (!entry) {
        return;
    }
    if (entry->color_skipped || entry->color_hex[0] != '#') {
        circe_color_intel_clear_entry(entry);
        return;
    }

    circe_color_intel_t intel = {0};
    if (!circe_color_intel_from_hex(entry->color_hex, &intel)) {
        circe_color_intel_clear_entry(entry);
        return;
    }

    entry->has_color_intel = true;
    entry->color_hue = intel.hue;
    entry->color_saturation = intel.saturation;
    entry->color_value = intel.value;
    copy_label(entry->color_family, sizeof(entry->color_family), intel.family);
    copy_label(entry->color_temperature, sizeof(entry->color_temperature), intel.temperature);
    copy_label(entry->color_brightness_label, sizeof(entry->color_brightness_label), intel.brightness_label);
    copy_label(entry->color_saturation_label, sizeof(entry->color_saturation_label), intel.saturation_label);
}

bool circe_color_intel_from_entry(const circe_entry_t *entry, circe_color_intel_t *out)
{
    if (!entry || !out) {
        return false;
    }
    if (entry->has_color_intel && entry->color_family[0]) {
        memset(out, 0, sizeof(*out));
        out->valid = true;
        out->hue = entry->color_hue;
        out->saturation = entry->color_saturation;
        out->value = entry->color_value;
        copy_label(out->family, sizeof(out->family), entry->color_family);
        copy_label(out->temperature, sizeof(out->temperature), entry->color_temperature);
        copy_label(out->brightness_label, sizeof(out->brightness_label), entry->color_brightness_label);
        copy_label(out->saturation_label, sizeof(out->saturation_label), entry->color_saturation_label);
        return true;
    }
    if (entry->color_skipped || entry->color_hex[0] != '#') {
        return false;
    }
    return circe_color_intel_from_hex(entry->color_hex, out);
}

bool circe_color_intel_from_timeline_item(const circe_timeline_item_t *item, circe_color_intel_t *out)
{
    if (!item || !out) {
        return false;
    }
    if (item->has_color_intel && item->color_family[0]) {
        memset(out, 0, sizeof(*out));
        out->valid = true;
        out->hue = item->color_hue;
        out->saturation = item->color_saturation;
        out->value = item->color_value;
        copy_label(out->family, sizeof(out->family), item->color_family);
        copy_label(out->temperature, sizeof(out->temperature), item->color_temperature);
        copy_label(out->brightness_label, sizeof(out->brightness_label), item->color_brightness_label);
        copy_label(out->saturation_label, sizeof(out->saturation_label), item->color_saturation_label);
        return true;
    }
    if (item->color_skipped || item->color_hex[0] != '#') {
        return false;
    }
    return circe_color_intel_from_hex(item->color_hex, out);
}

void circe_color_intel_format_review_traits(const circe_entry_t *entry, char *buf, size_t len)
{
    if (!buf || len == 0) {
        return;
    }
    circe_color_intel_t intel = {0};
    if (!circe_color_intel_from_entry(entry, &intel)) {
        buf[0] = '\0';
        return;
    }
    snprintf(buf, len, "%s / %s / %s", intel.family, intel.temperature, intel.brightness_label);
}

bool circe_color_intel_format_observation(const circe_entry_t *entry, char *buf, size_t len)
{
    if (!buf || len == 0 || !entry) {
        return false;
    }
    circe_color_intel_t intel = {0};
    if (!circe_color_intel_from_entry(entry, &intel)) {
        return false;
    }

    if (strcmp(intel.saturation_label, "vivid") == 0 && strcmp(intel.family, "gray") != 0 &&
        strcmp(intel.family, "unknown") != 0) {
        snprintf(buf, len, "I saved this as a %s %s.", intel.saturation_label, intel.family);
        return true;
    }
    if (strcmp(intel.temperature, "neutral") == 0) {
        snprintf(buf, len, "This color is %s.", intel.brightness_label);
        return true;
    }
    snprintf(buf, len, "This color is %s and %s.", intel.temperature, intel.brightness_label);
    return true;
}
