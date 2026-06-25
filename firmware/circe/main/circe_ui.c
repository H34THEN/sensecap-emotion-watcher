#include "circe_ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circe_copy.h"
#include "circe_entry_modes.h"
#include "circe_fonts.h"
#include "circe_hud.h"
#include "circe_index.h"
#include "circe_save.h"
#include "circe_storage.h"
#include "circe_strand_cache.h"
#include "circe_terminal.h"
#include "circe_theme.h"
#include "circe_time.h"
#include "circe_time_picker.h"
#include "circe_worker.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#define MAX_BTN_IDS       40
#define CIRCE_UI_COL_W    240
#define COL_W             CIRCE_UI_COL_W
#define COL_PAD           8
#define COL_H                240
#define CIRCE_MSG_NONE       (-1)

static const char *TAG = "circe_ui";

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
static char s_row_labels[MAX_BTN_IDS][48];
static int s_btn_id_next = 0;
static int s_row_label_next = 0;
static int s_btn_stack_idx = 0;
static circe_terminal_feed_t s_feed;
static lv_timer_t *s_nav_timer = NULL;
static lv_obj_t *s_first_row = NULL;
static circe_flow_step_t s_nav_back_step = CIRCE_FLOW_HOME;
static circe_time_picker_t s_time_picker;
static circe_storage_health_t s_diag_health;
static bool s_diag_health_valid;

static void btn_event_cb(lv_event_t *e);
static void refresh_strand_arc_from_blocks(circe_strand_block_t *blocks, int count);

static void strand_note_saved_color(const char *color_hex)
{
    if (!color_hex || color_hex[0] != '#') {
        return;
    }
    circe_strand_cache_append_color(color_hex);
}
static void go_step(circe_flow_step_t step);
static void apply_theme_to_shell(void);
static void time_picker_done(bool saved, void *ctx);
static void show_terminal_prompt_text(const char *text);

static void diagnostics_format_health_line(char *line, size_t len, const circe_storage_health_t *h)
{
    if (!line || len == 0 || !h) {
        return;
    }
    snprintf(line, len, "ready:%s entries:%d probe:%s", h->storage_ready ? "yes" : "no", h->entry_count,
             h->probe_passed ? "PASS" : "FAIL");
}

static void diagnostics_show_health_feed(void)
{
    char line[160];
    if (!s_diag_health_valid) {
        show_terminal_prompt_text("checking storage...");
        return;
    }
    diagnostics_format_health_line(line, sizeof(line), &s_diag_health);
    show_terminal_prompt_text(line);
}

static void diagnostics_apply_health(const circe_storage_health_t *h)
{
    if (!h) {
        return;
    }
    s_diag_health = *h;
    s_diag_health_valid = true;
    if (s_engine) {
        s_engine->storage_ready = h->storage_ready;
    }
    if (s_engine && s_engine->step == CIRCE_FLOW_DIAGNOSTICS) {
        diagnostics_show_health_feed();
    }
}

static bool post_health_check(void)
{
    s_diag_health_valid = false;
    circe_hud_set_subline(&s_hud, "Checking...");
    diagnostics_show_health_feed();
    return circe_worker_post_health_check();
}

static bool post_probe(void)
{
    circe_hud_set_subline(&s_hud, "Probing...");
    return circe_worker_post_storage_probe();
}

static bool enqueue_save_async(circe_flow_step_t on_success, int msg_key, bool quick_subline);
static void circe_ui_worker_done(const circe_worker_completion_t *c, void *ctx);

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
    const char *lines[] = {circe_copy_get(key)};
    circe_terminal_feed_set(&s_feed, lines, 1);
    circe_terminal_feed_show_cursor(&s_feed, true);
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
    circe_hud_show_terminal_shell(&s_hud, "CIRCE", "online");
}

static void clear_content(void)
{
    s_btn_id_next = 0;
    s_row_label_next = 0;
    s_btn_stack_idx = 0;
    s_first_row = NULL;
    s_slider = NULL;
    circe_time_picker_destroy(&s_time_picker);
    s_scroll = NULL;
    s_column = NULL;
    s_focus_body = NULL;
    circe_terminal_feed_destroy(&s_feed);
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

static const char *persist_row_label(const char *text)
{
    if (!text || s_row_label_next >= MAX_BTN_IDS) {
        return text ? text : "";
    }
    char *buf = s_row_labels[s_row_label_next++];
    circe_terminal_to_upper(buf, sizeof(s_row_labels[0]), text);
    return buf;
}

static lv_obj_t *add_btn(const char *label, const char *id)
{
    lv_obj_t *parent = btn_parent();
    const char *display = persist_row_label(label);
    lv_obj_t *row = circe_terminal_add_row(parent, display, id, btn_event_cb, s_group, s_btn_stack_idx++);
    if (!s_first_row) {
        s_first_row = row;
    }
    return row;
}

static void terminal_nav_back(void *ctx)
{
    (void)ctx;
    go_step(s_nav_back_step);
}

static void terminal_nav_sysmenu(void *ctx)
{
    (void)ctx;
    if (s_engine) {
        go_step(CIRCE_FLOW_MORE);
    }
}

static void nav_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_time_picker.active) {
        circe_time_picker_poll(&s_time_picker, time_picker_done, &s_hud);
    } else {
        circe_terminal_nav_poll();
    }
}

static void time_picker_done(bool saved, void *ctx)
{
    circe_hud_t *hud = (circe_hud_t *)ctx;
    if (saved) {
        if (hud) {
            circe_hud_set_subline(hud, "Time saved.");
        }
        go_step(CIRCE_FLOW_MORE);
        return;
    }
    go_step(CIRCE_FLOW_MORE);
}

static void setup_terminal_shell(circe_flow_step_t back_step, const char *status_line)
{
    s_nav_back_step = back_step;
    circe_terminal_nav_set_back_step(back_step);
    circe_hud_show_terminal_prompt(&s_hud, status_line ? status_line : "");
    if (!s_hud.viewport) {
        ESP_LOGE(TAG, "setup_terminal_shell: viewport null");
        return;
    }
    circe_terminal_feed_init(&s_feed, s_hud.viewport);
    begin_content_column();
}

static void show_terminal_prompt_feed(circe_pattern_key_t key)
{
    show_terminal_prompt_text(circe_copy_get(key));
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
    case CIRCE_WORKER_HEALTH_CHECK:
    case CIRCE_WORKER_STORAGE_STATUS:
    case CIRCE_WORKER_DIAGNOSTICS_REFRESH:
        diagnostics_apply_health(&c->health);
        circe_hud_set_subline(&s_hud, c->summary);
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

static void refresh_strand_arc_from_blocks(circe_strand_block_t *blocks, int count)
{
    (void)blocks;
    (void)count;
    if (!s_strand_arc) {
        return;
    }
    lv_obj_clean(s_strand_arc);
}

void circe_ui_refresh_strand_from_storage(void)
{
    if (!s_strand_arc) {
        return;
    }
    if (!lvgl_port_lock(1000)) {
        return;
    }
    refresh_strand_arc_from_blocks(NULL, 0);
    lvgl_port_unlock();
}

void circe_ui_refresh_strand_in_context(void)
{
    refresh_strand_arc_from_blocks(NULL, 0);
}

void circe_ui_apply_strand_blocks(const circe_strand_block_t *blocks, int count)
{
    (void)blocks;
    (void)count;
    refresh_strand_arc_from_blocks(NULL, 0);
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
    } else if (strcmp(id, "more_time") == 0) {
        go_step(CIRCE_FLOW_TIME);
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
        circe_hud_set_subline(&s_hud, "Strand unavailable");
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
    } else if (strcmp(id, "storage_probe") == 0) {
        worker_post_or_busy(post_probe);
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
    if (!s_strand_arc) {
        return;
    }
    lv_obj_clean(s_strand_arc);
}

void circe_ui_init(void)
{
    setup_encoder_group();
    circe_worker_init(circe_ui_worker_done, NULL);
    circe_terminal_nav_init(terminal_nav_back, terminal_nav_sysmenu, NULL);

    s_scr = lv_obj_create(NULL);
    lv_scr_load(s_scr);
    circe_theme_apply_screen(s_scr);

    circe_hud_create(s_scr, &s_hud);
    s_content = circe_hud_actions(&s_hud);
    s_strand_arc = circe_hud_strand_layer(&s_hud);

    s_nav_timer = lv_timer_create(nav_timer_cb, 50, NULL);
}

static void show_terminal_prompt_text(const char *text)
{
    const char *lines[] = {text};
    circe_terminal_feed_set(&s_feed, lines, 1);
    circe_terminal_feed_show_cursor(&s_feed, true);
}

void circe_ui_show_step(circe_flow_step_t step)
{
    if (!s_engine) {
        return;
    }
    clear_content();
    setup_encoder_group();
    apply_theme_to_shell();
    circe_hud_set_reset_mode(&s_hud, false);
    circe_terminal_nav_enable(true);

    switch (step) {
    case CIRCE_FLOW_HOME: {
        show_home();
        if (!s_hud.viewport) {
            ESP_LOGE(TAG, "HOME: viewport null");
            break;
        }
        circe_terminal_feed_init(&s_feed, s_hud.viewport);
        const char *lines[] = {"ready to check in", "start with body"};
        circe_terminal_feed_set(&s_feed, lines, 2);
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_HOME;
        add_btn("BODY CHECK-IN", "ready_body");
        add_btn("QUICK NOTE", "quick");
        add_btn("REVIEW", "review");
        add_btn("SETTINGS", "more");
        if (!circe_storage_is_ready()) {
            const char *warn[] = {"storage unavailable", "check memory card"};
            circe_terminal_feed_set(&s_feed, warn, 2);
            circe_terminal_feed_show_cursor(&s_feed, true);
        }
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_MORE:
        setup_terminal_shell(CIRCE_FLOW_HOME, "settings");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_MORE_MENU));
        s_nav_back_step = CIRCE_FLOW_HOME;
        add_btn(circe_copy_get(CIRCE_PATTERN_MORE_APPEARANCE), "more_appearance");
        add_btn("TIME", "more_time");
        add_btn(circe_copy_get(CIRCE_PATTERN_MORE_STORAGE), "more_storage");
        add_back_btn(CIRCE_FLOW_HOME);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_APPEARANCE:
        setup_terminal_shell(CIRCE_FLOW_MORE, "appearance");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_APPEARANCE_PROMPT));
        s_nav_back_step = CIRCE_FLOW_MORE;
        create_scroll_panel();
        for (int i = 0; i < circe_theme_count(); i++) {
            add_theme_row((circe_theme_id_t)i);
        }
        add_btn("APPLY", "appearance_apply");
        add_btn(circe_copy_get(CIRCE_PATTERN_NAV_CANCEL), "appearance_cancel");
        add_back_btn(CIRCE_FLOW_MORE);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_BODY_AREA:
        setup_terminal_shell(s_engine->editing_existing ? CIRCE_FLOW_EDIT : CIRCE_FLOW_HOME, "body check-in");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_BODY_AREA_PROMPT));
        s_nav_back_step = s_engine->editing_existing ? CIRCE_FLOW_EDIT : CIRCE_FLOW_HOME;
        create_scroll_panel();
        for (int i = 0; i < circe_body_area_count; i++) {
            add_btn(circe_body_areas[i], alloc_btn_id("area:%s", circe_body_areas[i]));
        }
        add_back_btn(s_nav_back_step);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_BODY_SENSATION:
        setup_terminal_shell(CIRCE_FLOW_BODY_AREA, "body sensation");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_BODY_SENSATION_PROMPT));
        s_nav_back_step = CIRCE_FLOW_BODY_AREA;
        create_scroll_panel();
        for (int i = 0; i < circe_body_sensation_count; i++) {
            add_btn(circe_body_sensations[i], alloc_btn_id("sen:%s", circe_body_sensations[i]));
        }
        add_back_btn(CIRCE_FLOW_BODY_AREA);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_INTENSITY:
        setup_terminal_shell(CIRCE_FLOW_BODY_SENSATION, "intensity");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_BODY_INTENSITY_PROMPT));
        s_nav_back_step = CIRCE_FLOW_BODY_SENSATION;
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
        add_btn("CONTINUE", "next_intensity");
        add_back_btn(CIRCE_FLOW_BODY_SENSATION);
        focus_first_obj(s_first_row ? s_first_row : s_slider);
        break;

    case CIRCE_FLOW_BODY_ADD_MORE:
        setup_terminal_shell(CIRCE_FLOW_INTENSITY, "add more?");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_BODY_ADD_ANOTHER));
        s_nav_back_step = CIRCE_FLOW_INTENSITY;
        add_btn("ADD ANOTHER SENSATION", "add_sensation");
        add_btn(circe_copy_get(CIRCE_PATTERN_BODY_CONTINUE), "continue_save");
        add_back_btn(CIRCE_FLOW_INTENSITY);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_COLOR_OPTIONAL:
        setup_terminal_shell(CIRCE_FLOW_BODY_ADD_MORE, "emotional tone");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_COLOR_OPTIONAL_PROMPT));
        s_nav_back_step = CIRCE_FLOW_BODY_ADD_MORE;
        create_scroll_panel();
        for (int i = 0; i < circe_quick_color_count; i++) {
            add_btn(circe_quick_color_labels[i], alloc_btn_id("color:%s", circe_quick_colors[i]));
        }
        add_btn("SKIP", "skip_color");
        add_back_btn(CIRCE_FLOW_BODY_ADD_MORE);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_SAVE_DONE:
        setup_terminal_shell(CIRCE_FLOW_HOME, "saved");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_SAVE_CONFIRMED));
        if (!circe_index_is_dirty()) {
            circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE));
        }
        strand_note_saved_color(s_engine->draft.color_hex);
        s_nav_back_step = CIRCE_FLOW_HOME;
        add_btn("HOME", "home");
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_REVIEW: {
        setup_terminal_shell(CIRCE_FLOW_HOME, "review entry");
        char line[320];
        snprintf(line, sizeof(line), "%s | areas %d | intensity %d", s_engine->draft.emotion,
                 s_engine->draft.body_area_count, s_engine->draft.intensity);
        show_terminal_prompt_text(line);
        s_nav_back_step = CIRCE_FLOW_HOME;
        add_btn("EDIT", "edit");
        add_btn("DELETE", "delete");
        add_btn("HOME", "home");
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_REVIEW_EMPTY: {
        setup_terminal_shell(CIRCE_FLOW_HOME, "review");
        const char *lines[] = {"no entries recorded yet", "begin a check-in to create one"};
        circe_terminal_feed_set(&s_feed, lines, 2);
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_HOME;
        add_btn("HOME", "home");
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_DELETE_CONFIRM:
        setup_terminal_shell(CIRCE_FLOW_REVIEW, "confirm delete");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_DELETE_CONFIRM));
        s_nav_back_step = CIRCE_FLOW_REVIEW;
        add_btn("YES, DELETE", "delete_yes");
        add_btn(circe_copy_get(CIRCE_PATTERN_NAV_CANCEL), alloc_btn_id("nav:%d", CIRCE_FLOW_REVIEW));
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_EDIT:
        setup_terminal_shell(CIRCE_FLOW_REVIEW, "edit entry");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_EDIT_PROMPT));
        s_nav_back_step = CIRCE_FLOW_REVIEW;
        add_btn(circe_copy_get(CIRCE_PATTERN_EDIT_COLOR), "edit_color");
        add_btn(circe_copy_get(CIRCE_PATTERN_EDIT_ADD_SENSATION), "edit_add_sensation");
        add_back_btn(CIRCE_FLOW_REVIEW);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_EDIT_COLOR:
        setup_terminal_shell(CIRCE_FLOW_EDIT, "change tone");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_EDIT_COLOR));
        s_nav_back_step = CIRCE_FLOW_EDIT;
        create_scroll_panel();
        for (int i = 0; i < circe_quick_color_count; i++) {
            add_btn(circe_quick_color_labels[i], alloc_btn_id("color:%s", circe_quick_colors[i]));
        }
        add_back_btn(CIRCE_FLOW_EDIT);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_QUICK_PICK:
        setup_terminal_shell(CIRCE_FLOW_HOME, "quick note");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_QUICK_ONE_TAP));
        s_nav_back_step = CIRCE_FLOW_HOME;
        create_scroll_panel();
        for (int i = 0; i < CIRCE_QUICK_PRESET_COUNT; i++) {
            add_btn(circe_quick_presets[i].label, alloc_btn_id("quick:%d", i));
        }
        add_back_btn(CIRCE_FLOW_HOME);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_STRAND: {
        setup_terminal_shell(CIRCE_FLOW_DIAGNOSTICS, "strand");
        show_terminal_prompt_text("strand unavailable");
        s_nav_back_step = CIRCE_FLOW_DIAGNOSTICS;
        add_back_btn(CIRCE_FLOW_DIAGNOSTICS);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_DIAGNOSTICS: {
        setup_terminal_shell(CIRCE_FLOW_MORE, "storage diagnostics");
        s_nav_back_step = CIRCE_FLOW_MORE;
        create_scroll_panel();
        diagnostics_show_health_feed();
        add_btn("TEST SAVE", "test_save");
        add_btn("RUN PROBE", "storage_probe");
        add_btn("REINIT STORAGE", "storage_reinit");
        add_btn("REBUILD INDEX", "rebuild");
        add_btn("SELF TEST", "selftest");
        add_btn("HOME", "home");
        focus_first_obj(s_first_row);
        worker_post_or_busy(post_health_check);
        break;
    }

    case CIRCE_FLOW_TIME:
        setup_terminal_shell(CIRCE_FLOW_MORE, "time setup");
        circe_hud_set_heading(&s_hud, "CIRCE");
        circe_hud_set_subline(&s_hud, "PRESS NEXT  HOLD SAVE");
        s_nav_back_step = CIRCE_FLOW_MORE;
        circe_terminal_nav_enable(false);
        begin_content_column();
        circe_time_picker_create(&s_time_picker, s_column);
        break;

    case CIRCE_FLOW_TIME_EDIT_DATE:
    case CIRCE_FLOW_TIME_EDIT_TIME:
        go_step(CIRCE_FLOW_TIME);
        break;

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
