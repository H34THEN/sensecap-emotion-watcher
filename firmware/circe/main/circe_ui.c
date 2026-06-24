#include "circe_ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circe_actions.h"
#include "circe_copy.h"
#include "circe_entry_modes.h"
#include "circe_fonts.h"
#include "circe_hud.h"
#include "circe_index.h"
#include "circe_save.h"
#include "circe_storage.h"
#include "circe_strand_cache.h"
#include "circe_theme.h"
#include "circe_worker.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#define MAX_BTN_IDS       40
#define CIRCE_UI_COL_W    240
#define COL_W             CIRCE_UI_COL_W
#define COL_PAD           8
#define STRAND_MAX        8
#define STRAND_CX         206
#define STRAND_CY           88
#define STRAND_R            90
#define STRAND_A0_DEG       35
#define STRAND_A1_DEG      145
#define STRAND_BLOCK         12
#define COL_H                240
#define CIRCE_MSG_NONE       (-1)

static const char *TAG = "circe_ui";

static int strand_angle_for(int index, int total)
{
    if (total <= 1) {
        return 90;
    }
    return STRAND_A0_DEG + (index * (STRAND_A1_DEG - STRAND_A0_DEG)) / (total - 1);
}

static circe_conversation_engine_t *s_engine = NULL;
static lv_obj_t *s_scr = NULL;
static circe_hud_t s_hud;
static lv_obj_t *s_content = NULL;
static lv_obj_t *s_column = NULL;
static lv_obj_t *s_scroll = NULL;
static lv_obj_t *s_slider = NULL;
static lv_obj_t *s_strand_arc = NULL;
static lv_group_t *s_group = NULL;
static lv_obj_t *s_focus_body = NULL;
static lv_obj_t *s_more_btn = NULL;
static circe_theme_id_t s_appearance_pick = CIRCE_THEME_CLASSIC;

static char s_review_id[CIRCE_MAX_ID];
static char s_btn_ids[MAX_BTN_IDS][48];
static int s_btn_id_next = 0;
static int s_btn_stack_idx = 0;
static circe_strand_block_t s_home_blocks[STRAND_MAX];
static int s_home_block_count = 0;

static void btn_event_cb(lv_event_t *e);
static void refresh_strand_arc_from_blocks(circe_strand_block_t *blocks, int count);

static void strand_note_saved_color(const char *color_hex)
{
    if (!color_hex || color_hex[0] != '#') {
        return;
    }
    circe_strand_cache_append_color(color_hex);
    if (s_home_block_count < STRAND_MAX) {
        strncpy(s_home_blocks[s_home_block_count].color_hex, color_hex, sizeof(s_home_blocks[0].color_hex) - 1);
        s_home_blocks[s_home_block_count].color_hex[sizeof(s_home_blocks[0].color_hex) - 1] = '\0';
        s_home_block_count++;
    }
    refresh_strand_arc_from_blocks(s_home_blocks, s_home_block_count);
}
static void go_step(circe_flow_step_t step);
static void apply_theme_to_shell(void);
static void worker_busy_notice(void);
static bool enqueue_save_async(circe_flow_step_t on_success, int msg_key, bool quick_subline);
static void circe_ui_worker_done(const circe_worker_completion_t *c, void *ctx);

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
    circe_hud_show_prompt(&s_hud, circe_copy_get(key));
}

static void show_save_error(circe_save_result_t result)
{
    const char *code = circe_save_result_name(result);
    const char *detail = circe_storage_get_last_error();
    ESP_LOGE(TAG, "save error: %s detail=%s", code, detail && detail[0] ? detail : "(none)");
    char prompt[96];
    snprintf(prompt, sizeof(prompt), "Save failed:\n%s", code);
    circe_hud_show_companion_prompt(&s_hud, prompt, NULL);
    circe_hud_set_subline(&s_hud, detail && detail[0] ? detail : "");
}

static void show_save_success_notice(circe_save_result_t result)
{
    if (result == CIRCE_SAVE_OK_INDEX_WARN) {
        circe_hud_set_subline(&s_hud, "Saved. Index will rebuild.");
    }
}

static void show_home(void)
{
    circe_hud_show_minimal_home(&s_hud, circe_copy_get(CIRCE_PATTERN_HOME_PROMPT),
                                circe_copy_get(CIRCE_PATTERN_HOME_SUBLINE));
}

static void clear_content(void)
{
    s_btn_id_next = 0;
    s_btn_stack_idx = 0;
    s_slider = NULL;
    s_scroll = NULL;
    s_column = NULL;
    s_focus_body = NULL;
    if (s_more_btn) {
        lv_obj_del(s_more_btn);
        s_more_btn = NULL;
    }
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
    lv_group_set_wrap(s_group, true);
    attach_encoder_indev();
}

static void focus_first_obj(lv_obj_t *obj)
{
    if (s_group && obj) {
        lv_group_focus_obj(obj);
    }
}

static void ui_center_column(lv_obj_t *obj, int y_ofs)
{
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, y_ofs);
}

static void ui_center_label(lv_obj_t *lbl)
{
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
}

static lv_obj_t *content_root(void)
{
    return s_column ? s_column : s_content;
}

static lv_obj_t *begin_content_column(void)
{
    s_column = lv_obj_create(s_content);
    lv_obj_set_size(s_column, COL_W, COL_H);
    ui_center_column(s_column, 0);
    lv_obj_set_style_bg_opa(s_column, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_column, 0, 0);
    lv_obj_set_style_pad_all(s_column, 0, 0);
    lv_obj_clear_flag(s_column, LV_OBJ_FLAG_SCROLLABLE);
    return s_column;
}

static lv_obj_t *create_scroll_panel(void)
{
    s_scroll = lv_obj_create(content_root());
    lv_obj_set_size(s_scroll, COL_W, COL_H);
    ui_center_column(s_scroll, 0);
    lv_obj_set_style_pad_row(s_scroll, COL_PAD, 0);
    lv_obj_set_style_pad_all(s_scroll, 0, 0);
    lv_obj_set_style_border_width(s_scroll, 0, 0);
    lv_obj_set_style_bg_opa(s_scroll, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(s_scroll, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(s_scroll, LV_DIR_VER);
    return s_scroll;
}

static lv_obj_t *btn_parent(void)
{
    return s_scroll ? s_scroll : s_content;
}

static lv_obj_t *add_btn(const char *label, const char *id)
{
    lv_obj_t *parent = btn_parent();
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_width(btn, COL_W);
    circe_theme_style_button(btn);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *)id);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    circe_theme_style_button_label(lbl);
    ui_center_label(lbl);
    lv_obj_center(lbl);
    if (s_scroll) {
        lv_obj_t *prev = NULL;
        uint32_t cnt = lv_obj_get_child_cnt(parent);
        if (cnt > 1) {
            prev = lv_obj_get_child(parent, cnt - 2);
        }
        if (prev) {
            lv_obj_align_to(btn, prev, LV_ALIGN_OUT_BOTTOM_MID, 0, COL_PAD);
        } else {
            lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 0);
        }
    } else {
        const circe_theme_palette_t *p = circe_theme_get_palette();
        int y = 12 + s_btn_stack_idx * (p->btn_min_h + COL_PAD);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y);
        s_btn_stack_idx++;
    }
    if (s_group) {
        lv_group_add_obj(s_group, btn);
    }
    return btn;
}

static void add_back_btn(circe_flow_step_t target)
{
    add_btn(circe_copy_get(CIRCE_PATTERN_NAV_BACK), alloc_btn_id("nav:%d", (int)target));
}

static void worker_busy_notice(void)
{
    circe_hud_set_subline(&s_hud, "Please wait...");
}

static bool enqueue_save_async(circe_flow_step_t on_success, int msg_key, bool quick_subline)
{
    if (!s_engine) {
        return false;
    }
    if (circe_worker_is_busy()) {
        worker_busy_notice();
        return false;
    }
    circe_hud_set_subline(&s_hud, "Saving...");
    if (!circe_worker_post_save_entry(&s_engine->draft, s_engine->editing_existing, on_success, msg_key,
                                      quick_subline)) {
        circe_hud_set_subline(&s_hud, "Save queue failed");
        return false;
    }
    return true;
}

static void circe_ui_worker_done(const circe_worker_completion_t *c, void *ctx)
{
    (void)ctx;
    if (!c || !s_engine) {
        return;
    }

    switch (c->type) {
    case CIRCE_WORKER_TEST_SAVE:
        circe_hud_set_subline(&s_hud, c->summary);
        go_step(CIRCE_FLOW_DIAGNOSTICS);
        break;
    case CIRCE_WORKER_SAVE_ENTRY:
        if (circe_save_result_is_success(c->save_result)) {
            show_save_success_notice(c->save_result);
            strncpy(s_review_id, c->entry_id, sizeof(s_review_id) - 1);
            s_review_id[sizeof(s_review_id) - 1] = '\0';
            s_engine->editing_existing = false;
            if (c->saved_color[0] == '#') {
                circe_strand_cache_append_color(c->saved_color);
                strand_note_saved_color(c->saved_color);
            }
            s_engine->storage_ready = circe_storage_is_ready();
            if (c->success_message != CIRCE_MSG_NONE) {
                show_message((circe_pattern_key_t)c->success_message);
            }
            if (c->show_quick_subline) {
                circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_QUICK_ADD_LATER));
            }
            go_step(c->success_step);
        } else {
            show_save_error(c->save_result);
        }
        break;
    case CIRCE_WORKER_DELETE_ENTRY:
        if (c->success) {
            s_review_id[0] = '\0';
            show_message(CIRCE_PATTERN_DELETE_DONE);
            go_step(CIRCE_FLOW_HOME);
        } else {
            circe_hud_set_subline(&s_hud, c->summary);
        }
        break;
    case CIRCE_WORKER_REBUILD_INDEX:
        circe_hud_set_subline(&s_hud, c->summary);
        go_step(CIRCE_FLOW_DIAGNOSTICS);
        break;
    case CIRCE_WORKER_REINIT_STORAGE:
        s_engine->storage_ready = c->storage_ready;
        circe_hud_set_subline(&s_hud, c->summary);
        go_step(CIRCE_FLOW_DIAGNOSTICS);
        break;
    case CIRCE_WORKER_STORAGE_PROBE:
        circe_hud_set_subline(&s_hud, c->summary);
        go_step(CIRCE_FLOW_DIAGNOSTICS);
        break;
    case CIRCE_WORKER_LOAD_REVIEW:
        if (c->review_found && c->success) {
            s_engine->draft = c->entry;
            strncpy(s_review_id, c->entry_id, sizeof(s_review_id) - 1);
            s_review_id[sizeof(s_review_id) - 1] = '\0';
            go_step(CIRCE_FLOW_REVIEW);
        } else {
            go_step(CIRCE_FLOW_REVIEW_EMPTY);
        }
        break;
    default:
        break;
    }
}

static bool worker_post_or_busy(bool (*post_fn)(void))
{
    if (circe_worker_is_busy()) {
        worker_busy_notice();
        return false;
    }
    return post_fn();
}

static bool post_test_save(void)
{
    circe_hud_set_subline(&s_hud, "Running test...");
    return circe_worker_post_test_save();
}

static bool post_rebuild(void)
{
    circe_hud_set_subline(&s_hud, "Rebuilding...");
    return circe_worker_post_rebuild_index();
}

static bool post_reinit(void)
{
    circe_hud_set_subline(&s_hud, "Reinitializing...");
    return circe_worker_post_reinit_storage();
}

static bool post_load_review(void)
{
    circe_hud_set_subline(&s_hud, "Loading...");
    return circe_worker_post_load_review();
}

static bool post_delete_review(void)
{
    circe_hud_set_subline(&s_hud, "Deleting...");
    return circe_worker_post_delete_entry(s_review_id);
}

static void strand_place_block(int index, int total, uint32_t color_hex, bool is_void)
{
    if (!s_strand_arc) {
        return;
    }
    int angle = strand_angle_for(index, total);
    int x = STRAND_CX + ((STRAND_R * lv_trigo_sin(angle)) >> LV_TRIGO_SHIFT) - (STRAND_BLOCK / 2);
    int y = STRAND_CY - ((STRAND_R * lv_trigo_cos(angle)) >> LV_TRIGO_SHIFT) - (STRAND_BLOCK / 2);

    lv_obj_t *block = lv_obj_create(s_strand_arc);
    lv_obj_set_size(block, STRAND_BLOCK, STRAND_BLOCK);
    lv_obj_set_pos(block, x, y);
    lv_obj_clear_flag(block, LV_OBJ_FLAG_SCROLLABLE);
    circe_theme_style_strand_block(block, color_hex, is_void);
}

static void refresh_strand_arc(void)
{
    if (!s_strand_arc) {
        return;
    }
    lv_obj_clean(s_strand_arc);
    strand_place_block(0, 1, 0, true);
}

static void refresh_strand_arc_from_blocks(circe_strand_block_t *blocks, int count)
{
    if (!s_strand_arc) {
        return;
    }
    lv_obj_clean(s_strand_arc);
    if (count <= 0) {
        refresh_strand_arc();
        return;
    }
    int shown = count;
    if (shown > STRAND_MAX) {
        shown = STRAND_MAX;
    }
    for (int i = 0; i < shown; i++) {
        strand_place_block(i, shown, parse_hex_color(blocks[i].color_hex), false);
    }
}

void circe_ui_refresh_strand_from_storage(void)
{
    if (!s_strand_arc) {
        return;
    }
    circe_strand_block_t blocks[STRAND_MAX];
    int count = 0;
    circe_storage_today_strand(blocks, STRAND_MAX, &count);

    if (!lvgl_port_lock(1000)) {
        return;
    }
    refresh_strand_arc_from_blocks(blocks, count);
    lvgl_port_unlock();
}

void circe_ui_refresh_strand_in_context(void)
{
    circe_strand_block_t blocks[STRAND_MAX];
    int count = 0;
    circe_storage_today_strand(blocks, STRAND_MAX, &count);
    refresh_strand_arc_from_blocks(blocks, count);
}

void circe_ui_apply_strand_blocks(const circe_strand_block_t *blocks, int count)
{
    if (!s_strand_arc) {
        return;
    }
    if (!blocks || count <= 0) {
        refresh_strand_arc();
        return;
    }
    refresh_strand_arc_from_blocks((circe_strand_block_t *)blocks, count);
}

static void apply_theme_to_shell(void)
{
    if (!s_scr) {
        return;
    }
    circe_theme_apply_screen(s_scr);
    circe_hud_apply_theme(&s_hud);
}

static lv_obj_t *add_theme_row(circe_theme_id_t id)
{
    const circe_theme_palette_t *p = circe_theme_get_palette_by_id(id);
    char label[48];
    snprintf(label, sizeof(label), "%s", p->display_name);
    lv_obj_t *btn = add_btn(label, alloc_btn_id("theme:%d", (int)id));
    if (id == s_appearance_pick) {
        lv_obj_add_state(btn, LV_STATE_FOCUS_KEY);
    }
    return btn;
}

static void btn_event_cb(lv_event_t *e)
{
    const char *id = (const char *)lv_event_get_user_data(e);
    if (!id || !s_engine) {
        return;
    }

    if (strcmp(id, "ready_body") == 0) {
        circe_conversation_start_body_only(s_engine);
        go_step(CIRCE_FLOW_BODY_AREA);
    } else if (strcmp(id, "body") == 0) {
        circe_conversation_start_body_only(s_engine);
        go_step(CIRCE_FLOW_BODY_AREA);
    } else if (strcmp(id, "quick") == 0) {
        circe_conversation_start_quick(s_engine);
        go_step(CIRCE_FLOW_QUICK_PICK);
    } else if (strcmp(id, "review") == 0) {
        worker_post_or_busy(post_load_review);
    } else if (strcmp(id, "more") == 0) {
        go_step(CIRCE_FLOW_MORE);
    } else if (strcmp(id, "more_appearance") == 0) {
        s_appearance_pick = circe_theme_get_active();
        go_step(CIRCE_FLOW_APPEARANCE);
    } else if (strcmp(id, "more_storage") == 0) {
        go_step(CIRCE_FLOW_DIAGNOSTICS);
    } else if (strcmp(id, "appearance_apply") == 0) {
        circe_theme_commit_preview();
        apply_theme_to_shell();
        circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_APPEARANCE_APPLIED));
        go_step(CIRCE_FLOW_MORE);
    } else if (strcmp(id, "appearance_cancel") == 0) {
        circe_theme_revert_preview();
        apply_theme_to_shell();
        go_step(CIRCE_FLOW_MORE);
    } else if (strcmp(id, "strand") == 0) {
        go_step(CIRCE_FLOW_STRAND);
    } else if (strcmp(id, "save") == 0) {
        enqueue_save_async(CIRCE_FLOW_SAVE_DONE, CIRCE_MSG_NONE, false);
    } else if (strcmp(id, "skip_color") == 0) {
        enqueue_save_async(CIRCE_FLOW_SAVE_DONE, CIRCE_MSG_NONE, false);
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
        if (s_review_id[0]) {
            worker_post_or_busy(post_delete_review);
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
        worker_post_or_busy(post_rebuild);
    } else if (strcmp(id, "selftest") == 0) {
        worker_post_or_busy(post_test_save);
    } else if (strcmp(id, "test_save") == 0) {
        worker_post_or_busy(post_test_save);
    } else if (strcmp(id, "storage_reinit") == 0) {
        worker_post_or_busy(post_reinit);
    } else if (strncmp(id, "nav:", 4) == 0) {
        go_step((circe_flow_step_t)atoi(id + 4));
    } else if (strncmp(id, "theme:", 6) == 0) {
        circe_theme_id_t pick = (circe_theme_id_t)atoi(id + 6);
        if (pick < CIRCE_THEME_COUNT) {
            s_appearance_pick = pick;
            circe_theme_preview(pick);
            apply_theme_to_shell();
            go_step(CIRCE_FLOW_APPEARANCE);
        }
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
            enqueue_save_async(CIRCE_FLOW_REVIEW, CIRCE_PATTERN_EDIT_SAVED, false);
        } else {
            go_step(CIRCE_FLOW_INTENSITY);
        }
    } else if (strncmp(id, "color:", 6) == 0) {
        strncpy(s_engine->draft.color_hex, id + 6, sizeof(s_engine->draft.color_hex) - 1);
        s_engine->draft.color_hex[sizeof(s_engine->draft.color_hex) - 1] = '\0';
        if (!circe_entry_validate_color_hex(s_engine->draft.color_hex)) {
            ESP_LOGW(TAG, "invalid color from btn id='%s' — using #808080", id);
            snprintf(s_engine->draft.color_hex, sizeof(s_engine->draft.color_hex), "#808080");
        }
        if (s_engine->editing_existing) {
            enqueue_save_async(CIRCE_FLOW_REVIEW, CIRCE_PATTERN_EDIT_SAVED, false);
        } else {
            enqueue_save_async(CIRCE_FLOW_SAVE_DONE, CIRCE_MSG_NONE, false);
        }
    } else if (strncmp(id, "quick:", 6) == 0) {
        int preset = id[6] - '0';
        circe_entry_modes_apply_quick_preset(&s_engine->draft, preset);
        enqueue_save_async(CIRCE_FLOW_SAVE_DONE, CIRCE_PATTERN_QUICK_SAVED, true);
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

void circe_ui_apply_boot_strand(void)
{
    circe_strand_block_t blocks[STRAND_MAX];
    int count = 0;
    circe_strand_cache_get_loaded(blocks, STRAND_MAX, &count);
    if (count > 0) {
        s_home_block_count = count;
        for (int i = 0; i < count; i++) {
            s_home_blocks[i] = blocks[i];
        }
        refresh_strand_arc_from_blocks(blocks, count);
    }
}

void circe_ui_init(void)
{
    setup_encoder_group();
    circe_worker_init(circe_ui_worker_done, NULL);

    s_scr = lv_obj_create(NULL);
    lv_scr_load(s_scr);
    circe_theme_apply_screen(s_scr);

    circe_hud_create(s_scr, &s_hud);
    s_content = circe_hud_actions(&s_hud);
    s_strand_arc = circe_hud_strand_layer(&s_hud);
}

void circe_ui_show_step(circe_flow_step_t step)
{
    if (!s_engine) {
        return;
    }
    clear_content();
    setup_encoder_group();
    apply_theme_to_shell();
    if (step != CIRCE_FLOW_HOME) {
        begin_content_column();
        show_message(circe_conversation_prompt_for_step(s_engine));
    }

    switch (step) {
    case CIRCE_FLOW_HOME: {
        circe_hud_set_reset_mode(&s_hud, true);
        show_home();
        circe_home_actions_t actions;
        circe_actions_layout_home_minimal(s_content, s_scr, &actions, btn_event_cb, s_group);
        s_focus_body = actions.ready;
        s_more_btn = actions.more;
        if (!circe_storage_is_ready()) {
            circe_hud_show_companion_prompt(&s_hud, circe_copy_get(CIRCE_PATTERN_ERROR_STORAGE_MISSING), NULL);
        }
        focus_first_obj(s_focus_body);
        break;
    }

    case CIRCE_FLOW_MORE:
        add_btn(circe_copy_get(CIRCE_PATTERN_MORE_APPEARANCE), "more_appearance");
        add_btn(circe_copy_get(CIRCE_PATTERN_MORE_STORAGE), "more_storage");
        add_back_btn(CIRCE_FLOW_HOME);
        break;

    case CIRCE_FLOW_APPEARANCE:
        create_scroll_panel();
        for (int i = 0; i < circe_theme_count(); i++) {
            add_theme_row((circe_theme_id_t)i);
        }
        add_btn("Apply", "appearance_apply");
        add_btn(circe_copy_get(CIRCE_PATTERN_NAV_CANCEL), "appearance_cancel");
        add_back_btn(CIRCE_FLOW_MORE);
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
        create_scroll_panel();
        s_slider = lv_slider_create(s_scroll);
        lv_slider_set_range(s_slider, 1, 10);
        lv_slider_set_value(s_slider, s_engine->draft.intensity, LV_ANIM_OFF);
        lv_obj_set_width(s_slider, COL_W - 16);
        circe_theme_style_slider(s_slider);
        lv_obj_align(s_slider, LV_ALIGN_TOP_MID, 0, 0);
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
            add_btn(circe_quick_color_labels[i], alloc_btn_id("color:%s", circe_quick_colors[i]));
        }
        add_btn("Skip color", "skip_color");
        add_back_btn(CIRCE_FLOW_BODY_ADD_MORE);
        break;

    case CIRCE_FLOW_SAVE_DONE:
        show_message(CIRCE_PATTERN_SAVE_CONFIRMED);
        if (!circe_index_is_dirty()) {
            circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE));
        }
        strand_note_saved_color(s_engine->draft.color_hex);
        add_btn("Home", "home");
        break;

    case CIRCE_FLOW_REVIEW: {
        create_scroll_panel();
        char line[320];
        snprintf(line, sizeof(line),
                 "Mode: %s\nEmotion: %s\nAreas: %d  Sensations: %d\nColor: %s  Intensity: %d\nRev: %d  Private: yes",
                 circe_entry_mode_str(s_engine->draft.entry_mode), s_engine->draft.emotion, s_engine->draft.body_area_count,
                 s_engine->draft.body_sensation_count, s_engine->draft.color_hex, s_engine->draft.intensity,
                 s_engine->draft.revision);
        lv_obj_t *card = lv_obj_create(s_scroll);
        lv_obj_set_width(card, COL_W);
        lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 0);
        circe_theme_style_card(card);
        lv_obj_t *lbl = lv_label_create(card);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, COL_W - 16);
        lv_label_set_text(lbl, line);
        ui_center_label(lbl);
        circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_BODY);
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);
        add_btn("Edit", "edit");
        add_btn("Delete", "delete");
        add_btn("Home", "home");
        break;
    }

    case CIRCE_FLOW_REVIEW_EMPTY: {
        create_scroll_panel();
        lv_obj_t *card = lv_obj_create(s_scroll);
        lv_obj_set_width(card, COL_W);
        lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 0);
        circe_theme_style_card(card);
        lv_obj_t *lbl = lv_label_create(card);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, COL_W - 16);
        lv_label_set_text(lbl, circe_copy_get(CIRCE_PATTERN_REVIEW_EMPTY));
        ui_center_label(lbl);
        circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_BODY);
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);
        add_btn("Home", "home");
        add_back_btn(CIRCE_FLOW_HOME);
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
            add_btn(circe_quick_color_labels[i], alloc_btn_id("color:%s", circe_quick_colors[i]));
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
        circe_strand_block_t blocks[STRAND_MAX];
        int count = 0;
        circe_storage_today_strand(blocks, STRAND_MAX, &count);
        char line[96];
        snprintf(line, sizeof(line), "%d entries today", count);
        lv_obj_t *lbl = lv_label_create(content_root());
        lv_label_set_text(lbl, line);
        ui_center_label(lbl);
        ui_center_column(lbl, 0);
        circe_theme_style_button_label(lbl);
        lv_obj_t *row = lv_obj_create(content_root());
        lv_obj_set_width(row, COL_W);
        lv_obj_set_height(row, 28);
        ui_center_column(row, 28);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        int bx = (COL_W - (count > 0 ? count * 26 - 4 : 22)) / 2;
        if (bx < 0) {
            bx = 0;
        }
        for (int i = 0; i < count && i < STRAND_MAX; i++) {
            lv_obj_t *block = lv_obj_create(row);
            lv_obj_set_size(block, 22, 22);
            lv_obj_set_pos(block, bx, 3);
            bx += 26;
            circe_theme_style_strand_block(block, parse_hex_color(blocks[i].color_hex), false);
        }
        if (count == 0) {
            lv_obj_t *block = lv_obj_create(row);
            lv_obj_set_size(block, 22, 22);
            lv_obj_set_pos(block, bx, 3);
            circe_theme_style_strand_block(block, 0, true);
        }
        add_btn("Home", "home");
        break;
    }

    case CIRCE_FLOW_DIAGNOSTICS: {
        create_scroll_panel();
        circe_storage_health_t h;
        circe_storage_health_check(&h);
        char line[480];
        snprintf(line, sizeof(line),
                 "Mounted: %s\nPath: %s\nWritable: %s\nStorage Ready: %s\nProbe: %s\nEntries Dir: %s\nIndex Dir: %s\n"
                 "Dirty: %s  Entries: %d\nCard: %ld MB\nLast error: %s",
                 h.sd_mounted ? "YES" : "NO", h.base_path, h.writable ? "YES" : "NO", h.storage_ready ? "YES" : "NO",
                 h.probe_detail, h.entries_dir_ok ? "YES" : "NO", h.index_dir_ok ? "YES" : "NO",
                 h.index_dirty ? "YES" : "NO", h.entry_count, h.card_size_mb,
                 h.last_error[0] ? h.last_error : "none");
        lv_obj_t *card = lv_obj_create(s_scroll);
        lv_obj_set_width(card, COL_W);
        lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 0);
        circe_theme_style_card(card);
        lv_obj_t *lbl = lv_label_create(card);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl, COL_W - 16);
        lv_label_set_text(lbl, line);
        ui_center_label(lbl);
        circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_BODY);
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 8);
        add_btn("Test Save", "test_save");
        add_btn("Reinitialize Storage", "storage_reinit");
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
    return enqueue_save_async(CIRCE_FLOW_SAVE_DONE, CIRCE_MSG_NONE, false);
}

bool circe_ui_delete_latest(void)
{
    if (!s_review_id[0]) {
        return false;
    }
    return worker_post_or_busy(post_delete_review);
}

void circe_ui_run_rebuild_test(void)
{
    worker_post_or_busy(post_rebuild);
}
