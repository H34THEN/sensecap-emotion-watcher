#include "circe_home_wheel.h"

#include <stdio.h>

#include "circe_fonts.h"
#include "circe_theme.h"
#include "esp_log.h"

#define HOME_DOUBLE_PRESS_MS 450
#define HOME_LONG_PRESS_MS   800

static const char *TAG = "circe_home_wheel";

static const char *s_home_labels[CIRCE_HOME_WHEEL_COUNT] = {
    "BODY CHECK-IN",
    "QUICK NOTE",
    "REGULATE",
    "REVIEW",
    "SETTINGS",
};

static const char *s_home_actions[CIRCE_HOME_WHEEL_COUNT] = {
    "ready_body",
    "quick",
    "regulate",
    "review",
    "more",
};

static int wrap_index(int idx)
{
    while (idx < 0) {
        idx += CIRCE_HOME_WHEEL_COUNT;
    }
    while (idx >= CIRCE_HOME_WHEEL_COUNT) {
        idx -= CIRCE_HOME_WHEEL_COUNT;
    }
    return idx;
}

const char *circe_home_wheel_label(int index)
{
    if (index < 0 || index >= CIRCE_HOME_WHEEL_COUNT) {
        return "";
    }
    return s_home_labels[index];
}

const char *circe_home_wheel_action_id(int index)
{
    if (index < 0 || index >= CIRCE_HOME_WHEEL_COUNT) {
        return "";
    }
    return s_home_actions[index];
}

static void style_center_label(lv_obj_t *lbl, circe_font_role_t role, lv_opa_t opa)
{
    circe_fonts_apply_label(lbl, role);
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->text), 0);
    lv_obj_set_style_text_opa(lbl, opa, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
}

static void update_labels(circe_home_wheel_t *wheel)
{
    if (!wheel || !wheel->active) {
        return;
    }
    int prev = wrap_index(wheel->selected - 1);
    int next = wrap_index(wheel->selected + 1);
    if (wheel->prev_lbl) {
        lv_label_set_text(wheel->prev_lbl, s_home_labels[prev]);
    }
    if (wheel->current_lbl) {
        lv_label_set_text(wheel->current_lbl, s_home_labels[wheel->selected]);
    }
    if (wheel->next_lbl) {
        lv_label_set_text(wheel->next_lbl, s_home_labels[next]);
    }
    if (wheel->index_lbl) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d", wheel->selected + 1, CIRCE_HOME_WHEEL_COUNT);
        lv_label_set_text(wheel->index_lbl, buf);
    }
}

void circe_home_wheel_create(circe_home_wheel_t *wheel, lv_obj_t *parent, int initial_index)
{
    if (!wheel || !parent) {
        return;
    }
    circe_home_wheel_destroy(wheel);
    wheel->selected = wrap_index(initial_index);
    wheel->active = true;

    wheel->root = lv_obj_create(parent);
    lv_obj_set_size(wheel->root, 280, 148);
    lv_obj_align(wheel->root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(wheel->root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wheel->root, 0, 0);
    lv_obj_set_style_pad_all(wheel->root, 0, 0);
    lv_obj_clear_flag(wheel->root, LV_OBJ_FLAG_SCROLLABLE);

    wheel->prev_lbl = lv_label_create(wheel->root);
    lv_obj_set_width(wheel->prev_lbl, 260);
    lv_obj_align(wheel->prev_lbl, LV_ALIGN_TOP_MID, 0, 4);
    style_center_label(wheel->prev_lbl, CIRCE_FONT_ROLE_CAPTION, LV_OPA_40);

    wheel->current_lbl = lv_label_create(wheel->root);
    lv_obj_set_width(wheel->current_lbl, 260);
    lv_obj_align(wheel->current_lbl, LV_ALIGN_TOP_MID, 0, 34);
    style_center_label(wheel->current_lbl, CIRCE_FONT_ROLE_HERO, LV_OPA_COVER);

    wheel->next_lbl = lv_label_create(wheel->root);
    lv_obj_set_width(wheel->next_lbl, 260);
    lv_obj_align(wheel->next_lbl, LV_ALIGN_TOP_MID, 0, 78);
    style_center_label(wheel->next_lbl, CIRCE_FONT_ROLE_CAPTION, LV_OPA_40);

    wheel->index_lbl = lv_label_create(wheel->root);
    lv_obj_set_width(wheel->index_lbl, 260);
    lv_obj_align(wheel->index_lbl, LV_ALIGN_TOP_MID, 0, 118);
    style_center_label(wheel->index_lbl, CIRCE_FONT_ROLE_CAPTION, LV_OPA_70);

    update_labels(wheel);
    ESP_LOGI(TAG, "home wheel created: 5 labels, index=%d", wheel->selected);
}

void circe_home_wheel_destroy(circe_home_wheel_t *wheel)
{
    if (!wheel) {
        return;
    }
    if (wheel->root) {
        lv_obj_del(wheel->root);
    }
    wheel->root = NULL;
    wheel->prev_lbl = NULL;
    wheel->current_lbl = NULL;
    wheel->next_lbl = NULL;
    wheel->index_lbl = NULL;
    wheel->active = false;
    wheel->enc_pressed = false;
    wheel->last_release_ms = 0;
    wheel->long_fired = false;
}

void circe_home_wheel_refresh(circe_home_wheel_t *wheel)
{
    update_labels(wheel);
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

void circe_home_wheel_poll(circe_home_wheel_t *wheel, int *action_out)
{
    if (action_out) {
        *action_out = CIRCE_HOME_WHEEL_ACTION_NONE;
    }
    if (!wheel || !wheel->active) {
        return;
    }

    lv_indev_t *enc = find_encoder_indev();
    if (!enc || !enc->driver || !enc->driver->read_cb) {
        return;
    }
    lv_indev_data_t data;
    lv_memset_00(&data, sizeof(data));
    enc->driver->read_cb(enc->driver, &data);

    if (data.enc_diff != 0) {
        wheel->selected = wrap_index(wheel->selected + data.enc_diff);
        update_labels(wheel);
    }

    uint32_t now = lv_tick_get();
    bool pressed = (data.state == LV_INDEV_STATE_PRESSED);

    if (pressed && !wheel->enc_pressed) {
        wheel->enc_pressed = true;
        wheel->press_start_ms = now;
        wheel->long_fired = false;
    } else if (!pressed && wheel->enc_pressed) {
        wheel->enc_pressed = false;
        if (wheel->long_fired) {
            wheel->last_release_ms = 0;
        } else if (wheel->last_release_ms != 0 && (now - wheel->last_release_ms) < HOME_DOUBLE_PRESS_MS) {
            wheel->last_release_ms = 0;
        } else {
            if (action_out) {
                *action_out = CIRCE_HOME_WHEEL_ACTION_OPEN;
            }
            wheel->last_release_ms = now;
        }
    } else if (pressed && wheel->enc_pressed && !wheel->long_fired &&
               (now - wheel->press_start_ms) >= HOME_LONG_PRESS_MS) {
        wheel->long_fired = true;
        if (action_out) {
            *action_out = CIRCE_HOME_WHEEL_ACTION_MORE;
        }
    }
}
