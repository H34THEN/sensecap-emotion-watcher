#pragma once

#include "circe_copy.h"
#include "circe_entry.h"

typedef enum {
    CIRCE_FLOW_HOME = 0,
    CIRCE_FLOW_READY,
    CIRCE_FLOW_BODY_AREA,
    CIRCE_FLOW_BODY_SENSATION,
    CIRCE_FLOW_INTENSITY,
    CIRCE_FLOW_EMOTION_TONE,
    CIRCE_FLOW_COLOR_PICKER,
    CIRCE_FLOW_COLOR_PRESETS,
    CIRCE_FLOW_COLOR_OPTIONAL,
    CIRCE_FLOW_SAVE_CONFIRM,
    CIRCE_FLOW_REVIEW,
    CIRCE_FLOW_REVIEW_EMPTY,
    CIRCE_FLOW_MEMORY_MENU,
    CIRCE_FLOW_MEMORY_BROWSE,
    CIRCE_FLOW_MEMORY_EMPTY,
    CIRCE_FLOW_MEMORY_ERROR,
    CIRCE_FLOW_DELETE_CONFIRM,
    CIRCE_FLOW_QUICK_PICK,
    CIRCE_FLOW_BODY_ADD_MORE,
    CIRCE_FLOW_SAVE_DONE,
    CIRCE_FLOW_REFLECTION,
    CIRCE_FLOW_STRAND,
    CIRCE_FLOW_EDIT,
    CIRCE_FLOW_EDIT_COLOR,
    CIRCE_FLOW_MORE,
    CIRCE_FLOW_APPEARANCE,
    CIRCE_FLOW_DIAGNOSTICS,
    CIRCE_FLOW_TIME,
    CIRCE_FLOW_TIME_EDIT_DATE,
    CIRCE_FLOW_TIME_EDIT_TIME,
    CIRCE_FLOW_GROUNDING,
    CIRCE_FLOW_BREATHING,
    CIRCE_FLOW_BODY_ANCHOR,
    CIRCE_FLOW_REGULATION_SAVE,
} circe_flow_step_t;

typedef struct {
    circe_flow_step_t step;
    circe_flow_step_t return_step;
    circe_entry_t draft;
    bool storage_ready;
    bool editing_existing;
} circe_conversation_engine_t;

void circe_conversation_init(circe_conversation_engine_t *engine);
void circe_conversation_start_body_only(circe_conversation_engine_t *engine);
void circe_conversation_start_quick(circe_conversation_engine_t *engine);
circe_pattern_key_t circe_conversation_prompt_for_step(const circe_conversation_engine_t *engine);
