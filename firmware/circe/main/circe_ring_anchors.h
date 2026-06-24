#pragma once

#include "lvgl.h"

typedef struct {
    lv_obj_t *calm;
    lv_obj_t *focus;
    lv_obj_t *balance;
    lv_obj_t *energy;
} circe_ring_anchors_t;

void circe_ring_anchors_create(lv_obj_t *scr, circe_ring_anchors_t *ring);
void circe_ring_anchors_apply_theme(circe_ring_anchors_t *ring);
