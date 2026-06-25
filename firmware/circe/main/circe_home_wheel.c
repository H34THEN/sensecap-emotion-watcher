#include "circe_home_wheel.h"

#include <stdio.h>

#include "circe_copy.h"
#include "circe_encoder.h"
#include "circe_fonts.h"
#include "circe_theme.h"
#include "esp_log.h"

static const char *TAG = "circe_home_wheel";

static const char *s_home_labels[CIRCE_HOME_WHEEL_COUNT] = {
    "BODY CHECK-IN",
    "REVIEW",
    "REGULATE",
    "SETTINGS",
    "DIAGNOSTICS",
    "QUICK NOTE",
};

static const char *s_home_actions[CIRCE_HOME_WHEEL_COUNT] = {
    "ready_body",
    "review",
    "regulate",
    "more",
    "diagnostics",
    "quick",
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

static void style_center_label(lv_obj_t *lbl, circe_font_role_t role, lv_opa_t opa, uint32_t color_hex)
{
    circe_fonts_apply_label(lbl, role);
    lv_obj_set_style_text_color(lbl, circe_theme_color(color_hex), 0);
    lv_obj_set_style_text_opa(lbl, opa, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
}

static void update_labels(circe_home_wheel_t *wheel)
{
    if (!wheel || !wheel->active) {
        return;
    }
    const circe_theme_palette_t *p = circe_theme_get_palette();
    if (wheel->current_lbl) {
        lv_label_set_text(wheel->current_lbl, s_home_labels[wheel->selected]);
    }
    if (wheel->index_lbl) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d / %d", wheel->selected + 1, CIRCE_HOME_WHEEL_COUNT);
        lv_label_set_text(wheel->index_lbl, buf);
    }
    if (wheel->hint_lbl) {
        char hint[96];
        snprintf(hint, sizeof(hint), "%s · %s · %s", circe_copy_get(CIRCE_PATTERN_NAV_ROTATE_CHOOSE),
                 circe_copy_get(CIRCE_PATTERN_NAV_PRESS_SELECT), circe_copy_get(CIRCE_PATTERN_NAV_TRIPLE_HOME));
        lv_label_set_text(wheel->hint_lbl, hint);
        style_center_label(wheel->hint_lbl, CIRCE_FONT_ROLE_CAPTION, LV_OPA_60, p->muted);
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
    circe_encoder_state_reset(&wheel->enc);

    const circe_theme_palette_t *p = circe_theme_get_palette();

    wheel->root = lv_obj_create(parent);
    lv_obj_set_size(wheel->root, 280, 148);
    lv_obj_align(wheel->root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(wheel->root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wheel->root, 0, 0);
    lv_obj_set_style_pad_all(wheel->root, 0, 0);
    lv_obj_clear_flag(wheel->root, LV_OBJ_FLAG_SCROLLABLE);

    wheel->prev_lbl = NULL;
    wheel->next_lbl = NULL;

    wheel->current_lbl = lv_label_create(wheel->root);
    lv_obj_set_width(wheel->current_lbl, 260);
    lv_obj_align(wheel->current_lbl, LV_ALIGN_TOP_MID, 0, 24);
    style_center_label(wheel->current_lbl, CIRCE_FONT_ROLE_HERO, LV_OPA_COVER, p->text);

    wheel->index_lbl = lv_label_create(wheel->root);
    lv_obj_set_width(wheel->index_lbl, 260);
    lv_obj_align(wheel->index_lbl, LV_ALIGN_TOP_MID, 0, 72);
    style_center_label(wheel->index_lbl, CIRCE_FONT_ROLE_CAPTION, LV_OPA_80, p->accent_primary);

    wheel->hint_lbl = lv_label_create(wheel->root);
    lv_obj_set_width(wheel->hint_lbl, 260);
    lv_obj_align(wheel->hint_lbl, LV_ALIGN_TOP_MID, 0, 102);
    style_center_label(wheel->hint_lbl, CIRCE_FONT_ROLE_CAPTION, LV_OPA_60, p->muted);

    update_labels(wheel);
    ESP_LOGI(TAG, "home wheel created: %d options, index=%d", CIRCE_HOME_WHEEL_COUNT, wheel->selected);
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
    wheel->hint_lbl = NULL;
    wheel->active = false;
    circe_encoder_state_reset(&wheel->enc);
}

void circe_home_wheel_refresh(circe_home_wheel_t *wheel)
{
    update_labels(wheel);
}

void circe_home_wheel_poll(circe_home_wheel_t *wheel, int *action_out)
{
    if (action_out) {
        *action_out = CIRCE_HOME_WHEEL_ACTION_NONE;
    }
    if (!wheel || !wheel->active) {
        return;
    }

    int diff = circe_encoder_read_diff();
    if (diff != 0) {
        wheel->selected = wrap_index(wheel->selected + diff);
        update_labels(wheel);
    }

    bool pressed = circe_encoder_read_pressed();
    int enc_action = circe_encoder_poll(&wheel->enc, diff, pressed);
    if (enc_action == CIRCE_ENC_ACTION_SELECT && action_out) {
        *action_out = CIRCE_HOME_WHEEL_ACTION_OPEN;
    } else if (enc_action == CIRCE_ENC_ACTION_LONG && action_out) {
        *action_out = CIRCE_HOME_WHEEL_ACTION_MORE;
    } else if (enc_action == CIRCE_ENC_ACTION_DOUBLE && action_out) {
        *action_out = CIRCE_HOME_WHEEL_ACTION_DOUBLE;
    } else if (enc_action == CIRCE_ENC_ACTION_TRIPLE && action_out) {
        *action_out = CIRCE_HOME_WHEEL_ACTION_TRIPLE;
    }
}
