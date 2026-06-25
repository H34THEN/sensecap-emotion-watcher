#include "circe_time_picker.h"

#include <stdio.h>
#include <string.h>

#include "circe_fonts.h"
#include "circe_hud.h"
#include "circe_theme.h"
#include "circe_time.h"

#define DOUBLE_PRESS_MS 450
#define LONG_PRESS_MS   800
#define ROW_H           34

static const char *s_field_names[] = {"YEAR", "MONTH", "DAY", "HOUR", "MIN"};

static bool is_leap(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int year, int month)
{
    static const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
        return 31;
    }
    int d = mdays[month - 1];
    if (month == 2 && is_leap(year)) {
        d = 29;
    }
    return d;
}

static void clamp_day(circe_time_picker_t *picker)
{
    int max_day = days_in_month(picker->year, picker->month);
    if (picker->day > max_day) {
        picker->day = max_day;
    }
    if (picker->day < 1) {
        picker->day = 1;
    }
}

static int wrap_inc(int value, int min_v, int max_v)
{
    if (value >= max_v) {
        return min_v;
    }
    return value + 1;
}

static int wrap_dec(int value, int min_v, int max_v)
{
    if (value <= min_v) {
        return max_v;
    }
    return value - 1;
}

static void adjust_field(circe_time_picker_t *picker, int delta)
{
    if (!picker || delta == 0) {
        return;
    }
    int step = delta > 0 ? 1 : -1;
    switch (picker->field) {
    case 0:
        if (step > 0) {
            picker->year = wrap_inc(picker->year, 2020, 2037);
        } else {
            picker->year = wrap_dec(picker->year, 2020, 2037);
        }
        clamp_day(picker);
        break;
    case 1:
        if (step > 0) {
            picker->month = wrap_inc(picker->month, 1, 12);
        } else {
            picker->month = wrap_dec(picker->month, 1, 12);
        }
        clamp_day(picker);
        break;
    case 2: {
        int max_day = days_in_month(picker->year, picker->month);
        if (step > 0) {
            picker->day = wrap_inc(picker->day, 1, max_day);
        } else {
            picker->day = wrap_dec(picker->day, 1, max_day);
        }
        break;
    }
    case 3:
        if (step > 0) {
            picker->hour = wrap_inc(picker->hour, 0, 23);
        } else {
            picker->hour = wrap_dec(picker->hour, 0, 23);
        }
        break;
    case 4:
        if (step > 0) {
            picker->minute = wrap_inc(picker->minute, 0, 59);
        } else {
            picker->minute = wrap_dec(picker->minute, 0, 59);
        }
        break;
    default:
        break;
    }
}

static void advance_field(circe_time_picker_t *picker)
{
    picker->field++;
    if (picker->field >= 5) {
        picker->field = 0;
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

static int32_t read_encoder_diff(lv_indev_t *indev)
{
    if (!indev || !indev->driver || !indev->driver->read_cb) {
        return 0;
    }
    lv_indev_data_t data;
    lv_memset_00(&data, sizeof(data));
    indev->driver->read_cb(indev->driver, &data);
    return data.enc_diff;
}

static lv_indev_state_t read_encoder_state(lv_indev_t *indev)
{
    if (!indev || !indev->driver || !indev->driver->read_cb) {
        return LV_INDEV_STATE_RELEASED;
    }
    lv_indev_data_t data;
    lv_memset_00(&data, sizeof(data));
    indev->driver->read_cb(indev->driver, &data);
    return data.state;
}

static void style_row_label(lv_obj_t *lbl, bool selected)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(selected ? p->text : p->muted), 0);
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_PROMPT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
}

static void format_row(char *buf, size_t len, int field_idx, int selected_field, int year, int month, int day, int hour,
                       int minute)
{
    const char *prefix = (field_idx == selected_field) ? "> " : "  ";
    const char *cursor = (field_idx == selected_field) ? " _" : "";
    switch (field_idx) {
    case 0:
        snprintf(buf, len, "%s%-5s %04d%s", prefix, s_field_names[0], year, cursor);
        break;
    case 1:
        snprintf(buf, len, "%s%-5s %02d%s", prefix, s_field_names[1], month, cursor);
        break;
    case 2:
        snprintf(buf, len, "%s%-5s %02d%s", prefix, s_field_names[2], day, cursor);
        break;
    case 3:
        snprintf(buf, len, "%s%-5s %02d%s", prefix, s_field_names[3], hour, cursor);
        break;
    case 4:
        snprintf(buf, len, "%s%-5s %02d%s", prefix, s_field_names[4], minute, cursor);
        break;
    default:
        buf[0] = '\0';
        break;
    }
}

void circe_time_picker_init_values(circe_time_picker_t *picker)
{
    if (!picker) {
        return;
    }
    if (circe_time_is_set()) {
        circe_time_get_local(&picker->year, &picker->month, &picker->day, &picker->hour, &picker->minute);
    } else {
        picker->year = 2026;
        picker->month = 1;
        picker->day = 1;
        picker->hour = 12;
        picker->minute = 0;
    }
    picker->field = 0;
    clamp_day(picker);
}

void circe_time_picker_create(circe_time_picker_t *picker, lv_obj_t *parent)
{
    if (!picker || !parent) {
        return;
    }
    memset(picker, 0, sizeof(*picker));
    circe_time_picker_init_values(picker);
    picker->active = true;
    picker->field = 0;
    picker->enc = find_encoder_indev();
    picker->enc_was_enabled = picker->enc && !picker->enc->proc.disabled;
    if (picker->enc) {
        lv_indev_set_group(picker->enc, NULL);
        lv_indev_enable(picker->enc, false);
    }

    for (int i = 0; i < 5; i++) {
        picker->rows[i] = lv_label_create(parent);
        lv_obj_set_width(picker->rows[i], 236);
        lv_obj_align(picker->rows[i], LV_ALIGN_TOP_MID, 0, i * ROW_H);
        style_row_label(picker->rows[i], false);
    }
    circe_time_picker_refresh(picker);
}

void circe_time_picker_destroy(circe_time_picker_t *picker)
{
    if (!picker) {
        return;
    }
    if (picker->enc && picker->enc_was_enabled) {
        lv_indev_enable(picker->enc, true);
    }
    picker->enc = NULL;
    for (int i = 0; i < 5; i++) {
        if (picker->rows[i]) {
            lv_obj_del(picker->rows[i]);
            picker->rows[i] = NULL;
        }
    }
    picker->active = false;
}

void circe_time_picker_refresh(circe_time_picker_t *picker)
{
    if (!picker) {
        return;
    }
    char line[40];
    for (int i = 0; i < 5; i++) {
        if (!picker->rows[i]) {
            continue;
        }
        format_row(line, sizeof(line), i, picker->field, picker->year, picker->month, picker->day, picker->hour,
                   picker->minute);
        lv_label_set_text(picker->rows[i], line);
        style_row_label(picker->rows[i], i == picker->field);
    }
}

static bool save_picker_time(circe_time_picker_t *picker)
{
    clamp_day(picker);
    return circe_time_apply(picker->year, picker->month, picker->day, picker->hour, picker->minute);
}

void circe_time_picker_poll(circe_time_picker_t *picker, circe_time_picker_cb_t on_done, void *ctx)
{
    if (!picker || !picker->active) {
        return;
    }

    lv_indev_t *enc = picker->enc ? picker->enc : find_encoder_indev();
    int32_t diff = read_encoder_diff(enc);
    if (diff != 0) {
        adjust_field(picker, diff > 0 ? 1 : -1);
        circe_time_picker_refresh(picker);
    }

    bool pressed = read_encoder_state(enc) == LV_INDEV_STATE_PRESSED;
    uint32_t now = lv_tick_get();

    if (pressed && !picker->enc_pressed) {
        picker->enc_pressed = true;
        picker->press_start_ms = now;
        picker->long_fired = false;
    } else if (!pressed && picker->enc_pressed) {
        picker->enc_pressed = false;
        if (picker->long_fired) {
            picker->last_release_ms = 0;
        } else if (picker->last_release_ms != 0 && (now - picker->last_release_ms) < DOUBLE_PRESS_MS) {
            picker->last_release_ms = 0;
            if (on_done) {
                on_done(false, ctx);
            }
        } else {
            advance_field(picker);
            circe_time_picker_refresh(picker);
            picker->last_release_ms = now;
        }
    } else if (pressed && picker->enc_pressed && !picker->long_fired && (now - picker->press_start_ms) >= LONG_PRESS_MS) {
        picker->long_fired = true;
        if (save_picker_time(picker)) {
            if (on_done) {
                on_done(true, ctx);
            }
        } else if (ctx) {
            circe_hud_t *hud = (circe_hud_t *)ctx;
            circe_hud_set_subline(hud, "Time save failed.");
        }
    }
}
