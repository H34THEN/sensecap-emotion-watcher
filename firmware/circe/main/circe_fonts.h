#pragma once

#include "lvgl.h"

typedef enum {
    CIRCE_FONT_ROLE_HERO = 0,
    CIRCE_FONT_ROLE_HEADING,
    CIRCE_FONT_ROLE_PROMPT,
    CIRCE_FONT_ROLE_BODY,
    CIRCE_FONT_ROLE_BUTTON,
    CIRCE_FONT_ROLE_CAPTION,
} circe_font_role_t;

void circe_fonts_init(void);
const lv_font_t *circe_fonts_get(circe_font_role_t role);
void circe_fonts_apply_label(lv_obj_t *lbl, circe_font_role_t role);
unsigned circe_fonts_flash_estimate_kb(void);
int circe_fonts_rendered_px(circe_font_role_t role);
