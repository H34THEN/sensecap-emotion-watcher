#pragma once

#include "lvgl.h"

#include "circe_voice_state_ui.h"

#define CIRCE_HUD_VIEWPORT_W 252
#define CIRCE_HUD_VIEWPORT_H 120
#define CIRCE_PRESENCE_SIZE  32

#define CIRCE_ACTION_PRIMARY_W 240
#define CIRCE_ACTION_PRIMARY_H 56
#define CIRCE_ACTION_SECONDARY_W 116
#define CIRCE_ACTION_SECONDARY_H 52

typedef struct {
    lv_obj_t *safe_ring;
    lv_obj_t *left_arc;
    lv_obj_t *right_arc;
    lv_obj_t *top_arc;
    lv_obj_t *viewport;
    lv_obj_t *presence;
    lv_obj_t *heading;
    lv_obj_t *prompt;
    lv_obj_t *response;
    lv_obj_t *actions;
    lv_obj_t *status;
    lv_obj_t *top_line;
    lv_obj_t *bottom_line;
    circe_voice_state_ui_t voice;
} circe_hud_t;

void circe_hud_create(lv_obj_t *scr, circe_hud_t *hud);
void circe_hud_apply_theme(circe_hud_t *hud);
void circe_hud_set_reset_mode(circe_hud_t *hud, bool enabled);
lv_obj_t *circe_hud_strand_layer(const circe_hud_t *hud);
lv_obj_t *circe_hud_actions(const circe_hud_t *hud);
circe_voice_state_ui_t *circe_hud_voice_state(circe_hud_t *hud);
void circe_hud_set_heading(circe_hud_t *hud, const char *text);
void circe_hud_set_prompt(circe_hud_t *hud, const char *text);
void circe_hud_set_response(circe_hud_t *hud, const char *text);
void circe_hud_set_subline(circe_hud_t *hud, const char *text);
void circe_hud_show_minimal_home(circe_hud_t *hud, const char *heading, const char *subline);
void circe_hud_show_companion_question(circe_hud_t *hud, const char *question, const char *subline);
void circe_hud_show_companion_home(circe_hud_t *hud, const char *heading, const char *prompt, const char *subline);
void circe_hud_show_companion_prompt(circe_hud_t *hud, const char *prompt, const char *response);
void circe_hud_show_home(circe_hud_t *hud, const char *heading, const char *subline);
void circe_hud_show_prompt(circe_hud_t *hud, const char *prompt);
