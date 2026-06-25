#pragma once

#include <stdbool.h>

#include "lvgl.h"

void circe_home_bg_init(lv_obj_t *scr);
void circe_home_bg_show(void);
void circe_home_bg_hide(void);
bool circe_home_bg_is_visible(void);
bool circe_home_bg_is_enabled(void);
