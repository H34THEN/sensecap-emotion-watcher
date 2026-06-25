#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "circe_conversation_engine.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *rows[5];
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int field;
    bool active;
    bool enc_was_enabled;
    lv_indev_t *enc;
    bool enc_pressed;
    uint32_t press_start_ms;
    uint32_t last_release_ms;
    bool long_fired;
} circe_time_picker_t;

typedef void (*circe_time_picker_cb_t)(bool saved, void *ctx);

void circe_time_picker_init_values(circe_time_picker_t *picker);
void circe_time_picker_create(circe_time_picker_t *picker, lv_obj_t *parent);
void circe_time_picker_destroy(circe_time_picker_t *picker);
void circe_time_picker_refresh(circe_time_picker_t *picker);
void circe_time_picker_poll(circe_time_picker_t *picker, circe_time_picker_cb_t on_done, void *ctx);
