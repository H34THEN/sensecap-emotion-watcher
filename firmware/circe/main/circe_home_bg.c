#include "circe_home_bg.h"

#include "assets/circe_homepage_bg.h"
#include "circe_ui_tokens.h"
#include "esp_log.h"

static const char *TAG = "circe_home_bg";

static lv_obj_t *s_scr;
static lv_obj_t *s_img;

bool circe_home_bg_is_enabled(void)
{
#if CIRCE_UI_HOME_USE_STATIC_BG
    return true;
#else
    return false;
#endif
}

void circe_home_bg_init(lv_obj_t *scr)
{
    if (!scr || s_img) {
        return;
    }
    s_scr = scr;
    s_img = lv_img_create(scr);
    lv_img_set_src(s_img, &circe_homepage_bg);
    lv_obj_set_pos(s_img, CIRCE_UI_HOME_BG_X, CIRCE_UI_HOME_BG_Y);
    lv_img_set_size_mode(s_img, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_antialias(s_img, false);
    lv_obj_clear_flag(s_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_img, LV_OBJ_FLAG_HIDDEN);
}

void circe_home_bg_show(void)
{
    if (!s_img || !circe_home_bg_is_enabled()) {
        return;
    }
    lv_obj_clear_flag(s_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_background(s_img);
    if (s_scr) {
        lv_obj_set_style_bg_opa(s_scr, LV_OPA_TRANSP, 0);
    }
    ESP_LOGI(TAG, "CIRCE home static background enabled: %dx%d RGB565", CIRCE_HOMEPAGE_BG_W, CIRCE_HOMEPAGE_BG_H);
}

void circe_home_bg_hide(void)
{
    if (!s_img) {
        return;
    }
    lv_obj_add_flag(s_img, LV_OBJ_FLAG_HIDDEN);
}

bool circe_home_bg_is_visible(void)
{
    return s_img && !lv_obj_has_flag(s_img, LV_OBJ_FLAG_HIDDEN);
}
