#pragma once

#include <stdbool.h>

#include "lvgl.h"

#include "circe_ui_tokens.h"

#define CIRCE_COLOR_PICKER_FIELD_W     CIRCE_UI_COLOR_FIELD_W
#define CIRCE_COLOR_PICKER_FIELD_H     CIRCE_UI_COLOR_FIELD_H
#define CIRCE_COLOR_PICKER_CANVAS_W    CIRCE_UI_COLOR_CANVAS_W
#define CIRCE_COLOR_PICKER_CANVAS_H    CIRCE_UI_COLOR_CANVAS_H
#define CIRCE_COLOR_PICKER_MAG_SIZE    CIRCE_UI_COLOR_MAG_SIZE
#define CIRCE_COLOR_PICKER_HEX_Y_OFS   CIRCE_UI_COLOR_HEX_Y_OFS
#define CIRCE_COLOR_PICKER_TRAIT_Y_OFS CIRCE_UI_COLOR_TRAIT_Y_OFS
#define CIRCE_COLOR_PICKER_SAT         85
#define CIRCE_COLOR_PICKER_LVGL_OBJS   8

typedef struct {
    lv_obj_t *root;
    lv_obj_t *field;
    lv_obj_t *hex_label;
    lv_obj_t *trait_label;
    lv_obj_t *preview;
    lv_obj_t *magnifier;
    lv_obj_t *cross_h;
    lv_obj_t *cross_v;
    lv_color_t *canvas_buf;
    char hex[8];
    char near_label[24];
    int hue;
    int value;
    bool active;
    bool color_set;
    bool enc_tune_value;
    bool enc_btn_last;
} circe_color_picker_t;

void circe_color_picker_create(circe_color_picker_t *picker, lv_obj_t *parent);
void circe_color_picker_destroy(circe_color_picker_t *picker);
void circe_color_picker_set_hex(circe_color_picker_t *picker, const char *hex);
void circe_color_picker_refresh(circe_color_picker_t *picker);
void circe_color_picker_apply_to_draft(circe_color_picker_t *picker, char *hex_out, size_t hex_len, char *label_out,
                                       size_t label_len, char *source_out, size_t source_len);
bool circe_color_picker_poll_encoder(circe_color_picker_t *picker);
