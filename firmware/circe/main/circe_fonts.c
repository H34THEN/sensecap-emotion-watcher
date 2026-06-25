#include "circe_fonts.h"

#include "esp_log.h"

static const char *TAG = "circe_fonts";

#if LV_FONT_MONTSERRAT_32
LV_FONT_DECLARE(lv_font_montserrat_32);
#endif
#if LV_FONT_MONTSERRAT_28
LV_FONT_DECLARE(lv_font_montserrat_28);
#endif
#if LV_FONT_MONTSERRAT_24
LV_FONT_DECLARE(lv_font_montserrat_24);
#endif
#if LV_FONT_MONTSERRAT_20
LV_FONT_DECLARE(lv_font_montserrat_20);
#endif
#if LV_FONT_MONTSERRAT_18
LV_FONT_DECLARE(lv_font_montserrat_18);
#endif
#if LV_FONT_MONTSERRAT_16
LV_FONT_DECLARE(lv_font_montserrat_16);
#endif
#if LV_FONT_MONTSERRAT_14
LV_FONT_DECLARE(lv_font_montserrat_14);
#endif

static const lv_font_t *s_font_default;

static const lv_font_t *pick_montserrat(int preferred_px, int fallback_px)
{
#if LV_FONT_MONTSERRAT_32
    if (preferred_px >= 32) {
        return &lv_font_montserrat_32;
    }
#endif
#if LV_FONT_MONTSERRAT_28
    if (preferred_px >= 28) {
        return &lv_font_montserrat_28;
    }
#endif
#if LV_FONT_MONTSERRAT_24
    if (preferred_px >= 24) {
        return &lv_font_montserrat_24;
    }
#endif
#if LV_FONT_MONTSERRAT_20
    if (preferred_px >= 20) {
        return &lv_font_montserrat_20;
    }
#endif
#if LV_FONT_MONTSERRAT_18
    if (preferred_px >= 18) {
        return &lv_font_montserrat_18;
    }
#endif
#if LV_FONT_MONTSERRAT_16
    if (preferred_px >= 16) {
        return &lv_font_montserrat_16;
    }
#endif
#if LV_FONT_MONTSERRAT_14
    if (fallback_px >= 14) {
        return &lv_font_montserrat_14;
    }
#endif
    (void)fallback_px;
    return s_font_default;
}

void circe_fonts_init(void)
{
    s_font_default = LV_FONT_DEFAULT;
    ESP_LOGI(TAG, "fonts: hero=32 prompt=26 caption=18");
}

const lv_font_t *circe_fonts_get(circe_font_role_t role)
{
    switch (role) {
    case CIRCE_FONT_ROLE_HERO:
        return pick_montserrat(32, 28);
    case CIRCE_FONT_ROLE_HEADING:
        return pick_montserrat(28, 24);
    case CIRCE_FONT_ROLE_PROMPT:
        return pick_montserrat(28, 24);
    case CIRCE_FONT_ROLE_BODY:
        return pick_montserrat(18, 16);
    case CIRCE_FONT_ROLE_BUTTON:
        return pick_montserrat(28, 24);
    case CIRCE_FONT_ROLE_CAPTION:
        return pick_montserrat(18, 18);
    default:
        return pick_montserrat(18, 14);
    }
}

int circe_fonts_rendered_px(circe_font_role_t role)
{
    switch (role) {
    case CIRCE_FONT_ROLE_HERO:
        return 32;
    case CIRCE_FONT_ROLE_HEADING:
        return 28;
    case CIRCE_FONT_ROLE_PROMPT:
        return 26;
    case CIRCE_FONT_ROLE_BODY:
        return 18;
    case CIRCE_FONT_ROLE_BUTTON:
        return 26;
    case CIRCE_FONT_ROLE_CAPTION:
        return 18;
    default:
        return 18;
    }
}

void circe_fonts_apply_label(lv_obj_t *lbl, circe_font_role_t role)
{
    if (!lbl) {
        return;
    }
    lv_obj_set_style_text_font(lbl, circe_fonts_get(role), 0);
}

unsigned circe_fonts_flash_estimate_kb(void)
{
    return 58;
}
