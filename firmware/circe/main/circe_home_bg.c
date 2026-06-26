#include "circe_home_bg.h"

#include "circe_theme.h"
#include "circe_ui_tokens.h"
#include "esp_log.h"

static const char *TAG = "circe_home_bg";

static lv_obj_t *s_ring;

bool circe_home_bg_is_enabled(void)
{
#if CIRCE_UI_HOME_OUTER_RING
    return true;
#else
    return false;
#endif
}

static void apply_ring_theme(void)
{
    if (!s_ring) {
        return;
    }
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_arc_color(s_ring, circe_theme_color(p->border), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(s_ring, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(s_ring, false, LV_PART_MAIN);
}

void circe_home_bg_init(lv_obj_t *scr)
{
    if (!scr || s_ring) {
        return;
    }

    s_ring = lv_arc_create(scr);
    lv_obj_set_size(s_ring, CIRCE_UI_HOME_OUTER_RING_SIZE, CIRCE_UI_HOME_OUTER_RING_SIZE);
    lv_obj_align(s_ring, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_mode(s_ring, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(s_ring, 0, 360);
    lv_arc_set_angles(s_ring, 0, 360);
    lv_obj_set_style_arc_width(s_ring, CIRCE_UI_HOME_OUTER_RING_W, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_ring, 0, LV_PART_INDICATOR);
    lv_obj_remove_style(s_ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(s_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(s_ring, 0, 0);
    apply_ring_theme();
    lv_obj_add_flag(s_ring, LV_OBJ_FLAG_HIDDEN);
}

void circe_home_bg_show(void)
{
    if (!s_ring || !circe_home_bg_is_enabled()) {
        return;
    }
    apply_ring_theme();
    lv_obj_clear_flag(s_ring, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_background(s_ring);
    ESP_LOGI(TAG, "CIRCE home outer ring enabled: %dpx on %dx%d canvas", CIRCE_UI_HOME_OUTER_RING_W,
             CIRCE_UI_HOME_OUTER_RING_SIZE, CIRCE_UI_HOME_OUTER_RING_SIZE);
}

void circe_home_bg_hide(void)
{
    if (!s_ring) {
        return;
    }
    lv_obj_add_flag(s_ring, LV_OBJ_FLAG_HIDDEN);
}

bool circe_home_bg_is_visible(void)
{
    return s_ring && !lv_obj_has_flag(s_ring, LV_OBJ_FLAG_HIDDEN);
}
