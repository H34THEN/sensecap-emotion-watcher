#include "circe_regulation.h"

#include <stdio.h>
#include <string.h>

#include "circe_fonts.h"
#include "circe_theme.h"
#include "esp_log.h"

#define REG_DOUBLE_PRESS_MS 450
#define REG_LONG_PRESS_MS   800
#define REG_BREATH_ROUNDS   3
#define REG_TICK_MS         1000

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
    lv_obj_t *prompt_lbl;
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

static void finish_body_anchor(bool completed)
{
    if (!s_reg.result) {
        return;
    }
    strncpy(s_reg.result->regulation_type, "body_anchor", sizeof(s_reg.result->regulation_type) - 1);
    s_reg.result->regulation_type[sizeof(s_reg.result->regulation_type) - 1] = '\0';
    s_reg.result->duration_seconds = s_reg.duration_sec;
    s_reg.result->session_completed = completed;
    s_reg.result->rounds_completed = completed ? 1 : 0;
    emit_action(completed ? CIRCE_REG_ACT_COMPLETE : CIRCE_REG_ACT_END);
}

static void update_anchor_prompt(void)
{
    if (!s_reg.prompt_lbl || s_reg.prompt_idx < 0 || s_reg.prompt_idx >= s_reg.prompt_count) {
        return;
    }
    char line[72];
    snprintf(line, sizeof(line), "> %s", s_anchor_prompts[s_reg.prompt_idx]);
    lv_label_set_text(s_reg.prompt_lbl, line);
}

static void anchor_next_prompt(void)
{
    if (s_reg.prompt_idx + 1 >= s_reg.prompt_count) {
        finish_body_anchor(true);
        return;
    }
    s_reg.prompt_idx++;
    update_anchor_prompt();
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

    if (s_reg.mode == REG_MODE_BODY_ANCHOR && data.enc_diff != 0) {
        s_reg.prompt_idx += data.enc_diff;
        if (s_reg.prompt_idx < 0) {
            s_reg.prompt_idx = 0;
        }
        if (s_reg.prompt_idx >= s_reg.prompt_count) {
            s_reg.prompt_idx = s_reg.prompt_count - 1;
        }
        update_anchor_prompt();
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
            } else if (s_reg.mode == REG_MODE_BODY_ANCHOR) {
                anchor_next_prompt();
            }
            s_reg.last_release_ms = now;
        }
    } else if (pressed && s_reg.enc_pressed && !s_reg.long_fired && (now - s_reg.press_start_ms) >= REG_LONG_PRESS_MS) {
        s_reg.long_fired = true;
        if (s_reg.mode == REG_MODE_BREATHING) {
            finish_breathing(false);
        } else if (s_reg.mode == REG_MODE_BODY_ANCHOR) {
            finish_body_anchor(false);
        }
    }
}

static void anchor_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_reg.active || s_reg.mode != REG_MODE_BODY_ANCHOR) {
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
    lv_obj_set_size(s_reg.root, 260, 200);
    lv_obj_align(s_reg.root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(s_reg.root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_reg.root, 0, 0);
    lv_obj_set_style_pad_all(s_reg.root, 0, 0);
    lv_obj_clear_flag(s_reg.root, LV_OBJ_FLAG_SCROLLABLE);

    s_reg.ring = lv_arc_create(s_reg.root);
    lv_obj_set_size(s_reg.ring, 120, 120);
    lv_obj_align(s_reg.ring, LV_ALIGN_TOP_MID, 0, 8);
    lv_arc_set_mode(s_reg.ring, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(s_reg.ring, 0, 360);
    lv_arc_set_angles(s_reg.ring, 0, 360);
    lv_obj_set_style_arc_width(s_reg.ring, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_reg.ring, 0, LV_PART_INDICATOR);
    lv_obj_remove_style(s_reg.ring, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(s_reg.ring, LV_OBJ_FLAG_CLICKABLE);

    s_reg.orb = lv_obj_create(s_reg.root);
    lv_obj_set_size(s_reg.orb, 48, 48);
    lv_obj_align(s_reg.orb, LV_ALIGN_TOP_MID, 0, 44);
    lv_obj_set_style_radius(s_reg.orb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_reg.orb, 1, 0);
    lv_obj_set_style_border_color(s_reg.orb, circe_theme_color(p->focus), 0);
    lv_obj_clear_flag(s_reg.orb, LV_OBJ_FLAG_CLICKABLE);

    s_reg.phase_lbl = lv_label_create(s_reg.root);
    lv_obj_set_width(s_reg.phase_lbl, 240);
    lv_obj_align(s_reg.phase_lbl, LV_ALIGN_TOP_MID, 0, 128);
    style_prompt_label(s_reg.phase_lbl);

    s_reg.count_lbl = lv_label_create(s_reg.root);
    lv_obj_set_width(s_reg.count_lbl, 240);
    lv_obj_align(s_reg.count_lbl, LV_ALIGN_TOP_MID, 0, 158);
    circe_fonts_apply_label(s_reg.count_lbl, CIRCE_FONT_ROLE_HERO);
    lv_obj_set_style_text_color(s_reg.count_lbl, circe_theme_color(p->text), 0);
    lv_obj_set_style_text_align(s_reg.count_lbl, LV_TEXT_ALIGN_CENTER, 0);

    update_breath_labels();
}

static void build_anchor_ui(lv_obj_t *parent)
{
    s_reg.root = lv_obj_create(parent);
    lv_obj_set_size(s_reg.root, 260, 120);
    lv_obj_align(s_reg.root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(s_reg.root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_reg.root, 0, 0);
    lv_obj_set_style_pad_all(s_reg.root, 0, 0);
    lv_obj_clear_flag(s_reg.root, LV_OBJ_FLAG_SCROLLABLE);

    s_reg.prompt_lbl = lv_label_create(s_reg.root);
    lv_obj_set_width(s_reg.prompt_lbl, 248);
    lv_obj_align(s_reg.prompt_lbl, LV_ALIGN_TOP_MID, 0, 16);
    lv_label_set_long_mode(s_reg.prompt_lbl, LV_LABEL_LONG_WRAP);
    style_prompt_label(s_reg.prompt_lbl);
    update_anchor_prompt();
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
    s_reg.timer = lv_timer_create(anchor_timer_cb, REG_TICK_MS, NULL);
    ESP_LOGI(TAG, "body anchor started: %d lvgl objects in module", 2);
}
