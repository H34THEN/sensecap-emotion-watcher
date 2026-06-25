#include "circe_conversation_engine.h"

#include <string.h>

#include "circe_copy.h"

void circe_conversation_init(circe_conversation_engine_t *engine)
{
    engine->step = CIRCE_FLOW_HOME;
    engine->return_step = CIRCE_FLOW_HOME;
    engine->storage_ready = false;
    engine->editing_existing = false;
    memset(&engine->draft, 0, sizeof(engine->draft));
}

void circe_conversation_start_body_only(circe_conversation_engine_t *engine)
{
    circe_entry_init_defaults(&engine->draft, CIRCE_ENTRY_MODE_BODY_ONLY);
    engine->editing_existing = false;
    engine->step = CIRCE_FLOW_BODY_AREA;
}

void circe_conversation_start_quick(circe_conversation_engine_t *engine)
{
    circe_entry_init_defaults(&engine->draft, CIRCE_ENTRY_MODE_QUICK);
    engine->draft.interaction_mode.short_answer = true;
    engine->step = CIRCE_FLOW_QUICK_PICK;
}

circe_pattern_key_t circe_conversation_prompt_for_step(const circe_conversation_engine_t *engine)
{
    switch (engine->step) {
    case CIRCE_FLOW_HOME:
        return CIRCE_PATTERN_HOME_HEADING;
    case CIRCE_FLOW_READY:
        return CIRCE_PATTERN_READY_PROMPT;
    case CIRCE_FLOW_BODY_AREA:
        return CIRCE_PATTERN_BODY_UNKNOWN_OKAY;
    case CIRCE_FLOW_BODY_SENSATION:
        return CIRCE_PATTERN_BODY_SENSATION_PROMPT;
    case CIRCE_FLOW_INTENSITY:
        return CIRCE_PATTERN_BODY_INTENSITY_PROMPT;
    case CIRCE_FLOW_COLOR_OPTIONAL:
        return CIRCE_PATTERN_COLOR_OPTIONAL_PROMPT;
    case CIRCE_FLOW_SAVE_CONFIRM:
        return CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE;
    case CIRCE_FLOW_QUICK_PICK:
        return CIRCE_PATTERN_QUICK_ONE_TAP;
    case CIRCE_FLOW_REVIEW:
        return CIRCE_PATTERN_SAVE_CONFIRMED;
    case CIRCE_FLOW_REVIEW_EMPTY:
        return CIRCE_PATTERN_HOME_REVIEW;
    case CIRCE_FLOW_DELETE_CONFIRM:
        return CIRCE_PATTERN_DELETE_CONFIRM;
    case CIRCE_FLOW_BODY_ADD_MORE:
        return CIRCE_PATTERN_BODY_ADD_ANOTHER;
    case CIRCE_FLOW_SAVE_DONE:
        return CIRCE_PATTERN_SAVE_CONFIRMED;
    case CIRCE_FLOW_STRAND:
        return CIRCE_PATTERN_STRAND_TODAY;
    case CIRCE_FLOW_EDIT:
        return CIRCE_PATTERN_EDIT_PROMPT;
    case CIRCE_FLOW_EDIT_COLOR:
        return CIRCE_PATTERN_EDIT_COLOR;
    case CIRCE_FLOW_MORE:
        return CIRCE_PATTERN_MORE_MENU;
    case CIRCE_FLOW_APPEARANCE:
        return CIRCE_PATTERN_APPEARANCE_PROMPT;
    case CIRCE_FLOW_DIAGNOSTICS:
        return CIRCE_PATTERN_DIAG_TITLE;
    case CIRCE_FLOW_TIME:
        return CIRCE_PATTERN_TIME_TITLE;
    case CIRCE_FLOW_TIME_EDIT_DATE:
        return CIRCE_PATTERN_TIME_EDIT_DATE;
    case CIRCE_FLOW_TIME_EDIT_TIME:
        return CIRCE_PATTERN_TIME_EDIT_TIME;
    default:
        return CIRCE_PATTERN_GREET_FIRST_TODAY;
    }
}
