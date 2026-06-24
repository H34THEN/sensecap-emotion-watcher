#pragma once

#include "lvgl.h"

typedef enum {
    CIRCE_VOICE_UI_IDLE = 0,
    CIRCE_VOICE_UI_LISTENING,
    CIRCE_VOICE_UI_THINKING,
    CIRCE_VOICE_UI_SPEAKING,
    CIRCE_VOICE_UI_MUTED,
} circe_voice_ui_state_t;

typedef struct {
    lv_obj_t *orb;
    circe_voice_ui_state_t state;
} circe_voice_state_ui_t;

void circe_voice_state_ui_init(circe_voice_state_ui_t *ui, lv_obj_t *presence_orb);
void circe_voice_state_ui_apply_theme(circe_voice_state_ui_t *ui);
void circe_voice_state_ui_set(circe_voice_state_ui_t *ui, circe_voice_ui_state_t state);
const char *circe_voice_state_ui_label(circe_voice_ui_state_t state);
