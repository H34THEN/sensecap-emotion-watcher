#pragma once

#include <stdbool.h>

#include "circe_encoder.h"
#include "lvgl.h"

#define CIRCE_SELECTOR_MAX_ITEMS 12

#define CIRCE_SELECTOR_ACTION_NONE   CIRCE_ENC_ACTION_NONE
#define CIRCE_SELECTOR_ACTION_SELECT CIRCE_ENC_ACTION_SELECT
#define CIRCE_SELECTOR_ACTION_DOUBLE CIRCE_ENC_ACTION_DOUBLE
#define CIRCE_SELECTOR_ACTION_TRIPLE CIRCE_ENC_ACTION_TRIPLE
#define CIRCE_SELECTOR_ACTION_LONG   CIRCE_ENC_ACTION_LONG

typedef struct {
    const char *label;
    const char *action_id;
} circe_selector_item_t;

typedef struct {
    lv_obj_t *root;
    lv_obj_t *title_lbl;
    lv_obj_t *current_lbl;
    lv_obj_t *index_lbl;
    lv_obj_t *hint_lbl;
    const circe_selector_item_t *items;
    int count;
    int selected;
    bool active;
    circe_encoder_state_t enc;
} circe_selector_t;

void circe_selector_create(circe_selector_t *sel, lv_obj_t *parent, const char *title,
                           const circe_selector_item_t *items, int count, int initial_index);
void circe_selector_destroy(circe_selector_t *sel);
void circe_selector_refresh(circe_selector_t *sel);
int circe_selector_poll(circe_selector_t *sel);
const char *circe_selector_selected_action(const circe_selector_t *sel);
const char *circe_selector_selected_label(const circe_selector_t *sel);
