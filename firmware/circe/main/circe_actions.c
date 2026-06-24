#include "circe_actions.h"

#include "circe_copy.h"
#include "circe_fonts.h"
#include "circe_hud.h"
#include "circe_theme.h"

static lv_obj_t *make_action_btn(lv_obj_t *parent, int w, int h, const char *label, const char *id, lv_event_cb_t cb,
                                 bool primary)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    if (primary) {
        circe_theme_style_primary_button(btn);
    } else {
        circe_theme_style_button(btn);
        lv_obj_set_height(btn, h);
    }
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)id);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    circe_theme_style_action_label(lbl, primary);
    lv_obj_center(lbl);
    return btn;
}

void circe_actions_layout_home_minimal(lv_obj_t *parent, lv_obj_t *scr, circe_home_actions_t *out, lv_event_cb_t cb,
                                       lv_group_t *group)
{
    out->ready = make_action_btn(parent, CIRCE_ACTION_PRIMARY_W, CIRCE_ACTION_PRIMARY_H,
                                 circe_copy_get(CIRCE_PATTERN_HOME_READY), "ready_body", cb, true);
    lv_obj_align(out->ready, LV_ALIGN_TOP_MID, 0, 0);

    out->quick = make_action_btn(parent, CIRCE_ACTION_SECONDARY_W, CIRCE_ACTION_SECONDARY_H,
                                 circe_copy_get(CIRCE_PATTERN_HOME_QUICK), "quick", cb, false);
    lv_obj_align(out->quick, LV_ALIGN_TOP_LEFT, 82, 64);

    out->review = make_action_btn(parent, CIRCE_ACTION_SECONDARY_W, CIRCE_ACTION_SECONDARY_H,
                                  circe_copy_get(CIRCE_PATTERN_HOME_REVIEW), "review", cb, false);
    lv_obj_align(out->review, LV_ALIGN_TOP_RIGHT, -82, 64);

    out->more = lv_btn_create(scr);
    lv_obj_set_size(out->more, 48, 48);
    lv_obj_align(out->more, LV_ALIGN_BOTTOM_RIGHT, -28, -18);
    circe_theme_style_button(out->more);
    lv_obj_set_style_radius(out->more, 24, 0);
    lv_obj_add_event_cb(out->more, cb, LV_EVENT_CLICKED, (void *)"more");
    lv_obj_t *gear = lv_label_create(out->more);
    lv_label_set_text(gear, LV_SYMBOL_SETTINGS);
    circe_theme_style_action_label(gear, false);
    lv_obj_center(gear);

    if (group) {
        lv_group_add_obj(group, out->ready);
        lv_group_add_obj(group, out->quick);
        lv_group_add_obj(group, out->review);
        lv_group_add_obj(group, out->more);
    }
}
