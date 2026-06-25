#pragma once

#include <stdbool.h>

#include "circe_encoder.h"
#include "lvgl.h"

#define CIRCE_HOME_WHEEL_COUNT       6
#define CIRCE_HOME_WHEEL_ACTION_NONE   (-1)
#define CIRCE_HOME_WHEEL_ACTION_OPEN   100
#define CIRCE_HOME_WHEEL_ACTION_MORE   101
#define CIRCE_HOME_WHEEL_ACTION_DOUBLE 102
#define CIRCE_HOME_WHEEL_ACTION_TRIPLE 103

typedef struct {
    lv_obj_t *root;
    lv_obj_t *prev_lbl;
    lv_obj_t *current_lbl;
    lv_obj_t *next_lbl;
    lv_obj_t *index_lbl;
    lv_obj_t *hint_lbl;
    int selected;
    bool active;
    circe_encoder_state_t enc;
} circe_home_wheel_t;

const char *circe_home_wheel_label(int index);
const char *circe_home_wheel_action_id(int index);

void circe_home_wheel_create(circe_home_wheel_t *wheel, lv_obj_t *parent, int initial_index);
void circe_home_wheel_destroy(circe_home_wheel_t *wheel);
void circe_home_wheel_refresh(circe_home_wheel_t *wheel);
void circe_home_wheel_poll(circe_home_wheel_t *wheel, int *action_out);
