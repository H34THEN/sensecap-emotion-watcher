#include "circe_regulation.h"

#include <stdio.h>
#include <string.h>

#include "circe_copy.h"
#include "circe_fonts.h"
#include "circe_theme.h"
#include "circe_ui_tokens.h"
#include "circe_voice.h"
#include "esp_log.h"

#define REG_DOUBLE_PRESS_MS 450
#define REG_LONG_PRESS_MS   800
#define REG_BREATH_ROUNDS   3
#define REG_TICK_MS         1000
#define REG_BILATERAL_PACE_DEFAULT 1000
#define REG_BILATERAL_PACE_MIN     700
#define REG_BILATERAL_PACE_MAX     1500

static const char *TAG = "circe_regulation";

typedef enum {
    REG_PHASE_INHALE = 0,
    REG_PHASE_HOLD,
    REG_PHASE_EXHALE,
} reg_breath_phase_t;

typedef enum {
    REG_MODE_NONE = 0,
    REG_MODE_BREATHING,
    REG_MODE_BODY_ANCHOR,
    REG_MODE_54321,
    REG_MODE_SENSORY,
    REG_MODE_BILATERAL,
} reg_mode_t;

static struct {
    bool active;
    reg_mode_t mode;
    circe_regulation_result_t *result;
    circe_regulation_action_cb_t action_cb;
    void *action_ctx;
    lv_obj_t *root;
    lv_timer_t *timer;
    bool paused;
    int round;
    int max_rounds;
    reg_breath_phase_t phase;
    int count;
    int duration_sec;
    lv_obj_t *phase_lbl;
    lv_obj_t *count_lbl;
    lv_obj_t *orb;
    lv_obj_t *ring;
    int prompt_idx;
    int prompt_count;
    const circe_pattern_key_t *prompt_keys;
    lv_obj_t *prompt_lbl;
    lv_obj_t *step_lbl;
    bool bilateral_left;
    int bilateral_cycles;
    int pace_ms;
    lv_obj_t *left_dot;
    lv_obj_t *right_dot;
    lv_obj_t *side_lbl;
    bool sensory_dim;
    bool enc_pressed;
    uint32_t press_start_ms;
    uint32_t last_release_ms;
    bool long_fired;
} s_reg;

static const char *s_anchor_prompts[] = {
    "find one contact point",
    "notice your hands",
    "soften what can soften",
    "name one safe object nearby",
};

static const circe_pattern_key_t s_54321_keys[] = {
    CIRCE_PATTERN_REG_54321_STEP_1,
    CIRCE_PATTERN_REG_54321_STEP_2,
    CIRCE_PATTERN_REG_54321_STEP_3,
    CIRCE_PATTERN_REG_54321_STEP_4,
    CIRCE_PATTERN_REG_54321_STEP_5,
};

static const circe_pattern_key_t s_sensory_keys[] = {
    CIRCE_PATTERN_REG_SENSORY_STEP_1,
    CIRCE_PATTERN_REG_SENSORY_STEP_2,
    CIRCE_PATTERN_REG_SENSORY_STEP_3,
    CIRCE_PATTERN_REG_SENSORY_STEP_4,
    CIRCE_PATTERN_REG_SENSORY_STEP_5,
};

static bool mode_is_step(reg_mode_t mode)
{
    return mode == REG_MODE_BODY_ANCHOR || mode == REG_MODE_54321 || mode == REG_MODE_SENSORY;
}

static void finish_bilateral(bool completed);
static void update_bilateral_visual(void);

static const char *phase_name(reg_breath_phase_t phase)
{
    switch (phase) {
    case REG_PHASE_HOLD:
        return "HOLD";
    case REG_PHASE_EXHALE:
        return "EXHALE";
    case REG_PHASE_INHALE:
    default:
        return "INHALE";
    }
}

static int phase_start_count(reg_breath_phase_t phase)
{
    switch (phase) {
    case REG_PHASE_HOLD:
        return 2;
    case REG_PHASE_EXHALE:
        return 6;
    case REG_PHASE_INHALE:
    default:
        return 4;
    }
}

static void style_prompt_label(lv_obj_t *lbl)
{
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_PROMPT);
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->text), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
}

static void emit_action(int action)
{
    if (s_reg.action_cb) {
        s_reg.action_cb(action, s_reg.action_ctx);
    }
}

static void update_breath_orb(void)
{
    if (!s_reg.orb) {
        return;
    }
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_opa_t pulse = (lv_opa_t)(LV_OPA_40 + (s_reg.count * 15));
    if (pulse > (lv_opa_t)250) {
        pulse = (lv_opa_t)250;
    }
    lv_obj_set_style_bg_color(s_reg.orb, circe_theme_color(p->focus), 0);
    lv_obj_set_style_bg_opa(s_reg.orb, pulse, 0);
    if (s_reg.ring) {
        lv_obj_set_style_arc_color(s_reg.ring, circe_theme_color(p->accent_muted), LV_PART_MAIN);
        lv_obj_set_style_arc_opa(s_reg.ring, pulse, LV_PART_MAIN);
    }
}

static void update_breath_labels(void)
{
    if (s_reg.phase_lbl) {
        lv_label_set_text(s_reg.phase_lbl, phase_name(s_reg.phase));
    }
    if (s_reg.count_lbl) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", s_reg.count);
        lv_label_set_text(s_reg.count_lbl, buf);
    }
    update_breath_orb();
}

static void finish_breathing(bool completed)
{
    if (!s_reg.result) {
        return;
    }
    strncpy(s_reg.result->regulation_type, "breathing", sizeof(s_reg.result->regulation_type) - 1);
    s_reg.result->regulation_type[sizeof(s_reg.result->regulation_type) - 1] = '\0';
    s_reg.result->duration_seconds = s_reg.duration_sec;
    s_reg.result->session_completed = completed;
    if (completed) {
        s_reg.result->rounds_completed = s_reg.max_rounds;
    } else if (s_reg.round > 1) {
        s_reg.result->rounds_completed = s_reg.round - 1;
    } else {
        s_reg.result->rounds_completed = 0;
    }
    emit_action(completed ? CIRCE_REG_ACT_COMPLETE : CIRCE_REG_ACT_END);
    if (completed) {
        circe_voice_play_event(CIRCE_VOICE_EVENT_SESSION_COMPLETE);
    }
}

static void advance_breath_phase(void)
{
    if (s_reg.phase == REG_PHASE_INHALE) {
        s_reg.phase = REG_PHASE_HOLD;
        s_reg.count = phase_start_count(REG_PHASE_HOLD);
    } else if (s_reg.phase == REG_PHASE_HOLD) {
        s_reg.phase = REG_PHASE_EXHALE;
        s_reg.count = phase_start_count(REG_PHASE_EXHALE);
    } else {
        if (s_reg.round >= s_reg.max_rounds) {
            finish_breathing(true);
            return;
        }
        s_reg.round++;
        s_reg.phase = REG_PHASE_INHALE;
        s_reg.count = phase_start_count(REG_PHASE_INHALE);
    }
    update_breath_labels();
    switch (s_reg.phase) {
    case REG_PHASE_INHALE:
        circe_voice_play_event(CIRCE_VOICE_EVENT_BREATHE_INHALE);
        break;
    case REG_PHASE_HOLD:
        circe_voice_play_event(CIRCE_VOICE_EVENT_BREATHE_HOLD);
        break;
    case REG_PHASE_EXHALE:
        circe_voice_play_event(CIRCE_VOICE_EVENT_BREATHE_EXHALE);
        break;
    default:
        break;
    }
}

static void breath_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_reg.active || s_reg.mode != REG_MODE_BREATHING || s_reg.paused) {
        return;
    }
    s_reg.duration_sec++;
    if (s_reg.count > 1) {
        s_reg.count--;
        update_breath_labels();
        return;
    }
    advance_breath_phase();
}

static void finish_steps_session(const char *type, bool completed)
{
    if (!s_reg.result) {
        return;
    }
    strncpy(s_reg.result->regulation_type, type, sizeof(s_reg.result->regulation_type) - 1);
    s_reg.result->regulation_type[sizeof(s_reg.result->regulation_type) - 1] = '\0';
    s_reg.result->duration_seconds = s_reg.duration_sec;
    s_reg.result->session_completed = completed;
    s_reg.result->rounds_completed = 0;
    if (completed) {
        s_reg.result->steps_completed = s_reg.prompt_count;
    } else if (s_reg.prompt_idx > 0) {
        s_reg.result->steps_completed = s_reg.prompt_idx + 1;
    } else {
        s_reg.result->steps_completed = 0;
    }
    emit_action(completed ? CIRCE_REG_ACT_COMPLETE : CIRCE_REG_ACT_END);
    if (completed) {
        circe_voice_play_event(CIRCE_VOICE_EVENT_SESSION_COMPLETE);
    }
}

static void finish_body_anchor(bool completed)
{
    finish_steps_session("body_anchor", completed);
}

static void update_step_prompt(void)
{
    if (!s_reg.prompt_lbl || s_reg.prompt_idx < 0 || s_reg.prompt_idx >= s_reg.prompt_count) {
        return;
    }
    const char *text = NULL;
    if (s_reg.mode == REG_MODE_BODY_ANCHOR && s_reg.prompt_idx < (int)(sizeof(s_anchor_prompts) / sizeof(s_anchor_prompts[0]))) {
        char line[72];
        snprintf(line, sizeof(line), "> %s", s_anchor_prompts[s_reg.prompt_idx]);
        lv_label_set_text(s_reg.prompt_lbl, line);
    } else if (s_reg.prompt_keys) {
        text = circe_copy_get(s_reg.prompt_keys[s_reg.prompt_idx]);
        lv_label_set_text(s_reg.prompt_lbl, text ? text : "");
    }
    if (s_reg.step_lbl) {
        char sub[32];
        snprintf(sub, sizeof(sub), "%d / %d", s_reg.prompt_idx + 1, s_reg.prompt_count);
        lv_label_set_text(s_reg.step_lbl, sub);
    }
}

static void step_next_prompt(void)
{
    if (s_reg.prompt_idx + 1 >= s_reg.prompt_count) {
        if (s_reg.mode == REG_MODE_54321) {
            finish_steps_session("grounding_54321", true);
        } else if (s_reg.mode == REG_MODE_SENSORY) {
            finish_steps_session("sensory_reset", true);
        } else {
            finish_body_anchor(true);
        }
        return;
    }
    s_reg.prompt_idx++;
    update_step_prompt();
}

static void poll_encoder(void)
{
    lv_indev_t *indev = NULL;
    lv_indev_data_t data;
    bool found = false;
    while ((indev = lv_indev_get_next(indev)) != NULL) {
        if (indev->driver->type != LV_INDEV_TYPE_ENCODER) {
            continue;
        }
        if (!indev->driver->read_cb) {
            break;
        }
        lv_memset_00(&data, sizeof(data));
        indev->driver->read_cb(indev->driver, &data);
        found = true;
        break;
    }
    if (!found) {
        return;
    }

    uint32_t now = lv_tick_get();
    bool pressed = (data.state == LV_INDEV_STATE_PRESSED);

    if (mode_is_step(s_reg.mode) && data.enc_diff != 0) {
        s_reg.prompt_idx += data.enc_diff;
        if (s_reg.prompt_idx < 0) {
            s_reg.prompt_idx = 0;
        }
        if (s_reg.prompt_idx >= s_reg.prompt_count) {
            s_reg.prompt_idx = s_reg.prompt_count - 1;
        }
        update_step_prompt();
    }

    if (s_reg.mode == REG_MODE_BILATERAL && data.enc_diff != 0) {
        s_reg.pace_ms += data.enc_diff * 100;
        if (s_reg.pace_ms < REG_BILATERAL_PACE_MIN) {
            s_reg.pace_ms = REG_BILATERAL_PACE_MIN;
        }
        if (s_reg.pace_ms > REG_BILATERAL_PACE_MAX) {
            s_reg.pace_ms = REG_BILATERAL_PACE_MAX;
        }
        if (s_reg.timer) {
            lv_timer_set_period(s_reg.timer, (uint32_t)s_reg.pace_ms);
        }
    }

    if (pressed && !s_reg.enc_pressed) {
        s_reg.enc_pressed = true;
        s_reg.press_start_ms = now;
        s_reg.long_fired = false;
    } else if (!pressed && s_reg.enc_pressed) {
        s_reg.enc_pressed = false;
        if (s_reg.long_fired) {
            s_reg.last_release_ms = 0;
        } else if (s_reg.last_release_ms != 0 && (now - s_reg.last_release_ms) < REG_DOUBLE_PRESS_MS) {
            emit_action(CIRCE_REG_ACT_BACK);
            s_reg.last_release_ms = 0;
        } else {
            if (s_reg.mode == REG_MODE_BREATHING) {
                s_reg.paused = !s_reg.paused;
            } else if (s_reg.mode == REG_MODE_BILATERAL) {
                s_reg.paused = !s_reg.paused;
            } else if (mode_is_step(s_reg.mode)) {
                step_next_prompt();
            }
            s_reg.last_release_ms = now;
        }
    } else if (pressed && s_reg.enc_pressed && !s_reg.long_fired && (now - s_reg.press_start_ms) >= REG_LONG_PRESS_MS) {
        s_reg.long_fired = true;
        if (s_reg.mode == REG_MODE_BREATHING) {
            finish_breathing(false);
        } else if (s_reg.mode == REG_MODE_BILATERAL) {
            finish_bilateral(false);
        } else if (s_reg.mode == REG_MODE_54321) {
            finish_steps_session("grounding_54321", false);
        } else if (s_reg.mode == REG_MODE_SENSORY) {
            finish_steps_session("sensory_reset", false);
        } else if (s_reg.mode == REG_MODE_BODY_ANCHOR) {
            finish_body_anchor(false);
        }
    }
}

static void finish_bilateral(bool completed)
{
    if (!s_reg.result) {
        return;
    }
    strncpy(s_reg.result->regulation_type, "bilateral_tap", sizeof(s_reg.result->regulation_type) - 1);
    s_reg.result->regulation_type[sizeof(s_reg.result->regulation_type) - 1] = '\0';
    s_reg.result->duration_seconds = s_reg.duration_sec;
    s_reg.result->session_completed = completed;
    s_reg.result->steps_completed = 0;
    s_reg.result->rounds_completed = s_reg.bilateral_cycles;
    s_reg.result->pace_ms = s_reg.pace_ms;
    emit_action(completed ? CIRCE_REG_ACT_COMPLETE : CIRCE_REG_ACT_END);
    if (completed) {
        circe_voice_play_event(CIRCE_VOICE_EVENT_SESSION_COMPLETE);
    }
}

static void update_bilateral_visual(void)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_opa_t active = LV_OPA_70;
    lv_opa_t idle = LV_OPA_20;
    if (s_reg.left_dot) {
        lv_obj_set_style_bg_opa(s_reg.left_dot, s_reg.bilateral_left ? active : idle, 0);
        lv_obj_set_style_bg_color(s_reg.left_dot, circe_theme_color(p->focus), 0);
    }
    if (s_reg.right_dot) {
        lv_obj_set_style_bg_opa(s_reg.right_dot, s_reg.bilateral_left ? idle : active, 0);
        lv_obj_set_style_bg_color(s_reg.right_dot, circe_theme_color(p->accent_muted), 0);
    }
    if (s_reg.side_lbl) {
        lv_label_set_text(s_reg.side_lbl,
                          s_reg.bilateral_left ? circe_copy_get(CIRCE_PATTERN_REG_BILATERAL_LEFT)
                                               : circe_copy_get(CIRCE_PATTERN_REG_BILATERAL_RIGHT));
    }
}

static void bilateral_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_reg.active || s_reg.mode != REG_MODE_BILATERAL || s_reg.paused) {
        return;
    }
    s_reg.duration_sec++;
    if (!s_reg.bilateral_left) {
        s_reg.bilateral_cycles++;
    }
    s_reg.bilateral_left = !s_reg.bilateral_left;
    update_bilateral_visual();
}

static void step_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_reg.active || !mode_is_step(s_reg.mode)) {
        return;
    }
    s_reg.duration_sec++;
}

void circe_regulation_result_clear(circe_regulation_result_t *result)
{
    if (!result) {
        return;
    }
    memset(result, 0, sizeof(*result));
}

void circe_regulation_apply_to_entry(circe_entry_t *entry, const circe_regulation_result_t *result)
{
    if (!entry || !result) {
        return;
    }
    circe_entry_init_defaults(entry, CIRCE_ENTRY_MODE_REGULATION);
    entry->emotion_skipped = true;
    strncpy(entry->emotion, CIRCE_EMOTION_UNKNOWN, sizeof(entry->emotion) - 1);
    strncpy(entry->emotion_label, "UNKNOWN", sizeof(entry->emotion_label) - 1);
    entry->color_skipped = true;
    entry->color_hex[0] = '\0';
    strncpy(entry->color_label, "SKIPPED", sizeof(entry->color_label) - 1);
    strncpy(entry->color_source, "skipped", sizeof(entry->color_source) - 1);
    entry->training_ok = false;
    entry->private_locked = true;
    strncpy(entry->regulation_type, result->regulation_type, sizeof(entry->regulation_type) - 1);
    entry->regulation_type[sizeof(entry->regulation_type) - 1] = '\0';
    entry->regulation_rounds_completed = result->rounds_completed;
    entry->regulation_steps_completed = result->steps_completed;
    entry->regulation_duration_seconds = result->duration_seconds;
    entry->regulation_session_completed = result->session_completed;
    entry->has_regulation = true;
    snprintf(entry->summary, sizeof(entry->summary), "regulation %s %ds", result->regulation_type,
             result->duration_seconds);
}

void circe_regulation_destroy(void)
{
    if (s_reg.timer) {
        lv_timer_del(s_reg.timer);
        s_reg.timer = NULL;
    }
    if (s_reg.root) {
        lv_obj_del(s_reg.root);
        s_reg.root = NULL;
    }
    memset(&s_reg, 0, sizeof(s_reg));
}

bool circe_regulation_active(void)
{
    return s_reg.active;
}

void circe_regulation_poll(void)
{
    if (!s_reg.active) {
        return;
    }
    poll_encoder();
}

static void build_breath_ui(lv_obj_t *parent)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();

    s_reg.root = lv_obj_create(parent);
    lv_obj_set_size(s_reg.root, CIRCE_UI_REG_ROOT_W, CIRCE_UI_REG_BREATH_ROOT_H);
    lv_obj_align(s_reg.root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(s_reg.root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_reg.root, 0, 0);
    lv_obj_set_style_pad_all(s_reg.root, 0, 0);
    lv_obj_clear_flag(s_reg.root, LV_OBJ_FLAG_SCROLLABLE);

    s_reg.ring = lv_arc_create(s_reg.root);
    lv_obj_set_size(s_reg.ring, CIRCE_UI_REG_RING_SIZE, CIRCE_UI_REG_RING_SIZE);
    lv_obj_align(s_reg.ring, LV_ALIGN_TOP_MID, 0, CIRCE_UI_REG_RING_Y);
    lv_arc_set_mode(s_reg.ring, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(s_reg.ring, 0, 360);
    lv_arc_set_angles(s_reg.ring, 0, 360);
    lv_obj_set_style_arc_width(s_reg.ring, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_reg.ring, 0, LV_PART_INDICATOR);
    lv_obj_remove_style(s_reg.ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_reg.ring, LV_OBJ_FLAG_CLICKABLE);

    s_reg.orb = lv_obj_create(s_reg.root);
    lv_obj_set_size(s_reg.orb, CIRCE_UI_REG_ORB_SIZE, CIRCE_UI_REG_ORB_SIZE);
    lv_obj_align(s_reg.orb, LV_ALIGN_TOP_MID, 0, CIRCE_UI_REG_ORB_Y);
    lv_obj_set_style_radius(s_reg.orb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_reg.orb, 1, 0);
    lv_obj_set_style_border_color(s_reg.orb, circe_theme_color(p->focus), 0);
    lv_obj_clear_flag(s_reg.orb, LV_OBJ_FLAG_CLICKABLE);

    s_reg.phase_lbl = lv_label_create(s_reg.root);
    lv_obj_set_width(s_reg.phase_lbl, CIRCE_UI_REG_LABEL_W);
    lv_obj_align(s_reg.phase_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_REG_PHASE_Y);
    style_prompt_label(s_reg.phase_lbl);

    s_reg.count_lbl = lv_label_create(s_reg.root);
    lv_obj_set_width(s_reg.count_lbl, CIRCE_UI_REG_LABEL_W);
    lv_obj_align(s_reg.count_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_REG_COUNT_Y);
    circe_fonts_apply_label(s_reg.count_lbl, CIRCE_FONT_ROLE_HERO);
    lv_obj_set_style_text_color(s_reg.count_lbl, circe_theme_color(p->text), 0);
    lv_obj_set_style_text_align(s_reg.count_lbl, LV_TEXT_ALIGN_CENTER, 0);

    update_breath_labels();
}

static void build_step_ui(lv_obj_t *parent, bool sensory_dim)
{
    s_reg.root = lv_obj_create(parent);
    lv_obj_set_size(s_reg.root, CIRCE_UI_REG_ROOT_W, CIRCE_UI_REG_STEP_ROOT_H);
    lv_obj_align(s_reg.root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_border_width(s_reg.root, 0, 0);
    lv_obj_set_style_pad_all(s_reg.root, 0, 0);
    lv_obj_clear_flag(s_reg.root, LV_OBJ_FLAG_SCROLLABLE);
    if (sensory_dim) {
        lv_obj_set_style_bg_color(s_reg.root, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(s_reg.root, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_bg_opa(s_reg.root, LV_OPA_TRANSP, 0);
    }

    s_reg.step_lbl = lv_label_create(s_reg.root);
    lv_obj_set_width(s_reg.step_lbl, CIRCE_UI_REG_STEP_LABEL_W);
    lv_obj_align(s_reg.step_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_REG_STEP_TITLE_Y);
    circe_fonts_apply_label(s_reg.step_lbl, CIRCE_FONT_ROLE_CAPTION);
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(s_reg.step_lbl, circe_theme_color(p->accent_muted), 0);
    lv_obj_set_style_text_align(s_reg.step_lbl, LV_TEXT_ALIGN_CENTER, 0);

    s_reg.prompt_lbl = lv_label_create(s_reg.root);
    lv_obj_set_width(s_reg.prompt_lbl, 248);
    lv_obj_align(s_reg.prompt_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_REG_STEP_BODY_Y);
    lv_label_set_long_mode(s_reg.prompt_lbl, LV_LABEL_LONG_WRAP);
    style_prompt_label(s_reg.prompt_lbl);
    if (sensory_dim) {
        lv_obj_set_style_text_opa(s_reg.prompt_lbl, LV_OPA_70, 0);
    }
    update_step_prompt();
}

static void build_anchor_ui(lv_obj_t *parent)
{
    s_reg.prompt_keys = NULL;
    build_step_ui(parent, false);
}

static void build_bilateral_ui(lv_obj_t *parent)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();

    s_reg.root = lv_obj_create(parent);
    lv_obj_set_size(s_reg.root, CIRCE_UI_REG_ROOT_W, CIRCE_UI_REG_BILATERAL_ROOT_H);
    lv_obj_align(s_reg.root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(s_reg.root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_reg.root, 0, 0);
    lv_obj_set_style_pad_all(s_reg.root, 0, 0);
    lv_obj_clear_flag(s_reg.root, LV_OBJ_FLAG_SCROLLABLE);

    s_reg.left_dot = lv_obj_create(s_reg.root);
    lv_obj_set_size(s_reg.left_dot, CIRCE_UI_REG_DOT_SIZE, CIRCE_UI_REG_DOT_SIZE);
    lv_obj_align(s_reg.left_dot, LV_ALIGN_TOP_MID, -CIRCE_UI_REG_DOT_X_OFS, CIRCE_UI_REG_DOT_Y);
    lv_obj_set_style_radius(s_reg.left_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_reg.left_dot, 0, 0);

    s_reg.right_dot = lv_obj_create(s_reg.root);
    lv_obj_set_size(s_reg.right_dot, CIRCE_UI_REG_DOT_SIZE, CIRCE_UI_REG_DOT_SIZE);
    lv_obj_align(s_reg.right_dot, LV_ALIGN_TOP_MID, CIRCE_UI_REG_DOT_X_OFS, CIRCE_UI_REG_DOT_Y);
    lv_obj_set_style_radius(s_reg.right_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_reg.right_dot, 0, 0);

    s_reg.side_lbl = lv_label_create(s_reg.root);
    lv_obj_set_width(s_reg.side_lbl, CIRCE_UI_REG_LABEL_W);
    lv_obj_align(s_reg.side_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_REG_SIDE_LABEL_Y);
    style_prompt_label(s_reg.side_lbl);
    lv_obj_set_style_text_color(s_reg.side_lbl, circe_theme_color(p->focus), 0);

    s_reg.bilateral_left = true;
    s_reg.bilateral_cycles = 0;
    update_bilateral_visual();
}

void circe_regulation_breathing_start(circe_regulation_result_t *result, lv_obj_t *parent,
                                      circe_regulation_action_cb_t cb, void *ctx)
{
    circe_regulation_destroy();
    if (!result || !parent) {
        return;
    }
    memset(&s_reg, 0, sizeof(s_reg));
    s_reg.active = true;
    s_reg.mode = REG_MODE_BREATHING;
    s_reg.result = result;
    s_reg.action_cb = cb;
    s_reg.action_ctx = ctx;
    s_reg.max_rounds = REG_BREATH_ROUNDS;
    s_reg.round = 1;
    s_reg.phase = REG_PHASE_INHALE;
    s_reg.count = phase_start_count(REG_PHASE_INHALE);
    circe_regulation_result_clear(result);

    build_breath_ui(parent);
    s_reg.timer = lv_timer_create(breath_timer_cb, REG_TICK_MS, NULL);
    circe_voice_play_event(CIRCE_VOICE_EVENT_REGULATION_START);
    circe_voice_play_event(CIRCE_VOICE_EVENT_BREATHE_INHALE);
    ESP_LOGI(TAG, "breathing started: %d lvgl objects in module", 5);
}

void circe_regulation_body_anchor_start(circe_regulation_result_t *result, lv_obj_t *parent,
                                       circe_regulation_action_cb_t cb, void *ctx)
{
    circe_regulation_destroy();
    if (!result || !parent) {
        return;
    }
    memset(&s_reg, 0, sizeof(s_reg));
    s_reg.active = true;
    s_reg.mode = REG_MODE_BODY_ANCHOR;
    s_reg.result = result;
    s_reg.action_cb = cb;
    s_reg.action_ctx = ctx;
    s_reg.prompt_count = (int)(sizeof(s_anchor_prompts) / sizeof(s_anchor_prompts[0]));
    s_reg.prompt_idx = 0;
    circe_regulation_result_clear(result);

    build_anchor_ui(parent);
    s_reg.timer = lv_timer_create(step_timer_cb, REG_TICK_MS, NULL);
    circe_voice_play_event(CIRCE_VOICE_EVENT_REGULATION_START);
    ESP_LOGI(TAG, "body anchor started");
}

void circe_regulation_54321_start(circe_regulation_result_t *result, lv_obj_t *parent,
                                  circe_regulation_action_cb_t cb, void *ctx)
{
    circe_regulation_destroy();
    if (!result || !parent) {
        return;
    }
    memset(&s_reg, 0, sizeof(s_reg));
    s_reg.active = true;
    s_reg.mode = REG_MODE_54321;
    s_reg.result = result;
    s_reg.action_cb = cb;
    s_reg.action_ctx = ctx;
    s_reg.prompt_keys = s_54321_keys;
    s_reg.prompt_count = (int)(sizeof(s_54321_keys) / sizeof(s_54321_keys[0]));
    s_reg.prompt_idx = 0;
    circe_regulation_result_clear(result);

    build_step_ui(parent, false);
    s_reg.timer = lv_timer_create(step_timer_cb, REG_TICK_MS, NULL);
    circe_voice_play_event(CIRCE_VOICE_EVENT_REGULATION_START);
    ESP_LOGI(TAG, "54321 started");
}

void circe_regulation_sensory_start(circe_regulation_result_t *result, lv_obj_t *parent,
                                    circe_regulation_action_cb_t cb, void *ctx)
{
    circe_regulation_destroy();
    if (!result || !parent) {
        return;
    }
    memset(&s_reg, 0, sizeof(s_reg));
    s_reg.active = true;
    s_reg.mode = REG_MODE_SENSORY;
    s_reg.result = result;
    s_reg.action_cb = cb;
    s_reg.action_ctx = ctx;
    s_reg.prompt_keys = s_sensory_keys;
    s_reg.prompt_count = (int)(sizeof(s_sensory_keys) / sizeof(s_sensory_keys[0]));
    s_reg.prompt_idx = 0;
    s_reg.sensory_dim = true;
    circe_regulation_result_clear(result);

    build_step_ui(parent, true);
    s_reg.timer = lv_timer_create(step_timer_cb, REG_TICK_MS, NULL);
    ESP_LOGI(TAG, "sensory reset started");
}

void circe_regulation_bilateral_start(circe_regulation_result_t *result, lv_obj_t *parent,
                                       circe_regulation_action_cb_t cb, void *ctx)
{
    circe_regulation_destroy();
    if (!result || !parent) {
        return;
    }
    memset(&s_reg, 0, sizeof(s_reg));
    s_reg.active = true;
    s_reg.mode = REG_MODE_BILATERAL;
    s_reg.result = result;
    s_reg.action_cb = cb;
    s_reg.action_ctx = ctx;
    s_reg.pace_ms = REG_BILATERAL_PACE_DEFAULT;
    circe_regulation_result_clear(result);

    build_bilateral_ui(parent);
    s_reg.timer = lv_timer_create(bilateral_timer_cb, (uint32_t)s_reg.pace_ms, NULL);
    circe_voice_play_event(CIRCE_VOICE_EVENT_REGULATION_START);
    ESP_LOGI(TAG, "bilateral tap started");
}
