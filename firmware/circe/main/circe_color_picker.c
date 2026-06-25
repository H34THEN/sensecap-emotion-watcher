#include "circe_color_picker.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "circe_buf.h"
#include "circe_color_intel.h"
#include "circe_entry_modes.h"
#include "circe_fonts.h"
#include "circe_theme.h"
#include "esp_log.h"

static const char *TAG = "circe_color_picker";

static void hsv_to_rgb(int h, int s, int v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s <= 0) {
        uint8_t c = (uint8_t)((v * 255) / 100);
        *r = *g = *b = c;
        return;
    }
    int region = h / 60;
    int remainder = (h % 60) * 255 / 60;
    int p = (v * (255 - s)) / 100;
    int q = (v * (255 - ((s * remainder) / 255))) / 100;
    int t = (v * (255 - ((s * (255 - remainder)) / 255))) / 100;
    int vv = (v * 255) / 100;
    switch (region) {
    case 0:
        *r = (uint8_t)vv;
        *g = (uint8_t)t;
        *b = (uint8_t)p;
        break;
    case 1:
        *r = (uint8_t)q;
        *g = (uint8_t)vv;
        *b = (uint8_t)p;
        break;
    case 2:
        *r = (uint8_t)p;
        *g = (uint8_t)vv;
        *b = (uint8_t)t;
        break;
    case 3:
        *r = (uint8_t)p;
        *g = (uint8_t)q;
        *b = (uint8_t)vv;
        break;
    case 4:
        *r = (uint8_t)t;
        *g = (uint8_t)p;
        *b = (uint8_t)vv;
        break;
    default:
        *r = (uint8_t)vv;
        *g = (uint8_t)p;
        *b = (uint8_t)q;
        break;
    }
}

static void rgb_to_hex(uint8_t r, uint8_t g, uint8_t b, char *hex, size_t len)
{
    snprintf(hex, len, "#%02X%02X%02X", r, g, b);
}

static void picker_from_hsv(circe_color_picker_t *picker)
{
    uint8_t r, g, b;
    hsv_to_rgb(picker->hue, CIRCE_COLOR_PICKER_SAT, picker->value, &r, &g, &b);
    rgb_to_hex(r, g, b, picker->hex, sizeof(picker->hex));
    picker->color_set = true;
}

static void hex_to_hsv(const char *hex, int *hue, int *value)
{
    if (!hex || hex[0] != '#' || strlen(hex) != 7) {
        *hue = 200;
        *value = 70;
        return;
    }
    unsigned r, g, b;
    sscanf(hex + 1, "%2x%2x%2x", &r, &g, &b);
    int maxc = (int)(r > g ? (r > b ? r : b) : (g > b ? g : b));
    int minc = (int)(r < g ? (r < b ? r : b) : (g < b ? g : b));
    *value = maxc * 100 / 255;
    if (maxc == minc) {
        *hue = 0;
        return;
    }
    int delta = maxc - minc;
    int h;
    if (maxc == (int)r) {
        h = 60 * ((int)g - (int)b) / delta;
    } else if (maxc == (int)g) {
        h = 60 * ((int)b - (int)r) / delta + 120;
    } else {
        h = 60 * ((int)r - (int)g) / delta + 240;
    }
    if (h < 0) {
        h += 360;
    }
    *hue = h;
}

static void style_label(lv_obj_t *lbl)
{
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_PROMPT);
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->text), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
}

static void style_trait_label(lv_obj_t *lbl)
{
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_CAPTION);
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->muted), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
}

static void update_near_label(circe_color_picker_t *picker)
{
    if (!picker) {
        return;
    }
    picker->near_label[0] = '\0';
    for (int i = 0; i < circe_color_preset_count; i++) {
        if (strcasecmp(picker->hex, circe_color_presets[i].hex) == 0) {
            snprintf(picker->near_label, sizeof(picker->near_label), "near %s", circe_color_presets[i].label);
            return;
        }
    }
}

static void update_trait_label(circe_color_picker_t *picker)
{
    if (!picker || !picker->trait_label) {
        return;
    }
    circe_color_intel_t intel = {0};
    if (picker->hex[0] == '#' && circe_color_intel_from_hex(picker->hex, &intel)) {
        char line[40];
        snprintf(line, sizeof(line), "%s %s %s", intel.family, intel.temperature, intel.brightness_label);
        lv_label_set_text(picker->trait_label, line);
    } else {
        lv_label_set_text(picker->trait_label, "");
    }
}

static void update_crosshair(circe_color_picker_t *picker)
{
    if (!picker || !picker->cross_h || !picker->cross_v) {
        return;
    }
    lv_coord_t cx = (lv_coord_t)(picker->hue * (CIRCE_COLOR_PICKER_FIELD_W - 1) / 359);
    lv_coord_t cy = (lv_coord_t)((100 - picker->value) * (CIRCE_COLOR_PICKER_FIELD_H - 1) / 100);
    lv_obj_set_pos(picker->cross_h, 4, cy);
    lv_obj_set_pos(picker->cross_v, cx, 4);
}

static void touch_to_hsv(circe_color_picker_t *picker, lv_coord_t x, lv_coord_t y)
{
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x >= CIRCE_COLOR_PICKER_FIELD_W) {
        x = CIRCE_COLOR_PICKER_FIELD_W - 1;
    }
    if (y >= CIRCE_COLOR_PICKER_FIELD_H) {
        y = CIRCE_COLOR_PICKER_FIELD_H - 1;
    }
    picker->hue = (int)(x * 359 / (CIRCE_COLOR_PICKER_FIELD_W - 1));
    picker->value = 100 - (int)(y * 100 / (CIRCE_COLOR_PICKER_FIELD_H - 1));
    if (picker->value < 5) {
        picker->value = 5;
    }
    picker_from_hsv(picker);
}

static void update_magnifier(circe_color_picker_t *picker, lv_coord_t x, lv_coord_t y)
{
    if (!picker->magnifier) {
        return;
    }
    uint8_t r, g, b;
    hsv_to_rgb(picker->hue, CIRCE_COLOR_PICKER_SAT, picker->value, &r, &g, &b);
    lv_obj_set_style_bg_color(picker->magnifier, lv_color_make(r, g, b), 0);
    lv_obj_set_style_bg_opa(picker->magnifier, LV_OPA_COVER, 0);

    lv_coord_t mx = x - CIRCE_COLOR_PICKER_MAG_SIZE / 2;
    lv_coord_t my = y - CIRCE_COLOR_PICKER_MAG_SIZE / 2;
    if (mx < 0) {
        mx = 0;
    }
    if (my < 0) {
        my = 0;
    }
    if (mx > CIRCE_COLOR_PICKER_FIELD_W - CIRCE_COLOR_PICKER_MAG_SIZE) {
        mx = CIRCE_COLOR_PICKER_FIELD_W - CIRCE_COLOR_PICKER_MAG_SIZE;
    }
    if (my > CIRCE_COLOR_PICKER_FIELD_H - CIRCE_COLOR_PICKER_MAG_SIZE) {
        my = CIRCE_COLOR_PICKER_FIELD_H - CIRCE_COLOR_PICKER_MAG_SIZE;
    }
    lv_obj_set_pos(picker->magnifier, mx, my);
    lv_obj_clear_flag(picker->magnifier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(picker->magnifier);
}

static void fill_gradient_canvas(lv_obj_t *canvas)
{
    for (lv_coord_t y = 0; y < CIRCE_COLOR_PICKER_CANVAS_H; y++) {
        int value = 100 - (int)(y * 100 / (CIRCE_COLOR_PICKER_CANVAS_H - 1));
        if (value < 5) {
            value = 5;
        }
        for (lv_coord_t x = 0; x < CIRCE_COLOR_PICKER_CANVAS_W; x++) {
            int hue = (int)(x * 359 / (CIRCE_COLOR_PICKER_CANVAS_W - 1));
            uint8_t r, g, b;
            hsv_to_rgb(hue, CIRCE_COLOR_PICKER_SAT, value, &r, &g, &b);
            lv_canvas_set_px_color(canvas, x, y, lv_color_make(r, g, b));
        }
    }
}

static void field_event_cb(lv_event_t *e)
{
    circe_color_picker_t *picker = lv_event_get_user_data(e);
    if (!picker || !picker->field) {
        return;
    }
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING) {
        if (code == LV_EVENT_RELEASED && picker->magnifier) {
            lv_obj_add_flag(picker->magnifier, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) {
        return;
    }
    lv_point_t pt;
    lv_indev_get_point(indev, &pt);
    lv_area_t a;
    lv_obj_get_coords(picker->field, &a);
    lv_coord_t x = pt.x - a.x1;
    lv_coord_t y = pt.y - a.y1;
    touch_to_hsv(picker, x, y);
    circe_color_picker_refresh(picker);
    update_magnifier(picker, x, y);
    update_crosshair(picker);
    lv_obj_move_foreground(picker->cross_h);
    lv_obj_move_foreground(picker->cross_v);
}

void circe_color_picker_set_hex(circe_color_picker_t *picker, const char *hex)
{
    if (!picker) {
        return;
    }
    hex_to_hsv(hex, &picker->hue, &picker->value);
    if (hex && hex[0] == '#') {
        strncpy(picker->hex, hex, sizeof(picker->hex) - 1);
        picker->hex[sizeof(picker->hex) - 1] = '\0';
        picker->color_set = true;
    } else {
        picker_from_hsv(picker);
    }
    update_crosshair(picker);
}

void circe_color_picker_create(circe_color_picker_t *picker, lv_obj_t *parent)
{
    if (!picker || !parent) {
        return;
    }
    memset(picker, 0, sizeof(*picker));
    picker->hue = 260;
    picker->value = 65;
    picker->active = true;
    picker_from_hsv(picker);

    const circe_theme_palette_t *p = circe_theme_get_palette();

    size_t buf_pixels = LV_CANVAS_BUF_SIZE_TRUE_COLOR(CIRCE_COLOR_PICKER_CANVAS_W, CIRCE_COLOR_PICKER_CANVAS_H);
    picker->canvas_buf = circe_buf_alloc(buf_pixels * sizeof(lv_color_t));
    if (!picker->canvas_buf) {
        ESP_LOGE(TAG, "canvas buffer alloc failed");
        return;
    }

    picker->root = lv_obj_create(parent);
    lv_obj_set_size(picker->root, CIRCE_COLOR_PICKER_FIELD_W + CIRCE_UI_COLOR_ROOT_PAD_W,
                    CIRCE_COLOR_PICKER_TRAIT_Y_OFS + 28);
    lv_obj_align(picker->root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(picker->root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(picker->root, 0, 0);
    lv_obj_set_style_pad_all(picker->root, 0, 0);
    lv_obj_clear_flag(picker->root, LV_OBJ_FLAG_SCROLLABLE);

    picker->field = lv_canvas_create(picker->root);
    lv_canvas_set_buffer(picker->field, picker->canvas_buf, CIRCE_COLOR_PICKER_CANVAS_W, CIRCE_COLOR_PICKER_CANVAS_H,
                         LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(picker->field, CIRCE_COLOR_PICKER_FIELD_W, CIRCE_COLOR_PICKER_FIELD_H);
    lv_obj_align(picker->field, LV_ALIGN_TOP_MID, 0, 0);
    lv_img_set_antialias(picker->field, true);
    fill_gradient_canvas(picker->field);
    lv_obj_set_style_border_width(picker->field, 1, 0);
    lv_obj_set_style_border_color(picker->field, circe_theme_color(p->focus), 0);
    lv_obj_set_style_radius(picker->field, 12, 0);
    lv_obj_set_style_pad_all(picker->field, 0, 0);
    lv_obj_add_flag(picker->field, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(picker->field, field_event_cb, LV_EVENT_ALL, picker);

    picker->cross_h = lv_obj_create(picker->field);
    lv_obj_set_size(picker->cross_h, CIRCE_COLOR_PICKER_FIELD_W - CIRCE_UI_COLOR_CROSS_INSET, 1);
    lv_obj_set_style_bg_color(picker->cross_h, circe_theme_color(p->focus), 0);
    lv_obj_set_style_bg_opa(picker->cross_h, LV_OPA_60, 0);
    lv_obj_clear_flag(picker->cross_h, LV_OBJ_FLAG_CLICKABLE);

    picker->cross_v = lv_obj_create(picker->field);
    lv_obj_set_size(picker->cross_v, 1, CIRCE_COLOR_PICKER_FIELD_H - CIRCE_UI_COLOR_CROSS_INSET);
    lv_obj_set_style_bg_color(picker->cross_v, circe_theme_color(p->focus), 0);
    lv_obj_set_style_bg_opa(picker->cross_v, LV_OPA_60, 0);
    lv_obj_clear_flag(picker->cross_v, LV_OBJ_FLAG_CLICKABLE);

    picker->magnifier = lv_obj_create(picker->field);
    lv_obj_set_size(picker->magnifier, CIRCE_COLOR_PICKER_MAG_SIZE, CIRCE_COLOR_PICKER_MAG_SIZE);
    lv_obj_set_style_radius(picker->magnifier, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(picker->magnifier, 2, 0);
    lv_obj_set_style_border_color(picker->magnifier, circe_theme_color(p->focus), 0);
    lv_obj_add_flag(picker->magnifier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(picker->magnifier, LV_OBJ_FLAG_CLICKABLE);

    picker->preview = lv_obj_create(picker->root);
    lv_obj_set_size(picker->preview, CIRCE_UI_COLOR_PREVIEW_SIZE, CIRCE_UI_COLOR_PREVIEW_SIZE);
    lv_obj_align(picker->preview, LV_ALIGN_TOP_LEFT, CIRCE_UI_COLOR_PREVIEW_X, CIRCE_COLOR_PICKER_HEX_Y_OFS);
    lv_obj_set_style_radius(picker->preview, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(picker->preview, 1, 0);
    lv_obj_set_style_border_color(picker->preview, circe_theme_color(p->muted), 0);
    lv_obj_clear_flag(picker->preview, LV_OBJ_FLAG_CLICKABLE);

    picker->hex_label = lv_label_create(picker->root);
    lv_obj_set_width(picker->hex_label, CIRCE_COLOR_PICKER_FIELD_W);
    lv_obj_align(picker->hex_label, LV_ALIGN_TOP_LEFT, CIRCE_UI_COLOR_HEX_LABEL_X, CIRCE_COLOR_PICKER_HEX_Y_OFS + 2);
    style_label(picker->hex_label);

    picker->trait_label = lv_label_create(picker->root);
    lv_obj_set_width(picker->trait_label, CIRCE_COLOR_PICKER_FIELD_W);
    lv_obj_align(picker->trait_label, LV_ALIGN_TOP_LEFT, CIRCE_UI_COLOR_HEX_LABEL_X, CIRCE_COLOR_PICKER_TRAIT_Y_OFS);
    style_trait_label(picker->trait_label);

    update_crosshair(picker);
    lv_obj_move_foreground(picker->cross_h);
    lv_obj_move_foreground(picker->cross_v);
    circe_color_picker_refresh(picker);

    ESP_LOGI(TAG, "created canvas picker %dx%d -> %dx%d, %u bytes, %d lvgl objects",
             CIRCE_COLOR_PICKER_CANVAS_W, CIRCE_COLOR_PICKER_CANVAS_H, CIRCE_COLOR_PICKER_FIELD_W,
             CIRCE_COLOR_PICKER_FIELD_H, (unsigned)(buf_pixels * sizeof(lv_color_t)), CIRCE_COLOR_PICKER_LVGL_OBJS);
}

void circe_color_picker_destroy(circe_color_picker_t *picker)
{
    if (!picker) {
        return;
    }
    if (picker->root) {
        lv_obj_del(picker->root);
    }
    if (picker->canvas_buf) {
        circe_buf_free(picker->canvas_buf);
    }
    picker->root = NULL;
    picker->field = NULL;
    picker->hex_label = NULL;
    picker->trait_label = NULL;
    picker->preview = NULL;
    picker->magnifier = NULL;
    picker->cross_h = NULL;
    picker->cross_v = NULL;
    picker->canvas_buf = NULL;
    picker->active = false;
}

void circe_color_picker_refresh(circe_color_picker_t *picker)
{
    if (!picker) {
        return;
    }
    update_near_label(picker);
    char line[48];
    if (picker->near_label[0]) {
        snprintf(line, sizeof(line), "%s  %s", picker->hex, picker->near_label);
    } else {
        snprintf(line, sizeof(line), "%s", picker->hex);
    }
    if (picker->hex_label) {
        lv_label_set_text(picker->hex_label, line);
    }
    if (picker->preview && picker->hex[0] == '#') {
        unsigned r, g, b;
        sscanf(picker->hex + 1, "%2x%2x%2x", &r, &g, &b);
        lv_obj_set_style_bg_color(picker->preview, lv_color_make((uint8_t)r, (uint8_t)g, (uint8_t)b), 0);
        lv_obj_set_style_bg_opa(picker->preview, LV_OPA_COVER, 0);
    }
    update_trait_label(picker);
    update_crosshair(picker);
}

void circe_color_picker_apply_to_draft(circe_color_picker_t *picker, char *hex_out, size_t hex_len, char *label_out,
                                       size_t label_len, char *source_out, size_t source_len)
{
    if (!picker || !hex_out) {
        return;
    }
    strncpy(hex_out, picker->hex, hex_len - 1);
    hex_out[hex_len - 1] = '\0';
    if (label_out && label_len > 0) {
        strncpy(label_out, "CUSTOM", label_len - 1);
        label_out[label_len - 1] = '\0';
    }
    if (source_out && source_len > 0) {
        strncpy(source_out, "touch_picker", source_len - 1);
        source_out[source_len - 1] = '\0';
    }
}

static lv_indev_t *find_encoder_indev(void)
{
    lv_indev_t *indev = NULL;
    while ((indev = lv_indev_get_next(indev)) != NULL) {
        if (indev->driver->type == LV_INDEV_TYPE_ENCODER) {
            return indev;
        }
    }
    return NULL;
}

bool circe_color_picker_poll_encoder(circe_color_picker_t *picker)
{
    if (!picker || !picker->active) {
        return false;
    }
    lv_indev_t *enc = find_encoder_indev();
    if (!enc || !enc->driver || !enc->driver->read_cb) {
        return false;
    }
    lv_indev_data_t data;
    lv_memset_00(&data, sizeof(data));
    enc->driver->read_cb(enc->driver, &data);
    if (data.state == LV_INDEV_STATE_PRESSED && !picker->enc_btn_last) {
        picker->enc_tune_value = !picker->enc_tune_value;
    }
    picker->enc_btn_last = (data.state == LV_INDEV_STATE_PRESSED);
    if (data.enc_diff == 0) {
        return picker->enc_btn_last;
    }
    if (picker->enc_tune_value) {
        picker->value += data.enc_diff * 2;
        if (picker->value < 5) {
            picker->value = 5;
        }
        if (picker->value > 100) {
            picker->value = 100;
        }
    } else {
        picker->hue += data.enc_diff * 3;
        if (picker->hue < 0) {
            picker->hue += 360;
        }
        if (picker->hue >= 360) {
            picker->hue -= 360;
        }
    }
    picker_from_hsv(picker);
    circe_color_picker_refresh(picker);
    return true;
}
