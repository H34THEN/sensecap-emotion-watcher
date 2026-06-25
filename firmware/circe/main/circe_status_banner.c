#include "circe_status_banner.h"

#include "circe_fonts.h"
#include "circe_theme.h"

#define BANNER_W 220
#define BANNER_H 56

static lv_obj_t *s_panel;
static lv_obj_t *s_label;
static lv_timer_t *s_hide_timer;

static void stop_hide_timer(void)
{
    if (s_hide_timer) {
        lv_timer_del(s_hide_timer);
        s_hide_timer = NULL;
    }
}

static void hide_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    s_hide_timer = NULL;
    if (s_panel) {
        lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

void circe_status_banner_init(lv_obj_t *scr)
{
    if (!scr || s_panel) {
        return;
    }
    s_panel = lv_obj_create(scr);
    lv_obj_set_size(s_panel, BANNER_W, BANNER_H);
    lv_obj_align(s_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(s_panel, 4, 0);
    lv_obj_set_style_border_width(s_panel, 2, 0);
    lv_obj_set_style_border_color(s_panel, circe_theme_color(0x000000), 0);
    lv_obj_set_style_bg_color(s_panel, circe_theme_color(0xFF2BD6), 0);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);

    s_label = lv_label_create(s_panel);
    lv_obj_set_width(s_label, BANNER_W - 16);
    lv_label_set_long_mode(s_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_label, LV_ALIGN_CENTER, 0, 0);
    circe_fonts_apply_label(s_label, CIRCE_FONT_ROLE_HERO);
    lv_obj_set_style_text_color(s_label, circe_theme_color(0x000000), 0);
    lv_obj_set_style_text_align(s_label, LV_TEXT_ALIGN_CENTER, 0);
}

void circe_status_banner_show(const char *text)
{
    if (!s_panel || !s_label) {
        return;
    }
    stop_hide_timer();
    lv_label_set_text(s_label, text ? text : "");
    lv_obj_clear_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_panel);
}

void circe_status_banner_hide(void)
{
    if (!s_panel) {
        return;
    }
    stop_hide_timer();
    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
}

void circe_status_banner_reset(void)
{
    stop_hide_timer();
    if (s_panel) {
        lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

bool circe_status_banner_visible(void)
{
    return s_panel && !lv_obj_has_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
}

bool circe_status_banner_is_visible(void)
{
    return circe_status_banner_visible();
}

bool circe_status_banner_has_auto_hide(void)
{
    return s_hide_timer != NULL;
}

void circe_status_banner_dismiss_indefinite(void)
{
    if (!s_hide_timer && s_panel && !lv_obj_has_flag(s_panel, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
    }
}

void circe_status_banner_show_timed(const char *text, uint32_t ms)
{
    circe_status_banner_show(text);
    if (ms > 0) {
        s_hide_timer = lv_timer_create(hide_timer_cb, ms, NULL);
        lv_timer_set_repeat_count(s_hide_timer, 1);
    }
}
