#include "circe_ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circe_copy.h"
#include "circe_entry_modes.h"
#include "circe_storage.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#define MAX_BTN_IDS 40
#define BTN_H         36
#define BTN_PAD       4

static circe_conversation_engine_t *s_engine = NULL;
static lv_obj_t *s_scr = NULL;
static lv_obj_t *s_prompt = NULL;
static lv_obj_t *s_content = NULL;
static lv_obj_t *s_scroll = NULL;
static lv_obj_t *s_slider = NULL;
static lv_obj_t *s_status = NULL;
static lv_obj_t *s_strand_row = NULL;
static lv_group_t *s_group = NULL;

static char s_review_id[CIRCE_MAX_ID];
static char s_btn_ids[MAX_BTN_IDS][48];
static int s_btn_id_next = 0;

static void btn_event_cb(lv_event_t *e);

static uint32_t parse_hex_color(const char *hex)
{
    if (!hex || hex[0] != '#') {
        return 0x808080;
    }
    return (uint32_t)strtoul(hex + 1, NULL, 16);
}

static const char *alloc_btn_id(const char *fmt, ...)
{
    if (s_btn_id_next >= MAX_BTN_IDS) {
        return "overflow";
    }
    char *buf = s_btn_ids[s_btn_id_next++];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(s_btn_ids[0]), fmt, args);
    va_end(args);
    return buf;
}

static void show_message(circe_pattern_key_t key)
{
    if (s_prompt) {
        lv_label_set_text(s_prompt, circe_copy_get(key));
    }
}

static void clear_content(void)
{
    s_btn_id_next = 0;
    s_slider = NULL;
    s_scroll = NULL;
    if (s_content) {
        lv_obj_clean(s_content);
    }
    if (s_group) {
        lv_group_remove_all_objs(s_group);
    }
}

static void attach_encoder_indev(void)
{
    if (!s_group) {
        return;
    }
    lv_indev_t *cur_drv = NULL;
    for (;;) {
        cur_drv = lv_indev_get_next(cur_drv);
        if (!cur_drv) {
            break;
        }
        if (cur_drv->driver->type == LV_INDEV_TYPE_ENCODER) {
            lv_indev_set_group(cur_drv, s_group);
        }
    }
}

static void setup_encoder_group(void)
{
    if (!s_group) {
        s_group = lv_group_get_default();
        if (!s_group) {
            s_group = lv_group_create();
            lv_group_set_default(s_group);
        }
    }
    attach_encoder_indev();
}

static lv_obj_t *create_scroll_panel(void)
{
    s_scroll = lv_obj_create(s_content);
    lv_obj_set_size(s_scroll, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(s_scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(s_scroll, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_scroll, LV_DIR_VER);
    lv_obj_set_style_pad_row(s_scroll, BTN_PAD, 0);
    lv_obj_set_style_pad_all(s_scroll, BTN_PAD, 0);
    lv_obj_set_style_border_width(s_scroll, 0, 0);
    return s_scroll;
}

static lv_obj_t *btn_parent(void)
{
    return s_scroll ? s_scroll : s_content;
}

static lv_obj_t *add_btn(const char *label, const char *id)
{
    lv_obj_t *btn = lv_btn_create(btn_parent());
    lv_obj_set_width(btn, LV_PCT(92));
    lv_obj_set_height(btn, BTN_H);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *)id);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
    if (s_group) {
        lv_group_add_obj(s_group, btn);
    }
    return btn;
}

static void add_back_btn(circe_flow_step_t target)
{
    add_btn(circe_copy_get(CIRCE_PATTERN_NAV_BACK), alloc_btn_id("nav:%d", (int)target));
}

static void go_step(circe_flow_step_t step);

static bool persist_entry(void)
{
    if (!s_engine) {
        return false;
    }
    s_engine->draft.training_ok = false;
    s_engine->draft.private_locked = true;
    strncpy(s_engine->draft.emotion, CIRCE_EMOTION_UNKNOWN, sizeof(s_engine->draft.emotion) - 1);
    s_engine->draft.emotion[sizeof(s_engine->draft.emotion) - 1] = '\0';
    if (s_engine->editing_existing) {
        if (!circe_entry_update(&s_engine->draft)) {
            show_message(CIRCE_PATTERN_ERROR_SAVE_FAILED);
            return false;
        }
    } else {
        if (!circe_entry_save_json_atomic(&s_engine->draft)) {
            show_message(CIRCE_PATTERN_ERROR_SAVE_FAILED);
            return false;
        }
        if (!circe_entry_index_insert(&s_engine->draft)) {
            show_message(CIRCE_PATTERN_ERROR_SAVE_FAILED);
            return false;
        }
    }
    strncpy(s_review_id, s_engine->draft.id, sizeof(s_review_id) - 1);
    s_review_id[sizeof(s_review_id) - 1] = '\0';
    s_engine->editing_existing = false;
    return true;
}

static void refresh_strand_row(void)
{
    if (!s_strand_row) {
        return;
    }
    lv_obj_clean(s_strand_row);
    lv_obj_t *void_block = lv_obj_create(s_strand_row);
    lv_obj_set_size(void_block, 18, 18);
    lv_obj_set_style_bg_color(void_block, lv_color_hex(0x2D3748), 0);
    lv_obj_set_style_border_width(void_block, 1, 0);
    lv_obj_set_style_border_color(void_block, lv_color_hex(0x4A5568), 0);
    lv_obj_set_style_radius(void_block, 3, 0);
}

void circe_ui_refresh_strand_from_storage(void)
{
    if (!s_strand_row) {
        return;
    }
    circe_strand_block_t blocks[16];
    int count = 0;
    circe_storage_today_strand(blocks, 16, &count);

    if (!lvgl_port_lock(1000)) {
        return;
    }
    lv_obj_clean(s_strand_row);
    if (count == 0) {
        refresh_strand_row();
    } else {
        for (int i = 0; i < count; i++) {
            lv_obj_t *block = lv_obj_create(s_strand_row);
            lv_obj_set_size(block, 18, 18);
            lv_obj_set_style_bg_color(block, lv_color_hex(parse_hex_color(blocks[i].color_hex)), 0);
            lv_obj_set_style_border_width(block, 0, 0);
            lv_obj_set_style_radius(block, 3, 0);
        }
    }
    lvgl_port_unlock();
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
    } else if (strcmp(id, "strand") == 0) {
        go_step(CIRCE_FLOW_STRAND);
    } else if (strcmp(id, "save") == 0) {
        if (persist_entry()) {
            go_step(CIRCE_FLOW_SAVE_DONE);
        }
    } else if (strcmp(id, "skip_color") == 0) {
        if (persist_entry()) {
            go_step(CIRCE_FLOW_SAVE_DONE);
        }
    } else if (strcmp(id, "next_intensity") == 0) {
        if (s_slider) {
            s_engine->draft.intensity = lv_slider_get_value(s_slider);
        }
        go_step(CIRCE_FLOW_BODY_ADD_MORE);
    } else if (strcmp(id, "add_sensation") == 0) {
        go_step(CIRCE_FLOW_BODY_AREA);
    } else if (strcmp(id, "continue_save") == 0) {
        go_step(CIRCE_FLOW_COLOR_OPTIONAL);
    } else if (strcmp(id, "delete") == 0) {
        go_step(CIRCE_FLOW_DELETE_CONFIRM);
    } else if (strcmp(id, "delete_yes") == 0) {
        if (s_review_id[0] && circe_entry_delete_hard(s_review_id)) {
            s_review_id[0] = '\0';
            show_message(CIRCE_PATTERN_DELETE_DONE);
            go_step(CIRCE_FLOW_HOME);
        }
    } else if (strcmp(id, "edit") == 0) {
        go_step(CIRCE_FLOW_EDIT);
    } else if (strcmp(id, "edit_color") == 0) {
        s_engine->editing_existing = true;
        go_step(CIRCE_FLOW_EDIT_COLOR);
    } else if (strcmp(id, "edit_add_sensation") == 0) {
        s_engine->editing_existing = true;
        go_step(CIRCE_FLOW_BODY_AREA);
    } else if (strcmp(id, "home") == 0) {
        s_engine->editing_existing = false;
        go_step(CIRCE_FLOW_HOME);
    } else if (strcmp(id, "rebuild") == 0) {
        int count = 0;
        circe_rebuild_index_from_json(&count);
        if (s_status) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Rebuild: %d entries", count);
            lv_label_set_text(s_status, buf);
        }
        go_step(CIRCE_FLOW_DIAGNOSTICS);
    } else if (strcmp(id, "selftest") == 0) {
        bool ok = circe_storage_run_self_test();
        if (s_status) {
            lv_label_set_text(s_status, ok ? "Self-test passed" : "Self-test failed");
        }
        go_step(CIRCE_FLOW_DIAGNOSTICS);
    } else if (strncmp(id, "nav:", 4) == 0) {
        go_step((circe_flow_step_t)atoi(id + 4));
    } else if (strncmp(id, "area:", 5) == 0) {
        if (s_engine->draft.body_area_count < CIRCE_MAX_BODY_AREAS) {
            int idx = s_engine->draft.body_area_count;
            strncpy(s_engine->draft.body_areas[idx], id + 5, sizeof(s_engine->draft.body_areas[idx]) - 1);
            s_engine->draft.body_areas[idx][sizeof(s_engine->draft.body_areas[idx]) - 1] = '\0';
            s_engine->draft.body_area_count++;
        }
        go_step(CIRCE_FLOW_BODY_SENSATION);
    } else if (strncmp(id, "sen:", 4) == 0) {
        if (s_engine->draft.body_sensation_count < CIRCE_MAX_BODY_SENSATIONS) {
            int idx = s_engine->draft.body_sensation_count;
            strncpy(s_engine->draft.body_sensations[idx], id + 4, sizeof(s_engine->draft.body_sensations[idx]) - 1);
            s_engine->draft.body_sensations[idx][sizeof(s_engine->draft.body_sensations[idx]) - 1] = '\0';
            s_engine->draft.body_sensation_count++;
        }
        if (s_engine->editing_existing) {
            if (persist_entry()) {
                show_message(CIRCE_PATTERN_EDIT_SAVED);
                go_step(CIRCE_FLOW_REVIEW);
            }
        } else {
            go_step(CIRCE_FLOW_INTENSITY);
        }
    } else if (strncmp(id, "color:", 6) == 0) {
        strncpy(s_engine->draft.color_hex, id + 6, sizeof(s_engine->draft.color_hex) - 1);
        s_engine->draft.color_hex[sizeof(s_engine->draft.color_hex) - 1] = '\0';
        if (s_engine->editing_existing) {
            if (persist_entry()) {
                show_message(CIRCE_PATTERN_EDIT_SAVED);
                go_step(CIRCE_FLOW_REVIEW);
            }
        } else if (persist_entry()) {
            go_step(CIRCE_FLOW_SAVE_DONE);
        }
    } else if (strncmp(id, "quick:", 6) == 0) {
        int preset = id[6] - '0';
        circe_entry_modes_apply_quick_preset(&s_engine->draft, preset);
        if (persist_entry()) {
            show_message(CIRCE_PATTERN_QUICK_SAVED);
            if (s_status) {
                lv_label_set_text(s_status, circe_copy_get(CIRCE_PATTERN_QUICK_ADD_LATER));
            }
            go_step(CIRCE_FLOW_SAVE_DONE);
        }
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

void circe_ui_set_engine(circe_conversation_engine_t *engine)
{
    s_engine = engine;
}

void circe_ui_init(void)
{
    setup_encoder_group();

    s_scr = lv_obj_create(NULL);
    lv_scr_load(s_scr);

    s_strand_row = lv_obj_create(s_scr);
    lv_obj_set_size(s_strand_row, LV_PCT(95), 24);
    lv_obj_align(s_strand_row, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_flex_flow(s_strand_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_strand_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_strand_row, 4, 0);
    lv_obj_set_style_bg_opa(s_strand_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_strand_row, 0, 0);
    lv_obj_clear_flag(s_strand_row, LV_OBJ_FLAG_SCROLLABLE);

    s_prompt = lv_label_create(s_scr);
    lv_obj_set_width(s_prompt, LV_PCT(95));
    lv_label_set_long_mode(s_prompt, LV_LABEL_LONG_WRAP);
    lv_obj_align(s_prompt, LV_ALIGN_TOP_MID, 0, 32);

    s_content = lv_obj_create(s_scr);
    lv_obj_set_size(s_content, LV_PCT(100), 250);
    lv_obj_align(s_content, LV_ALIGN_BOTTOM_MID, 0, -44);
    lv_obj_set_style_pad_all(s_content, BTN_PAD, 0);
    lv_obj_set_style_border_width(s_content, 0, 0);

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
    setup_encoder_group();
    show_message(circe_conversation_prompt_for_step(s_engine));

    switch (step) {
    case CIRCE_FLOW_HOME:
        refresh_strand_row();
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
        create_scroll_panel();
        for (int i = 0; i < circe_body_area_count; i++) {
            add_btn(circe_body_areas[i], alloc_btn_id("area:%s", circe_body_areas[i]));
        }
        add_back_btn(s_engine->editing_existing ? CIRCE_FLOW_EDIT : CIRCE_FLOW_HOME);
        break;

    case CIRCE_FLOW_BODY_SENSATION:
        create_scroll_panel();
        for (int i = 0; i < circe_body_sensation_count; i++) {
            add_btn(circe_body_sensations[i], alloc_btn_id("sen:%s", circe_body_sensations[i]));
        }
        add_back_btn(CIRCE_FLOW_BODY_AREA);
        break;

    case CIRCE_FLOW_INTENSITY:
        s_slider = lv_slider_create(s_content);
        lv_slider_set_range(s_slider, 1, 10);
        lv_slider_set_value(s_slider, s_engine->draft.intensity, LV_ANIM_OFF);
        lv_obj_set_width(s_slider, LV_PCT(85));
        if (s_group) {
            lv_group_add_obj(s_group, s_slider);
        }
        add_btn("Continue", "next_intensity");
        add_back_btn(CIRCE_FLOW_BODY_SENSATION);
        break;

    case CIRCE_FLOW_BODY_ADD_MORE:
        add_btn("Add another sensation", "add_sensation");
        add_btn(circe_copy_get(CIRCE_PATTERN_BODY_CONTINUE), "continue_save");
        add_back_btn(CIRCE_FLOW_INTENSITY);
        break;

    case CIRCE_FLOW_COLOR_OPTIONAL:
        create_scroll_panel();
        for (int i = 0; i < circe_quick_color_count; i++) {
            add_btn(circe_quick_colors[i], alloc_btn_id("color:%s", circe_quick_colors[i]));
        }
        add_btn("Skip color", "skip_color");
        add_back_btn(CIRCE_FLOW_BODY_ADD_MORE);
        break;

    case CIRCE_FLOW_SAVE_DONE:
        show_message(CIRCE_PATTERN_SAVE_CONFIRMED);
        if (s_status) {
            lv_label_set_text(s_status, circe_copy_get(CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE));
        }
        add_btn("Home", "home");
        refresh_strand_row();
        break;

    case CIRCE_FLOW_REVIEW: {
        char line[320];
        snprintf(line, sizeof(line),
                 "Mode: %s\nEmotion: %s\nAreas: %d  Sensations: %d\nColor: %s  Intensity: %d\nRev: %d  Private: yes",
                 circe_entry_mode_str(s_engine->draft.entry_mode), s_engine->draft.emotion, s_engine->draft.body_area_count,
                 s_engine->draft.body_sensation_count, s_engine->draft.color_hex, s_engine->draft.intensity,
                 s_engine->draft.revision);
        lv_obj_t *lbl = lv_label_create(s_content);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, LV_PCT(92));
        lv_label_set_text(lbl, line);
        add_btn("Edit", "edit");
        add_btn("Delete", "delete");
        add_btn("Home", "home");
        break;
    }

    case CIRCE_FLOW_DELETE_CONFIRM:
        add_btn("Yes, delete", "delete_yes");
        add_btn(circe_copy_get(CIRCE_PATTERN_NAV_CANCEL), alloc_btn_id("nav:%d", CIRCE_FLOW_REVIEW));
        break;

    case CIRCE_FLOW_EDIT:
        add_btn(circe_copy_get(CIRCE_PATTERN_EDIT_COLOR), "edit_color");
        add_btn(circe_copy_get(CIRCE_PATTERN_EDIT_ADD_SENSATION), "edit_add_sensation");
        add_back_btn(CIRCE_FLOW_REVIEW);
        break;

    case CIRCE_FLOW_EDIT_COLOR:
        create_scroll_panel();
        for (int i = 0; i < circe_quick_color_count; i++) {
            add_btn(circe_quick_colors[i], alloc_btn_id("color:%s", circe_quick_colors[i]));
        }
        add_back_btn(CIRCE_FLOW_EDIT);
        break;

    case CIRCE_FLOW_QUICK_PICK:
        create_scroll_panel();
        for (int i = 0; i < CIRCE_QUICK_PRESET_COUNT; i++) {
            add_btn(circe_quick_presets[i].label, alloc_btn_id("quick:%d", i));
        }
        add_back_btn(CIRCE_FLOW_HOME);
        break;

    case CIRCE_FLOW_STRAND: {
        circe_strand_block_t blocks[16];
        int count = 0;
        circe_storage_today_strand(blocks, 16, &count);
        char line[96];
        snprintf(line, sizeof(line), "%d entries today", count);
        lv_obj_t *lbl = lv_label_create(s_content);
        lv_label_set_text(lbl, line);
        lv_obj_t *row = lv_obj_create(s_content);
        lv_obj_set_size(row, LV_PCT(95), 28);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 4, 0);
        for (int i = 0; i < count; i++) {
            lv_obj_t *block = lv_obj_create(row);
            lv_obj_set_size(block, 22, 22);
            lv_obj_set_style_bg_color(block, lv_color_hex(parse_hex_color(blocks[i].color_hex)), 0);
            lv_obj_set_style_radius(block, 3, 0);
        }
        if (count == 0) {
            lv_obj_t *block = lv_obj_create(row);
            lv_obj_set_size(block, 22, 22);
            lv_obj_set_style_bg_color(block, lv_color_hex(0x2D3748), 0);
        }
        add_btn("Home", "home");
        break;
    }

    case CIRCE_FLOW_DIAGNOSTICS: {
        circe_storage_health_t h;
        circe_storage_health_check(&h);
        char line[280];
        snprintf(line, sizeof(line),
                 "SD: %s  Index: %s\nEntries: %d\nLast error: %s\n\n%s",
                 h.sd_mounted ? "yes" : "no", h.index_open ? "ok" : "no", h.entry_count,
                 h.last_error[0] ? h.last_error : "none", circe_copy_get(CIRCE_PATTERN_PRIVACY_STANDALONE));
        lv_obj_t *lbl = lv_label_create(s_content);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, LV_PCT(92));
        lv_label_set_text(lbl, line);
        add_btn("Rebuild index", "rebuild");
        add_btn("Run self test", "selftest");
        add_btn("Today strand", "strand");
        add_btn("Home", "home");
        break;
    }

    default:
        break;
    }

    attach_encoder_indev();
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
    if (s_status) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Rebuild: %d entries", count);
        lv_label_set_text(s_status, buf);
    }
}
