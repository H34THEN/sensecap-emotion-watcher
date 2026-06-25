#pragma once

#include "lvgl.h"

void circe_status_banner_init(lv_obj_t *scr);
void circe_status_banner_show(const char *text);
void circe_status_banner_show_timed(const char *text, uint32_t ms);
void circe_status_banner_hide(void);
bool circe_status_banner_visible(void);
