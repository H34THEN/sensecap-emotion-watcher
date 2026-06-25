#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CIRCE_VOICE_MODE_OFF = 0,
    CIRCE_VOICE_MODE_SOFT = 1,
} circe_voice_mode_t;

typedef enum {
    CIRCE_VOICE_EVENT_SAVE_OK = 0,
    CIRCE_VOICE_EVENT_REGULATION_START,
    CIRCE_VOICE_EVENT_BREATHE_INHALE,
    CIRCE_VOICE_EVENT_BREATHE_HOLD,
    CIRCE_VOICE_EVENT_BREATHE_EXHALE,
    CIRCE_VOICE_EVENT_SESSION_COMPLETE,
    CIRCE_VOICE_EVENT_ERROR_SOFT,
} circe_voice_event_t;

void circe_voice_init(void);
bool circe_voice_is_available(void);
bool circe_voice_is_enabled(void);
circe_voice_mode_t circe_voice_get_mode(void);
void circe_voice_set_mode(circe_voice_mode_t mode);
void circe_voice_play_event(circe_voice_event_t event);
