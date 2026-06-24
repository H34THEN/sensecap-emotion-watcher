#include "circe_ui.h"

#include <stdio.h>
#include <string.h>

#include "circe_copy.h"
#include "circe_entry_modes.h"
#include "circe_storage.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "circe_ui";

static circe_conversation_engine_t *s_engine = NULL;
static lv_obj_t *s_scr = NULL;
static lv_obj_t *s_prompt = NULL;
static lv_obj_t *s_content = NULL;
static lv_obj_t *s_slider = NULL;
static lv_obj_t *s_status = NULL;

static char s_review_id[CIRCE_MAX_ID];

static void show_message(circe_pattern_key_t key)
{
    if (s_prompt) {
        lv_label_set_text(s_prompt, circe_copy_get(key));
    }
}

static void clear_content(void)
{
    if (s_content) {
        lv_obj_clean(s_content);
    }
}

static void go_step(circe_flow_step_t step)
{
    if (!s_engine) {
        return;
    }
    s_engine->step = step;
    circe_ui_show_step(step);
}

static bool persist_entry(void)
{
    if (!s_engine) {
        return false;
    }
    s_engine->draft.training_ok = false;
    s_engine->draft.private_locked = true;
    strncpy(s_engine->draft.emotion, CIRCE_EMOTION_UNKNOWN, sizeof(s_engine->draft.emotion) - 1);
    if (!circe_entry_save_json_atomic(&s_engine->draft)) {
        show_message(CIRCE_PATTERN_ERROR_SAVE_FAILED);
        return false;
    }
    if (!circe_entry_index_insert(&s_engine->draft)) {
        show_message(CIRCE_PATTERN_ERROR_SAVE_FAILED);
        return false;
    }
    strncpy(s_review_id, s_engine->draft.id, sizeof(s_review_id) - 1);
    return true;
}

static void btn_event_cb(lv_event_t *e)
{
    const char *id = (const char *)lv_event_get_user_data(e);
    if (!id || !s_engine) {
        return;
    }

    if (strcmp(id, "body") == 0) {
        circe_conversation_start_body_only(s_engine);
        go_step(CIRCE_FLOW_BODY_AREA);
    } else if (strcmp(id, "quick") == 0) {
        circe_conversation_start_quick(s_engine);
        go_step(CIRCE_FLOW_QUICK_PICK);
    } else if (strcmp(id, "review") == 0) {
        if (circe_storage_get_latest_entry_id(s_review_id, sizeof(s_review_id))) {
            circe_entry_load(s_review_id, &s_engine->draft);
            go_step(CIRCE_FLOW_REVIEW);
        } else if (s_status) {
            lv_label_set_text(s_status, "No entries yet.");
        }
    } else if (strcmp(id, "more") == 0) {
        go_step(CIRCE_FLOW_DIAGNOSTICS);
    } else if (strcmp(id, "save") == 0) {
        if (persist_entry()) {
            show_message(CIRCE_PATTERN_SAVE_CONFIRMED);
            if (s_status) {
                lv_label_set_text(s_status, circe_copy_get(CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE));
            }
            go_step(CIRCE_FLOW_HOME);
        }
    } else if (strcmp(id, "skip_color") == 0) {
        go_step(CIRCE_FLOW_SAVE_CONFIRM);
    } else if (strcmp(id, "next_intensity") == 0) {
        if (s_slider) {
            s_engine->draft.intensity = lv_slider_get_value(s_slider);
        }
        go_step(CIRCE_FLOW_COLOR_OPTIONAL);
    } else if (strcmp(id, "delete") == 0) {
        go_step(CIRCE_FLOW_DELETE_CONFIRM);
    } else if (strcmp(id, "delete_yes") == 0) {
        if (s_review_id[0] && circe_entry_delete_hard(s_review_id)) {
            s_review_id[0] = '\0';
            show_message(CIRCE_PATTERN_DELETE_DONE);
            go_step(CIRCE_FLOW_HOME);
        }
    } else if (strcmp(id, "home") == 0) {
        go_step(CIRCE_FLOW_HOME);
    } else if (strcmp(id, "rebuild") == 0) {
        circe_ui_run_rebuild_test();
    } else if (strncmp(id, "area:", 5) == 0) {
        if (s_engine->draft.body_area_count < CIRCE_MAX_BODY_AREAS) {
            int idx = s_engine->draft.body_area_count;
            strncpy(s_engine->draft.body_areas[idx], id + 5, sizeof(s_engine->draft.body_areas[idx]) - 1);
            s_engine->draft.body_area_count++;
        }
        go_step(CIRCE_FLOW_BODY_SENSATION);
    } else if (strncmp(id, "sen:", 4) == 0) {
        if (s_engine->draft.body_sensation_count < CIRCE_MAX_BODY_SENSATIONS) {
            int idx = s_engine->draft.body_sensation_count;
            strncpy(s_engine->draft.body_sensations[idx], id + 4, sizeof(s_engine->draft.body_sensations[idx]) - 1);
            s_engine->draft.body_sensation_count++;
        }
        go_step(CIRCE_FLOW_INTENSITY);
    } else if (strncmp(id, "color:", 6) == 0) {
        strncpy(s_engine->draft.color_hex, id + 6, sizeof(s_engine->draft.color_hex) - 1);
        go_step(CIRCE_FLOW_SAVE_CONFIRM);
    } else if (strncmp(id, "quick:", 6) == 0) {
        int preset = id[6] - '0';
        circe_entry_modes_apply_quick_preset(&s_engine->draft, preset);
        if (persist_entry()) {
            show_message(CIRCE_PATTERN_QUICK_SAVED);
            if (s_status) {
                lv_label_set_text(s_status, circe_copy_get(CIRCE_PATTERN_QUICK_ADD_LATER));
            }
            go_step(CIRCE_FLOW_HOME);
        }
    }
}

static lv_obj_t *add_btn(const char *label, const char *id)
{
    lv_obj_t *btn = lv_btn_create(s_content);
    lv_obj_set_width(btn, LV_PCT(90));
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *)id);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
    return btn;
}

void circe_ui_set_engine(circe_conversation_engine_t *engine)
{
    s_engine = engine;
}

void circe_ui_init(void)
{
    s_scr = lv_obj_create(NULL);
    lv_scr_load(s_scr);

    s_prompt = lv_label_create(s_scr);
    lv_obj_set_width(s_prompt, LV_PCT(95));
    lv_label_set_long_mode(s_prompt, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_prompt, LV_ALIGN_TOP_MID, 0, 8);

    s_content = lv_obj_create(s_scr);
    lv_obj_set_size(s_content, LV_PCT(100), 280);
    lv_obj_align(s_content, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_flex_flow(s_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_content, 6, 0);

    s_status = lv_label_create(s_scr);
    lv_obj_set_width(s_status, LV_PCT(95));
    lv_label_set_long_mode(s_status, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_status, LV_ALIGN_BOTTOM_MID, 0, -4);
}

void circe_ui_show_step(circe_flow_step_t step)
{
    if (!s_engine) {
        return;
    }
    clear_content();
    s_slider = NULL;
    show_message(circe_conversation_prompt_for_step(s_engine));

    switch (step) {
    case CIRCE_FLOW_HOME:
        show_message(CIRCE_PATTERN_GREET_FIRST_TODAY);
        add_btn(circe_copy_get(CIRCE_PATTERN_HOME_BODY), "body");
        add_btn(circe_copy_get(CIRCE_PATTERN_HOME_QUICK), "quick");
        add_btn(circe_copy_get(CIRCE_PATTERN_HOME_REVIEW), "review");
        add_btn(circe_copy_get(CIRCE_PATTERN_HOME_MORE), "more");
        if (!s_engine->storage_ready) {
            show_message(CIRCE_PATTERN_ERROR_STORAGE_MISSING);
        }
        break;

    case CIRCE_FLOW_BODY_AREA:
        for (int i = 0; i < circe_body_area_count; i++) {
            static char ids[20][32];
            snprintf(ids[i], sizeof(ids[i]), "area:%s", circe_body_areas[i]);
            add_btn(circe_body_areas[i], ids[i]);
        }
        break;

    case CIRCE_FLOW_BODY_SENSATION:
        for (int i = 0; i < circe_body_sensation_count; i++) {
            static char ids[24][40];
            snprintf(ids[i], sizeof(ids[i]), "sen:%s", circe_body_sensations[i]);
            add_btn(circe_body_sensations[i], ids[i]);
        }
        break;

    case CIRCE_FLOW_INTENSITY: {
        s_slider = lv_slider_create(s_content);
        lv_slider_set_range(s_slider, 1, 10);
        lv_slider_set_value(s_slider, s_engine->draft.intensity, LV_ANIM_OFF);
        lv_obj_set_width(s_slider, LV_PCT(80));
        add_btn("Continue", "next_intensity");
        break;
    }

    case CIRCE_FLOW_COLOR_OPTIONAL:
        for (int i = 0; i < circe_quick_color_count; i++) {
            static char color_ids[4][24];
            snprintf(color_ids[i], sizeof(color_ids[i]), "color:%s", circe_quick_colors[i]);
            add_btn(circe_quick_colors[i], color_ids[i]);
        }
        add_btn("Skip color", "skip_color");
        break;

    case CIRCE_FLOW_SAVE_CONFIRM:
        show_message(CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE);
        if (persist_entry()) {
            show_message(CIRCE_PATTERN_SAVE_CONFIRMED);
            go_step(CIRCE_FLOW_HOME);
        }
        break;

    case CIRCE_FLOW_REVIEW: {
        char line[256];
        snprintf(line, sizeof(line), "Mode: %s\nEmotion: %s\nAreas: %d  Sensations: %d\nColor: %s  Intensity: %d\nPrivate: yes",
                 circe_entry_mode_str(s_engine->draft.entry_mode), s_engine->draft.emotion, s_engine->draft.body_area_count,
                 s_engine->draft.body_sensation_count, s_engine->draft.color_hex, s_engine->draft.intensity);
        lv_obj_t *lbl = lv_label_create(s_content);
        lv_label_set_text(lbl, line);
        add_btn("Delete", "delete");
        add_btn("Home", "home");
        break;
    }

    case CIRCE_FLOW_DELETE_CONFIRM:
        add_btn("Yes, delete", "delete_yes");
        add_btn("Cancel", "home");
        break;

    case CIRCE_FLOW_QUICK_PICK:
        for (int i = 0; i < CIRCE_QUICK_PRESET_COUNT; i++) {
            static char ids[4][16];
            snprintf(ids[i], sizeof(ids[i]), "quick:%d", i);
            add_btn(circe_quick_presets[i].label, ids[i]);
        }
        break;

    case CIRCE_FLOW_DIAGNOSTICS: {
        circe_storage_health_t h;
        circe_storage_health_check(&h);
        char line[160];
        snprintf(line, sizeof(line), "SD: %s  Index: %s\nEntries: %d\n%s", h.sd_mounted ? "ok" : "no", h.index_open ? "ok" : "no",
                 h.entry_count, h.last_error);
        lv_obj_t *lbl = lv_label_create(s_content);
        lv_label_set_text(lbl, line);
        add_btn("Rebuild index", "rebuild");
        add_btn("Run self test", "rebuild");
        add_btn("Home", "home");
        break;
    }

    default:
        break;
    }
}

bool circe_ui_save_draft(void)
{
    return persist_entry();
}

bool circe_ui_delete_latest(void)
{
    if (!s_review_id[0]) {
        return false;
    }
    return circe_entry_delete_hard(s_review_id);
}

void circe_ui_run_rebuild_test(void)
{
    int count = 0;
    circe_rebuild_index_from_json(&count);
    circe_storage_run_self_test();
    if (s_status) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Rebuild: %d entries", count);
        lv_label_set_text(s_status, buf);
    }
}
