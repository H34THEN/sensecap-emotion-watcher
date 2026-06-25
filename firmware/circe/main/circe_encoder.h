#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CIRCE_ENC_ACTION_NONE        0
#define CIRCE_ENC_ACTION_SELECT      1
#define CIRCE_ENC_ACTION_DOUBLE      2
#define CIRCE_ENC_ACTION_TRIPLE      3
#define CIRCE_ENC_ACTION_LONG        4

#define CIRCE_ENC_DOUBLE_MS  450
#define CIRCE_ENC_TRIPLE_MS  550
#define CIRCE_ENC_LONG_MS    800

typedef struct {
    bool pressed;
    uint32_t press_start_ms;
    uint32_t last_release_ms;
    int tap_count;
    bool long_fired;
} circe_encoder_state_t;

void circe_encoder_state_reset(circe_encoder_state_t *st);
int circe_encoder_read_diff(void);
bool circe_encoder_read_pressed(void);
int circe_encoder_poll(circe_encoder_state_t *st, int enc_diff, bool pressed);
