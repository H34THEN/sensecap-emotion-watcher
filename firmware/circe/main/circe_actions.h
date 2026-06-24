#pragma once

#include "lvgl.h"

typedef struct {
    lv_obj_t *ready;
    lv_obj_t *quick;
    lv_obj_t *review;
    lv_obj_t *more;
} circe_home_actions_t;

void circe_actions_layout_home_minimal(lv_obj_t *parent, lv_obj_t *scr, circe_home_actions_t *out, lv_event_cb_t cb,
                                       lv_group_t *group);
