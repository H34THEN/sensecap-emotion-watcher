#include "circe_ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "circe_body_map.h"
#include "circe_copy.h"
#include "circe_daily.h"
#include "circe_encoder.h"
#include "circe_entry_modes.h"
#include "circe_fonts.h"
#include "circe_hud.h"
#include "circe_index.h"
#include "circe_save.h"
#include "circe_selector.h"
#include "circe_status_banner.h"
#include "circe_ui_tokens.h"
#include "circe_storage.h"
#include "circe_strand_cache.h"
#include "circe_color_picker.h"
#include "circe_color_intel.h"
#include "circe_home_bg.h"
#include "circe_home_wheel.h"
#include "circe_memory_browser.h"
#include "circe_patterns.h"
#include "circe_photo.h"
#include "circe_voice.h"
#include "circe_reflection.h"
#include "circe_regulation.h"
#include "circe_timeline.h"
#include "circe_terminal.h"
#include "circe_theme.h"
#include "circe_time.h"
#include "circe_time_picker.h"
#include "circe_worker.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#define MAX_BTN_IDS       40
#define CIRCE_UI_COL_W    CIRCE_UI_CONTENT_COL_W
#define COL_W             CIRCE_UI_CONTENT_COL_W
#define COL_PAD           CIRCE_UI_CONTENT_COL_PAD
#define COL_H             CIRCE_UI_CONTENT_COL_H
#define CIRCE_MSG_NONE       (-1)

static const char *TAG = "circe_ui";

static const circe_selector_item_t s_memory_menu_items[] = {
    {"TODAY", "mem_today"},
    {"YESTERDAY", "mem_yesterday"},
    {"THIS WEEK", "mem_week"},
    {"ALL ENTRIES", "mem_all"},
    {"PATTERNS", "mem_patterns"},
    {"BODY MAP", "mem_body_map"},
    {"BACK", "nav:0"},
};

static const circe_selector_item_t s_settings_menu_items[] = {
    {"APPEARANCE", "more_appearance"},
    {"TIME", "more_time"},
    {"VOICE CUES", "more_voice"},
    {"BACK", "nav:0"},
};

static const circe_selector_item_t s_regulation_menu_items[] = {
    {"BREATHING", "reg_breathing"},
    {"BODY ANCHOR", "reg_anchor"},
    {"5-4-3-2-1", "reg_54321"},
    {"SENSORY RESET", "reg_sensory"},
    {"BILATERAL TAP", "reg_bilateral"},
    {"BACK", "nav:0"},
};

static const circe_selector_item_t s_voice_menu_items[] = {
    {"SOFT", "voice_soft"},
    {"TEST TONE", "voice_test"},
    {"OFF", "voice_off"},
    {"BACK", "nav:31"},
};

static const circe_selector_item_t s_diagnostics_menu_items[] = {
    {"TEST SAVE", "test_save"},
    {"RUN PROBE", "storage_probe"},
    {"REINIT STORAGE", "storage_reinit"},
    {"REBUILD INDEX", "rebuild"},
    {"SELF TEST", "selftest"},
    {"BACK", "nav:0"},
};

#define MEMORY_MENU_COUNT   (int)(sizeof(s_memory_menu_items) / sizeof(s_memory_menu_items[0]))
#define SETTINGS_MENU_COUNT (int)(sizeof(s_settings_menu_items) / sizeof(s_settings_menu_items[0]))
#define REG_MENU_COUNT      (int)(sizeof(s_regulation_menu_items) / sizeof(s_regulation_menu_items[0]))
#define VOICE_MENU_COUNT    (int)(sizeof(s_voice_menu_items) / sizeof(s_voice_menu_items[0]))
#define DIAG_MENU_COUNT     (int)(sizeof(s_diagnostics_menu_items) / sizeof(s_diagnostics_menu_items[0]))

static circe_encoder_state_t s_triple_enc;

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
static circe_color_picker_t s_color_picker;
static circe_regulation_result_t s_regulation_result;
static circe_home_wheel_t s_home_wheel;
static circe_selector_t s_selector;
static circe_selector_item_t s_dynamic_selector_items[CIRCE_SELECTOR_MAX_ITEMS];
static char s_dynamic_selector_ids[CIRCE_SELECTOR_MAX_ITEMS][48];
static circe_reflection_t s_reflection;
static circe_photo_result_t s_photo_result;
static circe_memory_browser_t s_memory_browser;
static struct {
    bool active;
    int selected;
    int count;
} s_pattern_browser;
static circe_patterns_result_t s_patterns_result;
static circe_body_map_summary_t s_body_map_summary;
static struct {
    bool active;
    int selected;
} s_body_map_browser;
static circe_timeline_category_t s_memory_category = CIRCE_TIMELINE_CAT_TODAY;
static bool s_memory_context;
static bool s_delete_from_memory;
static circe_flow_step_t s_worker_origin_step;
static bool s_worker_origin_valid;
static circe_storage_health_t s_diag_health;
static bool s_diag_health_valid;

static void dispatch_action(const char *id);
static void btn_event_cb(lv_event_t *e);
static void go_home_safe(void);
static void handle_selector_action(int enc_action);
static void ui_show_status_key(circe_pattern_key_t key);
static void ui_show_status_timed(circe_pattern_key_t key, uint32_t ms);
static void worker_mark_origin(void);
static void worker_clear_origin(void);
static bool worker_apply_navigation(circe_worker_cmd_type_t type);
static void setup_selector_menu(const char *title, const char *status_line, circe_flow_step_t back_step,
                                const circe_selector_item_t *items, int count);

static int build_body_area_selector(circe_flow_step_t back_step)
{
    int n = 0;
    for (int i = 0; i < circe_body_area_count && n < CIRCE_SELECTOR_MAX_ITEMS - 1; i++) {
        s_dynamic_selector_items[n].label = circe_body_areas[i];
        snprintf(s_dynamic_selector_ids[n], sizeof(s_dynamic_selector_ids[n]), "area:%s", circe_body_areas[i]);
        s_dynamic_selector_items[n].action_id = s_dynamic_selector_ids[n];
        n++;
    }
    s_dynamic_selector_items[n].label = circe_copy_get(CIRCE_PATTERN_NAV_BACK);
    snprintf(s_dynamic_selector_ids[n], sizeof(s_dynamic_selector_ids[n]), "nav:%d", (int)back_step);
    s_dynamic_selector_items[n].action_id = s_dynamic_selector_ids[n];
    return n + 1;
}

static int build_body_sensation_selector(circe_flow_step_t back_step)
{
    int n = 0;
    for (int i = 0; i < circe_body_sensation_count && n < CIRCE_SELECTOR_MAX_ITEMS - 1; i++) {
        s_dynamic_selector_items[n].label = circe_body_sensations[i];
        snprintf(s_dynamic_selector_ids[n], sizeof(s_dynamic_selector_ids[n]), "sen:%s", circe_body_sensations[i]);
        s_dynamic_selector_items[n].action_id = s_dynamic_selector_ids[n];
        n++;
    }
    s_dynamic_selector_items[n].label = circe_copy_get(CIRCE_PATTERN_NAV_BACK);
    snprintf(s_dynamic_selector_ids[n], sizeof(s_dynamic_selector_ids[n]), "nav:%d", (int)back_step);
    s_dynamic_selector_items[n].action_id = s_dynamic_selector_ids[n];
    return n + 1;
}

static int build_tone_selector(circe_flow_step_t back_step)
{
    int n = 0;
    for (int i = 0; i < circe_emotion_tone_count && n < CIRCE_SELECTOR_MAX_ITEMS - 2; i++) {
        s_dynamic_selector_items[n].label = circe_emotion_tones[i].label;
        snprintf(s_dynamic_selector_ids[n], sizeof(s_dynamic_selector_ids[n]), "tone:%d", i);
        s_dynamic_selector_items[n].action_id = s_dynamic_selector_ids[n];
        n++;
    }
    s_dynamic_selector_items[n].label = "SKIP";
    s_dynamic_selector_items[n].action_id = "tone_skip";
    n++;
    s_dynamic_selector_items[n].label = circe_copy_get(CIRCE_PATTERN_NAV_BACK);
    snprintf(s_dynamic_selector_ids[n], sizeof(s_dynamic_selector_ids[n]), "nav:%d", (int)back_step);
    s_dynamic_selector_items[n].action_id = s_dynamic_selector_ids[n];
    return n + 1;
}

static int build_theme_selector(void)
{
    int n = circe_theme_count();
    if (n > CIRCE_SELECTOR_MAX_ITEMS - 1) {
        n = CIRCE_SELECTOR_MAX_ITEMS - 1;
    }
    for (int i = 0; i < n; i++) {
        const circe_theme_palette_t *p = circe_theme_get_palette_by_id((circe_theme_id_t)i);
        s_dynamic_selector_items[i].label = p->display_name;
        snprintf(s_dynamic_selector_ids[i], sizeof(s_dynamic_selector_ids[i]), "theme_pick:%d", i);
        s_dynamic_selector_items[i].action_id = s_dynamic_selector_ids[i];
    }
    s_dynamic_selector_items[n].label = circe_copy_get(CIRCE_PATTERN_NAV_BACK);
    s_dynamic_selector_items[n].action_id = "nav:31";
    return n + 1;
}

static int build_quick_selector(void)
{
    int n = 0;
    for (int i = 0; i < CIRCE_QUICK_PRESET_COUNT && n < CIRCE_SELECTOR_MAX_ITEMS - 1; i++) {
        s_dynamic_selector_items[n].label = circe_quick_presets[i].label;
        snprintf(s_dynamic_selector_ids[n], sizeof(s_dynamic_selector_ids[n]), "quick:%d", i);
        s_dynamic_selector_items[n].action_id = s_dynamic_selector_ids[n];
        n++;
    }
    s_dynamic_selector_items[n].label = circe_copy_get(CIRCE_PATTERN_NAV_BACK);
    s_dynamic_selector_items[n].action_id = "nav:0";
    return n + 1;
}

static void strand_note_saved_color(const char *color_hex)
{
    if (!color_hex || color_hex[0] != '#') {
        return;
    }
    circe_strand_cache_append_color(color_hex);
}
static void go_step(circe_flow_step_t step);
static void apply_theme_to_shell(void);
static void format_entry_body_line(char *buf, size_t len, const circe_entry_t *e)
{
    if (!buf || len == 0 || !e) {
        return;
    }
    char areas[96] = {0};
    for (int i = 0; i < e->body_area_count && i < 2; i++) {
        if (i > 0) {
            strncat(areas, " ", sizeof(areas) - strlen(areas) - 1);
        }
        strncat(areas, e->body_areas[i], sizeof(areas) - strlen(areas) - 1);
    }
    char sens[96] = {0};
    for (int i = 0; i < e->body_sensation_count && i < 2; i++) {
        if (i > 0) {
            strncat(sens, " ", sizeof(sens) - strlen(sens) - 1);
        }
        strncat(sens, e->body_sensations[i], sizeof(sens) - strlen(sens) - 1);
    }
    if (areas[0] && sens[0]) {
        snprintf(buf, len, "body %s / %s / %d", areas, sens, e->intensity);
    } else {
        snprintf(buf, len, "body areas %d intensity %d", e->body_area_count, e->intensity);
    }
}

static void format_entry_tone_line(char *buf, size_t len, const circe_entry_t *e)
{
    if (!buf || len == 0 || !e) {
        return;
    }
    if (e->emotion_skipped || !e->emotion_label[0]) {
        snprintf(buf, len, "tone UNKNOWN");
    } else {
        snprintf(buf, len, "tone %s", e->emotion_label);
    }
}

static void format_entry_color_line(char *buf, size_t len, const circe_entry_t *e)
{
    if (!buf || len == 0 || !e) {
        return;
    }
    if (e->color_skipped || e->color_hex[0] == '\0') {
        snprintf(buf, len, "color SKIPPED");
        return;
    }
    char traits[48];
    circe_color_intel_format_review_traits(e, traits, sizeof(traits));
    if (traits[0]) {
        if (strcmp(e->color_label, "CUSTOM") == 0) {
            snprintf(buf, len, "color CUSTOM %s / %s", e->color_hex, traits);
        } else if (e->color_label[0]) {
            snprintf(buf, len, "color %s %s / %s", e->color_label, e->color_hex, traits);
        } else {
            snprintf(buf, len, "color %s / %s", e->color_hex, traits);
        }
        return;
    }
    if (strcmp(e->color_label, "CUSTOM") == 0) {
        snprintf(buf, len, "color CUSTOM %s", e->color_hex);
    } else if (e->color_label[0]) {
        snprintf(buf, len, "color %s %s", e->color_label, e->color_hex);
    } else {
        snprintf(buf, len, "color %s", e->color_hex);
    }
}

static bool entry_is_regulation(const circe_entry_t *e)
{
    return e && (e->entry_mode == CIRCE_ENTRY_MODE_REGULATION || e->has_regulation);
}

static bool photo_offer_eligible(void)
{
    return s_engine && circe_photo_entry_eligible(&s_engine->draft) && !s_engine->editing_existing;
}

static void format_regulation_type_label(const char *type, char *buf, size_t len)
{
    if (!buf || len == 0) {
        return;
    }
    if (!type || !type[0]) {
        snprintf(buf, len, "SESSION");
        return;
    }
    if (strcmp(type, "grounding_54321") == 0) {
        snprintf(buf, len, "5-4-3-2-1");
    } else if (strcmp(type, "sensory_reset") == 0) {
        snprintf(buf, len, "SENSORY RESET");
    } else if (strcmp(type, "bilateral_tap") == 0) {
        snprintf(buf, len, "BILATERAL TAP");
    } else if (strcmp(type, "body_anchor") == 0) {
        snprintf(buf, len, "BODY ANCHOR");
    } else if (strcmp(type, "breathing") == 0) {
        snprintf(buf, len, "BREATHING");
    } else {
        snprintf(buf, len, "%s", type);
    }
}

static void show_entry_summary_feed(const circe_entry_t *e)
{
    char l1[72];
    char l2[48];
    char l3[56];
    if (e->entry_mode == CIRCE_ENTRY_MODE_REGULATION || e->has_regulation) {
        char type_label[32];
        format_regulation_type_label(e->regulation_type, type_label, sizeof(type_label));
        snprintf(l1, sizeof(l1), "regulation %s", type_label);
        if (e->regulation_steps_completed > 0) {
            snprintf(l2, sizeof(l2), "duration %ds / %d steps", e->regulation_duration_seconds,
                     e->regulation_steps_completed);
        } else if (strcmp(e->regulation_type, "bilateral_tap") == 0) {
            snprintf(l2, sizeof(l2), "duration %ds / %d cycles", e->regulation_duration_seconds,
                     e->regulation_rounds_completed);
        } else {
            snprintf(l2, sizeof(l2), "duration %ds rounds %d", e->regulation_duration_seconds,
                     e->regulation_rounds_completed);
        }
        snprintf(l3, sizeof(l3), "completed %s", e->regulation_session_completed ? "yes" : "no");
    } else {
        format_entry_body_line(l1, sizeof(l1), e);
        format_entry_tone_line(l2, sizeof(l2), e);
        if (e->photo_attached) {
            snprintf(l3, sizeof(l3), "%s", circe_copy_get(CIRCE_PATTERN_PHOTO_ATTACHED));
        } else {
            format_entry_color_line(l3, sizeof(l3), e);
        }
    }
    const char *lines[] = {l1, l2, l3};
    circe_terminal_feed_set(&s_feed, lines, 3);
    circe_terminal_feed_show_cursor(&s_feed, true);
}

static void time_picker_done(bool saved, void *ctx);
static void show_terminal_prompt_text(const char *text);
static bool worker_post_or_busy(bool (*post_fn)(void));
static bool post_load_review(void);
static bool post_load_timeline(circe_timeline_category_t category);
static bool post_load_memory_entry(void);
static bool post_timeline_today(void);
static bool post_timeline_yesterday(void);
static bool post_timeline_week(void);
static bool post_timeline_all(void);
static bool post_load_patterns(void);
static void pattern_browser_begin(int count);
static void pattern_browser_refresh(void);
static void pattern_browser_poll(void);
static void body_map_browser_begin(void);
static void body_map_browser_refresh(void);
static void body_map_browser_poll(void);
static bool post_load_body_map(void);
static void open_memory_menu(void);
static void home_wheel_open_selected(void);

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
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_STATUS_CHECKING));
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
    circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_STATUS_CHECKING));
    diagnostics_show_health_feed();
    return circe_worker_post_health_check();
}

static bool post_probe(void)
{
    circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_STATUS_PROBING));
    if (!circe_worker_post_storage_probe()) {
        return false;
    }
    worker_mark_origin();
    return true;
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
    if (circe_home_bg_is_enabled()) {
        circe_hud_show_static_bg_home(&s_hud);
    } else {
        circe_hud_show_terminal_shell(&s_hud, "CIRCE", "online");
    }
}

static void apply_default_home_feed(void)
{
    if (!circe_storage_is_ready()) {
        char l1[64];
        char l2[64];
        snprintf(l1, sizeof(l1), "> %s", circe_copy_get(CIRCE_PATTERN_STORAGE_UNAVAILABLE_1));
        snprintf(l2, sizeof(l2), "> %s", circe_copy_get(CIRCE_PATTERN_STORAGE_UNAVAILABLE_2));
        const char *warn[] = {l1, l2};
        circe_terminal_feed_set(&s_feed, warn, 2);
        circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_HOME_WHEEL_HINT));
        return;
    }
    char feed_line[72];
    snprintf(feed_line, sizeof(feed_line), "> %s", circe_copy_get(CIRCE_PATTERN_HOME_FEED_READY));
    const char *lines[] = {feed_line};
    circe_terminal_feed_set(&s_feed, lines, 1);
    circe_hud_set_subline(&s_hud, "");
}

static void apply_daily_companion_feed(const circe_daily_summary_t *summary)
{
    if (!summary || !summary->valid || !s_engine || s_engine->step != CIRCE_FLOW_HOME) {
        return;
    }
    if (!s_feed.panel) {
        return;
    }
    char l1[72];
    char l2[72];
    snprintf(l1, sizeof(l1), "> %s", summary->primary_line);
    if (summary->subline[0]) {
        snprintf(l2, sizeof(l2), "> %s", summary->subline);
        const char *lines[] = {l1, l2};
        circe_terminal_feed_set(&s_feed, lines, 2);
    } else {
        const char *lines[] = {l1};
        circe_terminal_feed_set(&s_feed, lines, 1);
    }
    circe_hud_set_subline(&s_hud, "");
}

static bool post_load_daily_companion(void)
{
    if (!circe_storage_is_ready()) {
        return false;
    }
    if (circe_worker_is_busy()) {
        return false;
    }
    return circe_worker_post_load_daily_companion();
}

static void clear_content(void)
{
    s_btn_id_next = 0;
    s_row_label_next = 0;
    s_btn_stack_idx = 0;
    s_first_row = NULL;
    s_slider = NULL;
    circe_time_picker_destroy(&s_time_picker);
    circe_color_picker_destroy(&s_color_picker);
    circe_regulation_destroy();
    circe_home_wheel_destroy(&s_home_wheel);
    circe_selector_destroy(&s_selector);
    circe_memory_browser_destroy(&s_memory_browser);
    circe_encoder_state_reset(&s_triple_enc);
    s_pattern_browser.active = false;
    s_pattern_browser.count = 0;
    s_pattern_browser.selected = 0;
    s_body_map_browser.active = false;
    s_body_map_browser.selected = 0;
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

static void regulation_action_cb(int action, void *ctx)
{
    (void)ctx;
    if (action == CIRCE_REG_ACT_BACK) {
        circe_regulation_result_clear(&s_regulation_result);
        go_step(CIRCE_FLOW_GROUNDING);
        return;
    }
    go_step(CIRCE_FLOW_REGULATION_SAVE);
}

static void home_wheel_open_selected(void)
{
    if (!s_engine) {
        return;
    }
    const char *id = circe_home_wheel_action_id(s_home_wheel.selected);
    if (!id || !id[0]) {
        return;
    }
    if (strcmp(id, "ready_body") == 0) {
        circe_conversation_start_body_only(s_engine);
        go_step(CIRCE_FLOW_BODY_AREA);
    } else if (strcmp(id, "quick") == 0) {
        circe_conversation_start_quick(s_engine);
        go_step(CIRCE_FLOW_QUICK_PICK);
    } else if (strcmp(id, "regulate") == 0) {
        circe_regulation_result_clear(&s_regulation_result);
        go_step(CIRCE_FLOW_GROUNDING);
    } else if (strcmp(id, "review") == 0) {
        open_memory_menu();
    } else if (strcmp(id, "more") == 0) {
        go_step(CIRCE_FLOW_MORE);
    } else if (strcmp(id, "diagnostics") == 0) {
        go_step(CIRCE_FLOW_DIAGNOSTICS);
    }
}

static void nav_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_engine && s_engine->step != CIRCE_FLOW_HOME && !s_home_wheel.active && !s_selector.active &&
        !s_time_picker.active) {
        int triple_action =
            circe_encoder_poll(&s_triple_enc, circe_encoder_read_diff(), circe_encoder_read_pressed());
        if (triple_action == CIRCE_ENC_ACTION_TRIPLE) {
            go_home_safe();
            return;
        }
    }
    if (s_time_picker.active) {
        circe_time_picker_poll(&s_time_picker, time_picker_done, &s_hud);
    } else if (s_home_wheel.active) {
        int action = CIRCE_HOME_WHEEL_ACTION_NONE;
        circe_home_wheel_poll(&s_home_wheel, &action);
        if (action == CIRCE_HOME_WHEEL_ACTION_OPEN) {
            home_wheel_open_selected();
        } else if (action == CIRCE_HOME_WHEEL_ACTION_MORE) {
            go_step(CIRCE_FLOW_MORE);
        } else if (action == CIRCE_HOME_WHEEL_ACTION_DOUBLE) {
            /* no-op on home */
        } else if (action == CIRCE_HOME_WHEEL_ACTION_TRIPLE) {
            go_home_safe();
        }
    } else if (s_selector.active) {
        handle_selector_action(circe_selector_poll(&s_selector));
    } else if (s_pattern_browser.active) {
        int prev = s_pattern_browser.selected;
        pattern_browser_poll();
        if (s_pattern_browser.selected != prev) {
            pattern_browser_refresh();
        }
    } else if (s_body_map_browser.active) {
        int prev = s_body_map_browser.selected;
        body_map_browser_poll();
        if (s_body_map_browser.selected != prev) {
            body_map_browser_refresh();
        }
    } else if (s_memory_browser.active) {
        int prev = s_memory_browser.selected;
        int action = CIRCE_MEMORY_BROWSER_ACTION_NONE;
        circe_memory_browser_poll(&s_memory_browser, &action);
        if (s_memory_browser.selected != prev) {
            circe_memory_browser_refresh(&s_memory_browser, &s_feed, &s_hud);
        }
        if (action == CIRCE_MEMORY_BROWSER_ACTION_OPEN) {
            worker_post_or_busy(post_load_memory_entry);
        }
    } else if (circe_regulation_active()) {
        circe_regulation_poll();
    } else if (s_color_picker.active) {
        circe_color_picker_poll_encoder(&s_color_picker);
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

static void ui_show_status_key(circe_pattern_key_t key)
{
    circe_status_banner_show(circe_copy_get(key));
    circe_hud_set_subline(&s_hud, "");
}

static void ui_show_status_timed(circe_pattern_key_t key, uint32_t ms)
{
    circe_status_banner_show_timed(circe_copy_get(key), ms);
    circe_hud_set_subline(&s_hud, "");
}

static void worker_mark_origin(void)
{
    if (s_engine) {
        s_worker_origin_step = s_engine->step;
        s_worker_origin_valid = true;
    }
}

static void worker_clear_origin(void)
{
    s_worker_origin_valid = false;
}

static bool worker_apply_navigation(circe_worker_cmd_type_t type)
{
    if (s_worker_origin_valid && s_engine && s_engine->step != s_worker_origin_step) {
        ESP_LOGW(TAG, "ignore stale worker navigation type=%d origin=%d current=%d", (int)type,
                 (int)s_worker_origin_step, (int)s_engine->step);
        circe_status_banner_reset();
        worker_clear_origin();
        return false;
    }
    worker_clear_origin();
    circe_status_banner_dismiss_indefinite();
    return true;
}

static void go_home_safe(void)
{
    circe_regulation_destroy();
    circe_status_banner_reset();
    worker_clear_origin();
    s_memory_context = false;
    s_delete_from_memory = false;
    go_step(CIRCE_FLOW_HOME);
}

static void setup_selector_menu(const char *title, const char *status_line, circe_flow_step_t back_step,
                                const circe_selector_item_t *items, int count)
{
    setup_terminal_shell(back_step, status_line ? status_line : title);
    show_terminal_prompt_text(title);
    s_nav_back_step = back_step;
    circe_terminal_nav_enable(false);
    circe_selector_create(&s_selector, s_content, title, items, count, 0);
}

static void handle_selector_action(int enc_action)
{
    if (enc_action == CIRCE_SELECTOR_ACTION_TRIPLE) {
        go_home_safe();
        return;
    }
    if (enc_action == CIRCE_SELECTOR_ACTION_DOUBLE) {
        go_step(s_nav_back_step);
        return;
    }
    if (enc_action == CIRCE_SELECTOR_ACTION_LONG) {
        go_step(CIRCE_FLOW_MORE);
        return;
    }
    if (enc_action == CIRCE_SELECTOR_ACTION_SELECT) {
        const char *id = circe_selector_selected_action(&s_selector);
        if (id && id[0]) {
            dispatch_action(id);
        }
    }
}

static void worker_busy_notice(void)
{
    ui_show_status_key(CIRCE_PATTERN_STATUS_LOADING_BANNER);
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
    ui_show_status_key(CIRCE_PATTERN_STATUS_SAVING_BANNER);
    if (!circe_worker_post_save_entry(&s_engine->draft, s_engine->editing_existing, on_success, msg_key,
                                      quick_subline)) {
        circe_status_banner_reset();
        ui_show_status_timed(CIRCE_PATTERN_STATUS_LOADING_BANNER, 1200);
        return false;
    }
    worker_mark_origin();
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
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        ui_show_status_timed(CIRCE_PATTERN_STATUS_ENTRY_SAVED, 1000);
        circe_hud_set_subline(&s_hud, c->summary);
        go_step(CIRCE_FLOW_DIAGNOSTICS);
        break;
    case CIRCE_WORKER_SAVE_ENTRY:
        if (circe_save_result_is_success(c->save_result)) {
            if (!worker_apply_navigation(c->type)) {
                break;
            }
            ui_show_status_timed(CIRCE_PATTERN_STATUS_ENTRY_SAVED, 1000);
            strncpy(s_review_id, c->entry_id, sizeof(s_review_id) - 1);
            s_review_id[sizeof(s_review_id) - 1] = '\0';
            s_engine->editing_existing = false;
            if (c->saved_color[0] == '#' && !c->entry.color_skipped) {
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
            if (c->success_step == CIRCE_FLOW_REFLECTION) {
                circe_reflection_generate(&c->entry, &s_reflection);
                circe_voice_play_event(CIRCE_VOICE_EVENT_SAVE_OK);
                go_step(CIRCE_FLOW_REFLECTION);
            } else {
                go_step(c->success_step);
            }
        } else {
            circe_status_banner_reset();
            worker_clear_origin();
            show_save_error(c->save_result);
        }
        break;
    case CIRCE_WORKER_PHOTO_CAPTURE:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        s_engine->draft = c->entry;
        s_photo_result = c->photo_result;
        go_step(CIRCE_FLOW_PHOTO_RESULT);
        break;
    case CIRCE_WORKER_DELETE_ENTRY:
        if (c->success) {
            if (!worker_apply_navigation(c->type)) {
                break;
            }
            s_review_id[0] = '\0';
            show_message(CIRCE_PATTERN_DELETE_DONE);
            s_engine->editing_existing = false;
            if (s_delete_from_memory) {
                s_delete_from_memory = false;
                s_memory_context = false;
                if (!post_load_timeline(s_memory_category)) {
                    worker_busy_notice();
                    go_step(CIRCE_FLOW_MEMORY_MENU);
                }
            } else {
                go_step(CIRCE_FLOW_HOME);
            }
        } else {
            circe_status_banner_reset();
            worker_clear_origin();
            circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_STATUS_DELETE_FAILED));
        }
        break;
    case CIRCE_WORKER_REBUILD_INDEX:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        circe_hud_set_subline(&s_hud, c->summary);
        go_step(CIRCE_FLOW_DIAGNOSTICS);
        break;
    case CIRCE_WORKER_REINIT_STORAGE:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        s_engine->storage_ready = c->storage_ready;
        circe_hud_set_subline(&s_hud, c->summary);
        go_step(CIRCE_FLOW_DIAGNOSTICS);
        break;
    case CIRCE_WORKER_STORAGE_PROBE:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        circe_hud_set_subline(&s_hud, c->summary);
        go_step(CIRCE_FLOW_DIAGNOSTICS);
        break;
    case CIRCE_WORKER_HEALTH_CHECK:
    case CIRCE_WORKER_STORAGE_STATUS:
    case CIRCE_WORKER_DIAGNOSTICS_REFRESH:
        worker_clear_origin();
        circe_status_banner_dismiss_indefinite();
        diagnostics_apply_health(&c->health);
        circe_hud_set_subline(&s_hud, c->summary);
        break;
    case CIRCE_WORKER_LOAD_REVIEW:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        if (c->review_found && c->success) {
            s_engine->draft = c->entry;
            strncpy(s_review_id, c->entry_id, sizeof(s_review_id) - 1);
            s_review_id[sizeof(s_review_id) - 1] = '\0';
            s_memory_context = false;
            go_step(CIRCE_FLOW_REVIEW);
        } else {
            go_step(CIRCE_FLOW_MEMORY_MENU);
        }
        break;
    case CIRCE_WORKER_LOAD_TIMELINE:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        if (c->timeline_index_error) {
            go_step(CIRCE_FLOW_MEMORY_ERROR);
        } else if (c->timeline_empty) {
            go_step(CIRCE_FLOW_MEMORY_EMPTY);
        } else {
            go_step(CIRCE_FLOW_MEMORY_BROWSE);
        }
        break;
    case CIRCE_WORKER_LOAD_ENTRY:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        if (c->success) {
            s_engine->draft = c->entry;
            s_engine->editing_existing = true;
            strncpy(s_review_id, c->entry_id, sizeof(s_review_id) - 1);
            s_review_id[sizeof(s_review_id) - 1] = '\0';
            s_memory_context = true;
            go_step(CIRCE_FLOW_REVIEW);
        } else {
            circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_STATUS_LOAD_FAILED));
        }
        break;
    case CIRCE_WORKER_LOAD_PATTERNS:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        s_patterns_result = c->patterns;
        if (c->patterns.storage_unavailable) {
            go_step(CIRCE_FLOW_PATTERNS_ERROR);
        } else if (!c->success || c->patterns.index_error) {
            go_step(CIRCE_FLOW_PATTERNS_ERROR);
        } else if (c->patterns.not_enough_entries) {
            go_step(CIRCE_FLOW_PATTERNS_EMPTY);
        } else if (c->patterns.no_patterns || c->patterns.count <= 0) {
            go_step(CIRCE_FLOW_PATTERNS_NONE);
        } else {
            go_step(CIRCE_FLOW_PATTERNS);
        }
        break;
    case CIRCE_WORKER_LOAD_DAILY_COMPANION:
        worker_clear_origin();
        if (s_engine->step == CIRCE_FLOW_HOME) {
            apply_daily_companion_feed(&c->daily);
        }
        break;
    case CIRCE_WORKER_LOAD_BODY_MAP:
        if (!worker_apply_navigation(c->type)) {
            break;
        }
        s_body_map_summary = c->body_map;
        if (s_body_map_summary.state == CIRCE_BODY_MAP_STATE_STORAGE) {
            go_step(CIRCE_FLOW_BODY_MAP_ERROR);
        } else if (s_body_map_summary.state == CIRCE_BODY_MAP_STATE_ERROR) {
            go_step(CIRCE_FLOW_BODY_MAP_ERROR);
        } else if (s_body_map_summary.state == CIRCE_BODY_MAP_STATE_EMPTY) {
            go_step(CIRCE_FLOW_BODY_MAP_EMPTY);
        } else {
            go_step(CIRCE_FLOW_BODY_MAP);
        }
        break;
    default:
        worker_clear_origin();
        circe_status_banner_dismiss_indefinite();
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
    ui_show_status_key(CIRCE_PATTERN_STATUS_SAVING_BANNER);
    if (!circe_worker_post_test_save()) {
        circe_status_banner_reset();
        return false;
    }
    worker_mark_origin();
    return true;
}

static bool post_rebuild(void)
{
    circe_hud_set_subline(&s_hud, "Rebuilding...");
    if (!circe_worker_post_rebuild_index()) {
        return false;
    }
    worker_mark_origin();
    return true;
}

static bool post_reinit(void)
{
    circe_hud_set_subline(&s_hud, "Reinitializing...");
    if (!circe_worker_post_reinit_storage()) {
        return false;
    }
    worker_mark_origin();
    return true;
}

static void open_memory_menu(void)
{
    s_memory_context = false;
    s_delete_from_memory = false;
    go_step(CIRCE_FLOW_MEMORY_MENU);
}

static bool post_load_review(void)
{
    open_memory_menu();
    return true;
}

static bool post_load_timeline(circe_timeline_category_t category)
{
    s_memory_category = category;
    ui_show_status_key(CIRCE_PATTERN_STATUS_LOADING_BANNER);
    if (!circe_worker_post_load_timeline(category)) {
        circe_status_banner_reset();
        return false;
    }
    worker_mark_origin();
    return true;
}

static bool post_load_memory_entry(void)
{
    const char *id = circe_memory_browser_selected_id(&s_memory_browser);
    if (!id || !id[0]) {
        return false;
    }
    ui_show_status_key(CIRCE_PATTERN_STATUS_LOADING_BANNER);
    if (!circe_worker_post_load_entry(id)) {
        circe_status_banner_reset();
        return false;
    }
    worker_mark_origin();
    return true;
}

static bool post_timeline_today(void)
{
    return post_load_timeline(CIRCE_TIMELINE_CAT_TODAY);
}

static bool post_timeline_yesterday(void)
{
    return post_load_timeline(CIRCE_TIMELINE_CAT_YESTERDAY);
}

static bool post_timeline_week(void)
{
    return post_load_timeline(CIRCE_TIMELINE_CAT_THIS_WEEK);
}

static bool post_timeline_all(void)
{
    return post_load_timeline(CIRCE_TIMELINE_CAT_ALL);
}

static bool post_load_patterns(void)
{
    ui_show_status_key(CIRCE_PATTERN_PATTERNS_LOADING);
    if (!circe_worker_post_load_patterns()) {
        circe_status_banner_reset();
        return false;
    }
    worker_mark_origin();
    return true;
}

static bool post_load_body_map(void)
{
    ui_show_status_key(CIRCE_PATTERN_BODY_MAP_LOADING);
    if (!circe_worker_post_load_body_map()) {
        circe_status_banner_reset();
        return false;
    }
    worker_mark_origin();
    return true;
}

static void pattern_browser_begin(int count)
{
    s_pattern_browser.active = count > 0;
    s_pattern_browser.count = count > 0 ? count : 0;
    s_pattern_browser.selected = 0;
}

static void pattern_browser_refresh(void)
{
    if (!s_pattern_browser.active || s_pattern_browser.count <= 0) {
        return;
    }
    if (s_pattern_browser.selected < 0) {
        s_pattern_browser.selected = 0;
    }
    if (s_pattern_browser.selected >= s_pattern_browser.count) {
        s_pattern_browser.selected = s_pattern_browser.count - 1;
    }
    const circe_pattern_summary_t *p = &s_patterns_result.items[s_pattern_browser.selected];
    char l1[96];
    char l2[96];
    snprintf(l1, sizeof(l1), "%s", p->primary[0] ? p->primary : circe_copy_get(CIRCE_PATTERN_PATTERNS_NONE_1));
    snprintf(l2, sizeof(l2), "%s",
             p->subline[0] ? p->subline : circe_copy_get(CIRCE_PATTERN_PATTERNS_OBSERVATION_SUBLINE));
    const char *lines[] = {l1, l2};
    circe_terminal_feed_set(&s_feed, lines, 2);
    circe_terminal_feed_show_cursor(&s_feed, true);
    char sub[48];
    snprintf(sub, sizeof(sub), "%d / %d", s_pattern_browser.selected + 1, s_pattern_browser.count);
    circe_hud_set_subline(&s_hud, sub);
}

static void pattern_browser_poll(void)
{
    if (!s_pattern_browser.active || s_pattern_browser.count <= 0) {
        return;
    }
    lv_indev_t *enc = NULL;
    while ((enc = lv_indev_get_next(enc)) != NULL) {
        if (enc->driver->type != LV_INDEV_TYPE_ENCODER) {
            continue;
        }
        lv_indev_data_t data;
        lv_memset_00(&data, sizeof(data));
        enc->driver->read_cb(enc->driver, &data);
        if (data.enc_diff != 0) {
            s_pattern_browser.selected += data.enc_diff;
            while (s_pattern_browser.selected < 0) {
                s_pattern_browser.selected += s_pattern_browser.count;
            }
            while (s_pattern_browser.selected >= s_pattern_browser.count) {
                s_pattern_browser.selected -= s_pattern_browser.count;
            }
        }
        break;
    }
}

static void body_map_browser_begin(void)
{
    s_body_map_browser.active = s_body_map_summary.row_count > 0;
    s_body_map_browser.selected = 0;
}

static void body_map_browser_refresh(void)
{
    if (!s_body_map_browser.active || s_body_map_summary.row_count <= 0) {
        return;
    }
    if (s_body_map_browser.selected < 0) {
        s_body_map_browser.selected = 0;
    }
    if (s_body_map_browser.selected >= s_body_map_summary.row_count) {
        s_body_map_browser.selected = s_body_map_summary.row_count - 1;
    }

    int max_score = s_body_map_summary.rows[0].score;
    char line_bufs[CIRCE_TERMINAL_FEED_LINES][48];
    const char *lines[CIRCE_TERMINAL_FEED_LINES];
    int n = 0;
    for (int i = 0; i < s_body_map_summary.row_count && n < CIRCE_TERMINAL_FEED_LINES; i++) {
        char row[40];
        circe_body_map_format_row(row, sizeof(row), &s_body_map_summary.rows[i], max_score);
        snprintf(line_bufs[n], sizeof(line_bufs[n]), "> %s", row);
        lines[n] = line_bufs[n];
        n++;
    }
    circe_terminal_feed_set(&s_feed, lines, n);
    circe_terminal_feed_show_cursor(&s_feed, true);

    char detail[64];
    circe_body_map_format_detail(detail, sizeof(detail), &s_body_map_summary.rows[s_body_map_browser.selected]);
    circe_hud_set_subline(&s_hud, detail[0] ? detail : circe_copy_get(CIRCE_PATTERN_BODY_MAP_OBSERVATION));
}

static void body_map_browser_poll(void)
{
    if (!s_body_map_browser.active || s_body_map_summary.row_count <= 0) {
        return;
    }
    lv_indev_t *enc = NULL;
    while ((enc = lv_indev_get_next(enc)) != NULL) {
        if (enc->driver->type != LV_INDEV_TYPE_ENCODER) {
            continue;
        }
        lv_indev_data_t data;
        lv_memset_00(&data, sizeof(data));
        enc->driver->read_cb(enc->driver, &data);
        if (data.enc_diff != 0) {
            s_body_map_browser.selected += data.enc_diff;
            while (s_body_map_browser.selected < 0) {
                s_body_map_browser.selected += s_body_map_summary.row_count;
            }
            while (s_body_map_browser.selected >= s_body_map_summary.row_count) {
                s_body_map_browser.selected -= s_body_map_summary.row_count;
            }
        }
        break;
    }
}

static bool post_delete_review(void)
{
    ui_show_status_key(CIRCE_PATTERN_STATUS_DELETING_BANNER);
    if (!circe_worker_post_delete_entry(s_review_id)) {
        circe_status_banner_reset();
        return false;
    }
    worker_mark_origin();
    return true;
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
    dispatch_action((const char *)lv_event_get_user_data(e));
}

static void dispatch_action(const char *id)
{
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
    } else if (strcmp(id, "regulate") == 0) {
        circe_regulation_result_clear(&s_regulation_result);
        go_step(CIRCE_FLOW_GROUNDING);
    } else if (strcmp(id, "reg_breathing") == 0) {
        go_step(CIRCE_FLOW_BREATHING);
    } else if (strcmp(id, "reg_anchor") == 0) {
        go_step(CIRCE_FLOW_BODY_ANCHOR);
    } else if (strcmp(id, "reg_54321") == 0) {
        go_step(CIRCE_FLOW_REG_54321);
    } else if (strcmp(id, "reg_sensory") == 0) {
        go_step(CIRCE_FLOW_SENSORY_RESET);
    } else if (strcmp(id, "reg_bilateral") == 0) {
        go_step(CIRCE_FLOW_BILATERAL_TAP);
    } else if (strcmp(id, "reg_note") == 0) {
        circe_conversation_start_body_only(s_engine);
        go_step(CIRCE_FLOW_BODY_AREA);
    } else if (strcmp(id, "reg_save") == 0) {
        circe_regulation_apply_to_entry(&s_engine->draft, &s_regulation_result);
        enqueue_save_async(CIRCE_FLOW_REFLECTION, CIRCE_MSG_NONE, false);
    } else if (strcmp(id, "reg_skip") == 0) {
        circe_regulation_result_clear(&s_regulation_result);
        go_step(CIRCE_FLOW_HOME);
    } else if (strcmp(id, "photo_offer") == 0) {
        go_step(CIRCE_FLOW_PHOTO_CONSENT);
    } else if (strcmp(id, "photo_consent_continue") == 0) {
        circe_photo_mark_consent_given();
        go_step(CIRCE_FLOW_PHOTO_CAPTURE);
    } else if (strcmp(id, "photo_capture") == 0) {
        if (circe_worker_is_busy()) {
            worker_busy_notice();
        } else {
            circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_STATUS_SAVING));
            if (!circe_worker_post_photo_capture(&s_engine->draft)) {
                circe_hud_set_subline(&s_hud, "Photo queue failed");
            }
        }
    } else if (strcmp(id, "photo_skip") == 0) {
        go_step(CIRCE_FLOW_REFLECTION);
    } else if (strcmp(id, "review") == 0) {
        if (s_engine->step == CIRCE_FLOW_PHOTO_RESULT && s_engine->draft.id[0]) {
            go_step(CIRCE_FLOW_REVIEW);
        } else {
            open_memory_menu();
        }
    } else if (strcmp(id, "more") == 0) {
        go_step(CIRCE_FLOW_MORE);
    } else if (strcmp(id, "more_appearance") == 0) {
        s_appearance_pick = circe_theme_get_active();
        go_step(CIRCE_FLOW_APPEARANCE);
    } else if (strcmp(id, "more_time") == 0) {
        go_step(CIRCE_FLOW_TIME);
    } else if (strcmp(id, "more_storage") == 0) {
        go_step(CIRCE_FLOW_DIAGNOSTICS);
    } else if (strcmp(id, "more_voice") == 0) {
        go_step(CIRCE_FLOW_VOICE_CUES);
    } else if (strcmp(id, "voice_off") == 0) {
        circe_voice_set_mode(CIRCE_VOICE_MODE_OFF);
        ui_show_status_timed(CIRCE_PATTERN_VOICE_CUES_OFF, 1000);
        go_step(CIRCE_FLOW_VOICE_CUES);
    } else if (strcmp(id, "voice_soft") == 0) {
        circe_voice_set_mode(CIRCE_VOICE_MODE_SOFT);
        if (!circe_voice_is_available()) {
            ui_show_status_timed(CIRCE_PATTERN_STATUS_AUDIO_UNAVAILABLE, 1500);
        } else {
            ui_show_status_timed(CIRCE_PATTERN_VOICE_ENABLED, 1000);
        }
        go_step(CIRCE_FLOW_VOICE_CUES);
    } else if (strcmp(id, "voice_test") == 0) {
        if (circe_voice_get_mode() != CIRCE_VOICE_MODE_SOFT) {
            ui_show_status_timed(CIRCE_PATTERN_VOICE_CUES_OFF, 1000);
        } else {
            ui_show_status_key(CIRCE_PATTERN_VOICE_PLAYING_TONE);
            if (circe_voice_play_test_tone()) {
                ui_show_status_timed(CIRCE_PATTERN_VOICE_TONE_SENT, 1000);
            } else {
                ui_show_status_timed(CIRCE_PATTERN_STATUS_AUDIO_UNAVAILABLE, 1500);
            }
        }
        go_step(CIRCE_FLOW_VOICE_CUES);
    } else if (strncmp(id, "theme_pick:", 11) == 0) {
        int idx = atoi(id + 11);
        if (idx >= 0 && idx < circe_theme_count()) {
            s_appearance_pick = (circe_theme_id_t)idx;
            circe_theme_preview(s_appearance_pick);
            circe_theme_commit_preview();
            apply_theme_to_shell();
            ui_show_status_timed(CIRCE_PATTERN_APPEARANCE_APPLIED, 1200);
        }
        go_step(CIRCE_FLOW_MORE);
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
        enqueue_save_async(CIRCE_FLOW_REFLECTION, CIRCE_MSG_NONE, false);
    } else if (strcmp(id, "next_intensity") == 0) {
        if (s_slider) {
            s_engine->draft.intensity = lv_slider_get_value(s_slider);
        }
        go_step(CIRCE_FLOW_BODY_ADD_MORE);
    } else if (strcmp(id, "add_sensation") == 0) {
        go_step(CIRCE_FLOW_BODY_AREA);
    } else if (strcmp(id, "continue_save") == 0) {
        go_step(CIRCE_FLOW_EMOTION_TONE);
    } else if (strcmp(id, "tone_skip") == 0) {
        circe_entry_modes_apply_tone(&s_engine->draft, "UNKNOWN", "unknown", true);
        go_step(CIRCE_FLOW_COLOR_PICKER);
    } else if (strncmp(id, "tone:", 5) == 0) {
        int idx = atoi(id + 5);
        if (idx >= 0 && idx < circe_emotion_tone_count) {
            circe_entry_modes_apply_tone(&s_engine->draft, circe_emotion_tones[idx].label,
                                         circe_emotion_tones[idx].value, false);
        }
        if (s_engine->editing_existing) {
            enqueue_save_async(CIRCE_FLOW_REVIEW, CIRCE_PATTERN_EDIT_SAVED, false);
        } else {
            go_step(CIRCE_FLOW_COLOR_PICKER);
        }
    } else if (strcmp(id, "color_save") == 0) {
        if (s_color_picker.active && s_color_picker.hex[0] == '#') {
            circe_entry_modes_apply_color_touch(&s_engine->draft, s_color_picker.hex);
        }
        if (s_engine->editing_existing) {
            enqueue_save_async(CIRCE_FLOW_REVIEW, CIRCE_PATTERN_EDIT_SAVED, false);
        } else {
            go_step(CIRCE_FLOW_SAVE_CONFIRM);
        }
    } else if (strcmp(id, "color_skip") == 0) {
        circe_entry_modes_apply_color_skipped(&s_engine->draft);
        if (s_engine->editing_existing) {
            enqueue_save_async(CIRCE_FLOW_REVIEW, CIRCE_PATTERN_EDIT_SAVED, false);
        } else {
            go_step(CIRCE_FLOW_SAVE_CONFIRM);
        }
    } else if (strcmp(id, "color_presets") == 0) {
        go_step(CIRCE_FLOW_COLOR_PRESETS);
    } else if (strncmp(id, "preset:", 7) == 0) {
        int idx = atoi(id + 7);
        if (idx >= 0 && idx < circe_color_preset_count) {
            circe_entry_modes_apply_color_preset(&s_engine->draft, circe_color_presets[idx].label,
                                                 circe_color_presets[idx].hex);
        }
        if (s_engine->editing_existing) {
            enqueue_save_async(CIRCE_FLOW_REVIEW, CIRCE_PATTERN_EDIT_SAVED, false);
        } else {
            go_step(CIRCE_FLOW_SAVE_CONFIRM);
        }
    } else if (strcmp(id, "confirm_save") == 0) {
        enqueue_save_async(CIRCE_FLOW_REFLECTION, CIRCE_MSG_NONE, false);
    } else if (strcmp(id, "change_color") == 0) {
        go_step(CIRCE_FLOW_COLOR_PICKER);
    } else if (strcmp(id, "change_tone") == 0) {
        go_step(CIRCE_FLOW_EMOTION_TONE);
    } else if (strcmp(id, "mem_today") == 0) {
        worker_post_or_busy(post_timeline_today);
    } else if (strcmp(id, "mem_yesterday") == 0) {
        worker_post_or_busy(post_timeline_yesterday);
    } else if (strcmp(id, "mem_week") == 0) {
        worker_post_or_busy(post_timeline_week);
    } else if (strcmp(id, "mem_all") == 0) {
        worker_post_or_busy(post_timeline_all);
    } else if (strcmp(id, "mem_patterns") == 0) {
        worker_post_or_busy(post_load_patterns);
    } else if (strcmp(id, "mem_body_map") == 0) {
        worker_post_or_busy(post_load_body_map);
    } else if (strcmp(id, "body_map") == 0) {
        worker_post_or_busy(post_load_body_map);
    } else if (strcmp(id, "mem_view") == 0) {
        worker_post_or_busy(post_load_memory_entry);
    } else if (strcmp(id, "mem_delete") == 0) {
        const char *entry_id = circe_memory_browser_selected_id(&s_memory_browser);
        if (entry_id && entry_id[0]) {
            strncpy(s_review_id, entry_id, sizeof(s_review_id) - 1);
            s_review_id[sizeof(s_review_id) - 1] = '\0';
            s_delete_from_memory = true;
            go_step(CIRCE_FLOW_DELETE_CONFIRM);
        }
    } else if (strcmp(id, "delete") == 0) {
        if (s_memory_context) {
            s_delete_from_memory = true;
        }
        go_step(CIRCE_FLOW_DELETE_CONFIRM);
    } else if (strcmp(id, "delete_yes") == 0) {
        if (s_review_id[0]) {
            worker_post_or_busy(post_delete_review);
        }
    } else if (strcmp(id, "edit") == 0) {
        if (!entry_is_regulation(&s_engine->draft)) {
            go_step(CIRCE_FLOW_EDIT);
        }
    } else if (strcmp(id, "edit_color") == 0) {
        s_engine->editing_existing = true;
        go_step(CIRCE_FLOW_COLOR_PICKER);
    } else if (strcmp(id, "edit_tone") == 0) {
        s_engine->editing_existing = true;
        go_step(CIRCE_FLOW_EMOTION_TONE);
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
    } else if (strncmp(id, "quick:", 6) == 0) {
        int preset = id[6] - '0';
        circe_entry_modes_apply_quick_preset(&s_engine->draft, preset);
        enqueue_save_async(CIRCE_FLOW_REFLECTION, CIRCE_PATTERN_QUICK_SAVED, true);
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

static void terminal_nav_triple_home(void *ctx)
{
    (void)ctx;
    go_home_safe();
}

void circe_ui_init(void)
{
    setup_encoder_group();
    circe_worker_init(circe_ui_worker_done, NULL);
    circe_terminal_nav_init(terminal_nav_back, terminal_nav_sysmenu, terminal_nav_triple_home, NULL);

    s_scr = lv_obj_create(NULL);
    lv_scr_load(s_scr);
    circe_theme_apply_screen(s_scr);

    circe_hud_create(s_scr, &s_hud);
    circe_home_bg_init(s_scr);
    circe_status_banner_init(s_scr);
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
    circe_status_banner_dismiss_indefinite();
    if (step != CIRCE_FLOW_HOME) {
        circe_home_bg_hide();
        if (s_hud.safe_ring) {
            lv_obj_clear_flag(s_hud.safe_ring, LV_OBJ_FLAG_HIDDEN);
        }
    }
    setup_encoder_group();
    if (step != CIRCE_FLOW_HOME) {
        circe_hud_set_reset_mode(&s_hud, false);
    }
    apply_theme_to_shell();
    circe_terminal_nav_enable(true);

    switch (step) {
    case CIRCE_FLOW_HOME: {
        circe_home_bg_show();
        show_home();
        if (!s_hud.viewport) {
            ESP_LOGE(TAG, "HOME: viewport null");
            break;
        }
        circe_terminal_feed_init(&s_feed, s_hud.viewport);
        apply_default_home_feed();
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_HOME;
        circe_terminal_nav_enable(false);
        circe_home_wheel_create(&s_home_wheel, s_content, 0);
        post_load_daily_companion();
        circe_home_bg_show();
        break;
    }

    case CIRCE_FLOW_MORE:
        setup_selector_menu("SETTINGS", "settings", CIRCE_FLOW_HOME, s_settings_menu_items, SETTINGS_MENU_COUNT);
        break;

    case CIRCE_FLOW_APPEARANCE:
        setup_selector_menu("APPEARANCE", circe_copy_get(CIRCE_PATTERN_APPEARANCE_PROMPT), CIRCE_FLOW_MORE,
                            s_dynamic_selector_items, build_theme_selector());
        break;

    case CIRCE_FLOW_BODY_AREA: {
        circe_flow_step_t back = s_engine->editing_existing ? CIRCE_FLOW_EDIT : CIRCE_FLOW_HOME;
        setup_terminal_shell(back, "body check-in");
        setup_selector_menu("BODY", circe_copy_get(CIRCE_PATTERN_BODY_AREA_PROMPT), back, s_dynamic_selector_items,
                            build_body_area_selector(back));
        break;
    }

    case CIRCE_FLOW_BODY_SENSATION:
        setup_terminal_shell(CIRCE_FLOW_BODY_AREA, "body sensation");
        setup_selector_menu("SENSATION", circe_copy_get(CIRCE_PATTERN_BODY_SENSATION_PROMPT), CIRCE_FLOW_BODY_AREA,
                            s_dynamic_selector_items, build_body_sensation_selector(CIRCE_FLOW_BODY_AREA));
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

    case CIRCE_FLOW_EMOTION_TONE: {
        circe_flow_step_t back = s_engine->editing_existing ? CIRCE_FLOW_EDIT : CIRCE_FLOW_BODY_ADD_MORE;
        setup_terminal_shell(back, "emotional tone");
        setup_selector_menu("TONE", circe_copy_get(CIRCE_PATTERN_TONE_PROMPT), back, s_dynamic_selector_items,
                            build_tone_selector(back));
        circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_TONE_ROUGH_OK));
        break;
    }

    case CIRCE_FLOW_COLOR_PICKER:
        s_nav_back_step = s_engine->editing_existing ? CIRCE_FLOW_EDIT : CIRCE_FLOW_EMOTION_TONE;
        circe_terminal_nav_set_back_step(s_nav_back_step);
        circe_hud_show_color_field_layout(&s_hud, "color field", circe_copy_get(CIRCE_PATTERN_COLOR_FIELD_HINT));
        s_column = lv_obj_create(s_content);
        lv_obj_set_size(s_column, 300, 352);
        lv_obj_align(s_column, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_opa(s_column, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(s_column, 0, 0);
        lv_obj_set_style_pad_all(s_column, 0, 0);
        lv_obj_clear_flag(s_column, LV_OBJ_FLAG_SCROLLABLE);
        if (s_engine->draft.color_hex[0] == '#') {
            circe_color_picker_create(&s_color_picker, s_column);
            circe_color_picker_set_hex(&s_color_picker, s_engine->draft.color_hex);
            circe_color_picker_refresh(&s_color_picker);
        } else {
            circe_color_picker_create(&s_color_picker, s_column);
        }
        s_scroll = lv_obj_create(s_column);
        lv_obj_set_size(s_scroll, 300, 120);
        lv_obj_align(s_scroll, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_opa(s_scroll, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(s_scroll, 0, 0);
        lv_obj_set_style_pad_all(s_scroll, 0, 0);
        lv_obj_add_flag(s_scroll, LV_OBJ_FLAG_SCROLLABLE);
        add_btn("PRESETS", "color_presets");
        add_btn("SAVE", "color_save");
        add_btn("SKIP", "color_skip");
        add_back_btn(s_nav_back_step);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_COLOR_PRESETS:
        setup_terminal_shell(CIRCE_FLOW_COLOR_PICKER, "color presets");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_COLOR_PRESETS_PROMPT));
        s_nav_back_step = CIRCE_FLOW_COLOR_PICKER;
        create_scroll_panel();
        for (int i = 0; i < circe_color_preset_count; i++) {
            char label[40];
            snprintf(label, sizeof(label), "%s  %s", circe_color_presets[i].label, circe_color_presets[i].hex);
            add_btn(label, alloc_btn_id("preset:%d", i));
        }
        add_back_btn(CIRCE_FLOW_COLOR_PICKER);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_SAVE_CONFIRM:
        setup_terminal_shell(CIRCE_FLOW_COLOR_PICKER, "entry ready");
        show_entry_summary_feed(&s_engine->draft);
        s_nav_back_step = CIRCE_FLOW_COLOR_PICKER;
        add_btn("SAVE", "confirm_save");
        add_btn("CHANGE COLOR", "change_color");
        add_btn("CHANGE TONE", "change_tone");
        add_back_btn(CIRCE_FLOW_COLOR_PICKER);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_COLOR_OPTIONAL:
        go_step(CIRCE_FLOW_EMOTION_TONE);
        break;

    case CIRCE_FLOW_SAVE_DONE:
        setup_terminal_shell(CIRCE_FLOW_HOME, "saved");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_SAVE_CONFIRMED));
        if (!circe_index_is_dirty()) {
            circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE));
        }
        strand_note_saved_color(s_engine->draft.color_skipped ? NULL : s_engine->draft.color_hex);
        s_nav_back_step = CIRCE_FLOW_HOME;
        add_btn("HOME", "home");
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_REFLECTION: {
        setup_terminal_shell(CIRCE_FLOW_HOME, s_reflection.is_regulation ? "session saved" : "saved");
        show_terminal_prompt_text(s_reflection.main_text[0] ? s_reflection.main_text
                                                            : "Saved. I can remember this with you.");
        if (s_reflection.subline[0]) {
            circe_hud_set_subline(&s_hud, s_reflection.subline);
        } else if (!circe_index_is_dirty()) {
            circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_PRIVACY_DEFAULT_NOTICE));
        }
        strand_note_saved_color(s_engine->draft.color_skipped ? NULL : s_engine->draft.color_hex);
        s_nav_back_step = CIRCE_FLOW_HOME;
        if (s_reflection.suggest_regulate) {
            add_btn("REGULATE", "regulate");
        }
        if (photo_offer_eligible()) {
            add_btn("PHOTO", "photo_offer");
        }
        add_btn("REVIEW", "review");
        add_btn("HOME", "home");
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_PHOTO_CONSENT:
        setup_terminal_shell(CIRCE_FLOW_REFLECTION, "photo memory");
        {
            const char *lines[] = {circe_copy_get(CIRCE_PATTERN_PHOTO_TITLE),
                                   circe_copy_get(CIRCE_PATTERN_PHOTO_OPTIONAL),
                                   circe_copy_get(CIRCE_PATTERN_PHOTO_LOCAL)};
            circe_terminal_feed_set(&s_feed, lines, 3);
            circe_terminal_feed_show_cursor(&s_feed, true);
        }
        s_nav_back_step = CIRCE_FLOW_REFLECTION;
        create_scroll_panel();
        add_btn("CONTINUE", "photo_consent_continue");
        add_btn("SKIP", "photo_skip");
        add_back_btn(CIRCE_FLOW_REFLECTION);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_PHOTO_CAPTURE:
        setup_terminal_shell(CIRCE_FLOW_PHOTO_CONSENT, "photo capture");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_PHOTO_TITLE));
        circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_PHOTO_LOCAL));
        s_nav_back_step = CIRCE_FLOW_PHOTO_CONSENT;
        create_scroll_panel();
        add_btn(circe_copy_get(CIRCE_PATTERN_PHOTO_CAPTURE), "photo_capture");
        add_btn(circe_copy_get(CIRCE_PATTERN_PHOTO_SKIP), "photo_skip");
        add_back_btn(CIRCE_FLOW_PHOTO_CONSENT);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_PHOTO_RESULT: {
        setup_terminal_shell(CIRCE_FLOW_REFLECTION, "photo result");
        if (s_photo_result == CIRCE_PHOTO_RESULT_SAVED) {
            show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_PHOTO_SAVED));
        } else if (s_photo_result == CIRCE_PHOTO_RESULT_SAVE_FAILED) {
            show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_PHOTO_FAILED));
        } else {
            show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_PHOTO_UNAVAILABLE));
        }
        circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_PHOTO_ENTRY_STILL_SAVED));
        s_nav_back_step = CIRCE_FLOW_REFLECTION;
        create_scroll_panel();
        add_btn("REVIEW", "review");
        add_btn("HOME", "home");
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_REVIEW: {
        circe_flow_step_t back = s_memory_context ? CIRCE_FLOW_MEMORY_BROWSE : CIRCE_FLOW_HOME;
        setup_terminal_shell(back, s_memory_context ? "memory detail" : "review entry");
        show_entry_summary_feed(&s_engine->draft);
        s_nav_back_step = back;
        if (!entry_is_regulation(&s_engine->draft)) {
            add_btn("EDIT", "edit");
        }
        add_btn("DELETE", "delete");
        if (s_memory_context) {
            add_back_btn(CIRCE_FLOW_MEMORY_BROWSE);
        } else {
            add_btn("HOME", "home");
        }
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_VOICE_CUES:
        setup_selector_menu("VOICE CUES", circe_copy_get(CIRCE_PATTERN_VOICE_TITLE), CIRCE_FLOW_MORE,
                            s_voice_menu_items, VOICE_MENU_COUNT);
        break;

    case CIRCE_FLOW_MEMORY_MENU:
        setup_selector_menu("REVIEW", circe_copy_get(CIRCE_PATTERN_MEMORY_MENU_PROMPT), CIRCE_FLOW_HOME,
                            s_memory_menu_items, MEMORY_MENU_COUNT);
        break;

    case CIRCE_FLOW_PATTERNS:
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, "patterns");
        circe_terminal_nav_enable(true);
        pattern_browser_begin(s_patterns_result.count);
        pattern_browser_refresh();
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        create_scroll_panel();
        if (circe_patterns_any_suggest_regulate(&s_patterns_result)) {
            add_btn("REGULATE", "regulate");
        }
        add_btn("BODY MAP", "body_map");
        add_btn("REVIEW", "review");
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_PATTERNS_EMPTY: {
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, "patterns");
        const char *lines[] = {circe_copy_get(CIRCE_PATTERN_PATTERNS_EMPTY_1),
                               circe_copy_get(CIRCE_PATTERN_PATTERNS_EMPTY_2)};
        circe_terminal_feed_set(&s_feed, lines, 2);
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_PATTERNS_NONE: {
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, "patterns");
        const char *lines[] = {circe_copy_get(CIRCE_PATTERN_PATTERNS_NONE_1),
                               circe_copy_get(CIRCE_PATTERN_PATTERNS_NONE_2)};
        circe_terminal_feed_set(&s_feed, lines, 2);
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_PATTERNS_ERROR: {
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, "patterns");
        const char *l1 =
            s_patterns_result.storage_unavailable ? circe_copy_get(CIRCE_PATTERN_PATTERNS_STORAGE_1)
                                                  : circe_copy_get(CIRCE_PATTERN_PATTERNS_ERROR_1);
        const char *l2 =
            s_patterns_result.storage_unavailable ? circe_copy_get(CIRCE_PATTERN_PATTERNS_STORAGE_2)
                                                  : circe_copy_get(CIRCE_PATTERN_PATTERNS_ERROR_2);
        const char *lines[] = {l1, l2};
        circe_terminal_feed_set(&s_feed, lines, 2);
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        if (!s_patterns_result.storage_unavailable) {
            add_btn("DIAGNOSTICS", alloc_btn_id("nav:%d", CIRCE_FLOW_DIAGNOSTICS));
        }
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_BODY_MAP:
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, "body map");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_BODY_MAP_TITLE));
        circe_terminal_nav_enable(true);
        body_map_browser_begin();
        body_map_browser_refresh();
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        create_scroll_panel();
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_BODY_MAP_EMPTY: {
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, "body map");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_BODY_MAP_TITLE));
        const char *elines[] = {circe_copy_get(CIRCE_PATTERN_BODY_MAP_EMPTY_1),
                                circe_copy_get(CIRCE_PATTERN_BODY_MAP_EMPTY_2)};
        circe_terminal_feed_set(&s_feed, elines, 2);
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_BODY_MAP_ERROR: {
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, "body map");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_BODY_MAP_TITLE));
        const char *l1 = s_body_map_summary.state == CIRCE_BODY_MAP_STATE_STORAGE
                             ? circe_copy_get(CIRCE_PATTERN_BODY_MAP_STORAGE_1)
                             : circe_copy_get(CIRCE_PATTERN_BODY_MAP_ERROR_1);
        const char *l2 = s_body_map_summary.state == CIRCE_BODY_MAP_STATE_STORAGE
                             ? circe_copy_get(CIRCE_PATTERN_BODY_MAP_STORAGE_2)
                             : circe_copy_get(CIRCE_PATTERN_BODY_MAP_ERROR_2);
        const char *elines[] = {l1, l2};
        circe_terminal_feed_set(&s_feed, elines, 2);
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        if (s_body_map_summary.state != CIRCE_BODY_MAP_STATE_STORAGE) {
            add_btn("DIAGNOSTICS", alloc_btn_id("nav:%d", CIRCE_FLOW_DIAGNOSTICS));
        }
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_MEMORY_BROWSE: {
        const circe_timeline_cache_t *cache = circe_timeline_get_cache();
        int count = cache ? cache->count : 0;
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, circe_timeline_category_title(s_memory_category));
        circe_terminal_nav_enable(false);
        circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_MEMORY_BROWSE_HINT));
        circe_memory_browser_begin(&s_memory_browser, count, 0);
        circe_memory_browser_refresh(&s_memory_browser, &s_feed, &s_hud);
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        create_scroll_panel();
        add_btn("VIEW", "mem_view");
        add_btn("DELETE", "mem_delete");
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_MEMORY_EMPTY: {
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, circe_timeline_category_title(s_memory_category));
        char l1[64];
        char l2[64];
        circe_timeline_empty_copy(s_memory_category, l1, sizeof(l1), l2, sizeof(l2));
        const char *elines[] = {l1, l2};
        circe_terminal_feed_set(&s_feed, elines, 2);
        circe_terminal_feed_show_cursor(&s_feed, true);
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_MEMORY_ERROR:
        setup_terminal_shell(CIRCE_FLOW_MEMORY_MENU, "memory");
        {
            char l1[64];
            char l2[64];
            snprintf(l1, sizeof(l1), "> %s", circe_copy_get(CIRCE_PATTERN_MEMORY_INDEX_REPAIR_1));
            snprintf(l2, sizeof(l2), "> %s", circe_copy_get(CIRCE_PATTERN_MEMORY_INDEX_REPAIR_2));
            const char *elines[] = {l1, l2};
            circe_terminal_feed_set(&s_feed, elines, 2);
            circe_terminal_feed_show_cursor(&s_feed, true);
        }
        s_nav_back_step = CIRCE_FLOW_MEMORY_MENU;
        add_btn("DIAGNOSTICS", alloc_btn_id("nav:%d", CIRCE_FLOW_DIAGNOSTICS));
        add_back_btn(CIRCE_FLOW_MEMORY_MENU);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_REVIEW_EMPTY: {
        setup_terminal_shell(CIRCE_FLOW_HOME, "review");
        {
            char l1[64];
            char l2[64];
            snprintf(l1, sizeof(l1), "%s", circe_copy_get(CIRCE_PATTERN_REVIEW_EMPTY));
            snprintf(l2, sizeof(l2), "%s", circe_copy_get(CIRCE_PATTERN_REVIEW_EMPTY_SUB));
            const char *lines[] = {l1, l2};
            circe_terminal_feed_set(&s_feed, lines, 2);
            circe_terminal_feed_show_cursor(&s_feed, true);
        }
        s_nav_back_step = CIRCE_FLOW_HOME;
        add_btn("HOME", "home");
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_DELETE_CONFIRM: {
        circe_flow_step_t back = s_memory_context ? CIRCE_FLOW_REVIEW
                                                  : (s_delete_from_memory ? CIRCE_FLOW_MEMORY_BROWSE
                                                                          : CIRCE_FLOW_REVIEW);
        setup_terminal_shell(back, "confirm delete");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_DELETE_CONFIRM));
        s_nav_back_step = back;
        add_btn("YES, DELETE", "delete_yes");
        add_btn(circe_copy_get(CIRCE_PATTERN_NAV_CANCEL), alloc_btn_id("nav:%d", (int)back));
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_EDIT:
        if (entry_is_regulation(&s_engine->draft)) {
            go_step(CIRCE_FLOW_REVIEW);
            break;
        }
        setup_terminal_shell(CIRCE_FLOW_REVIEW, "edit entry");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_EDIT_PROMPT));
        s_nav_back_step = CIRCE_FLOW_REVIEW;
        add_btn("CHANGE TONE", "edit_tone");
        add_btn("CHANGE COLOR", "edit_color");
        add_btn(circe_copy_get(CIRCE_PATTERN_EDIT_ADD_SENSATION), "edit_add_sensation");
        add_back_btn(CIRCE_FLOW_REVIEW);
        focus_first_obj(s_first_row);
        break;

    case CIRCE_FLOW_EDIT_COLOR:
        go_step(CIRCE_FLOW_COLOR_PICKER);
        break;

    case CIRCE_FLOW_QUICK_PICK:
        setup_selector_menu("QUICK NOTE", circe_copy_get(CIRCE_PATTERN_QUICK_ONE_TAP), CIRCE_FLOW_HOME,
                            s_dynamic_selector_items, build_quick_selector());
        break;

    case CIRCE_FLOW_GROUNDING:
        setup_terminal_shell(CIRCE_FLOW_HOME, "regulate");
        {
            char l1[64];
            char l2[64];
            snprintf(l1, sizeof(l1), "> %s", circe_copy_get(CIRCE_PATTERN_REG_GROUNDING_1));
            snprintf(l2, sizeof(l2), "> %s", circe_copy_get(CIRCE_PATTERN_REG_GROUNDING_2));
            const char *lines[] = {l1, l2};
            circe_terminal_feed_set(&s_feed, lines, 2);
            circe_terminal_feed_show_cursor(&s_feed, true);
        }
        s_nav_back_step = CIRCE_FLOW_HOME;
        circe_terminal_nav_enable(false);
        circe_selector_create(&s_selector, s_content, "REGULATE", s_regulation_menu_items, REG_MENU_COUNT, 0);
        break;

    case CIRCE_FLOW_BREATHING:
        setup_terminal_shell(CIRCE_FLOW_GROUNDING, "breathing pace");
        {
            char l1[64];
            char l2[64];
            snprintf(l1, sizeof(l1), "> %s", circe_copy_get(CIRCE_PATTERN_REG_BREATH_1));
            snprintf(l2, sizeof(l2), "> %s", circe_copy_get(CIRCE_PATTERN_REG_BREATH_2));
            const char *lines[] = {l1, l2};
            circe_terminal_feed_set(&s_feed, lines, 2);
            circe_terminal_feed_show_cursor(&s_feed, false);
        }
        circe_hud_set_subline(&s_hud, "PRESS PAUSE  DBL BACK  HOLD END");
        s_nav_back_step = CIRCE_FLOW_GROUNDING;
        circe_terminal_nav_enable(false);
        begin_content_column();
        circe_regulation_breathing_start(&s_regulation_result, s_column, regulation_action_cb, NULL);
        break;

    case CIRCE_FLOW_BODY_ANCHOR:
        setup_terminal_shell(CIRCE_FLOW_GROUNDING, "body anchor");
        {
            char l1[64];
            char l2[64];
            snprintf(l1, sizeof(l1), "> %s", circe_copy_get(CIRCE_PATTERN_REG_ANCHOR_1));
            snprintf(l2, sizeof(l2), "> %s", circe_copy_get(CIRCE_PATTERN_REG_ANCHOR_2));
            const char *lines[] = {l1, l2};
            circe_terminal_feed_set(&s_feed, lines, 2);
            circe_terminal_feed_show_cursor(&s_feed, false);
        }
        circe_hud_set_subline(&s_hud, "TURN PROMPT  PRESS NEXT  DBL BACK");
        s_nav_back_step = CIRCE_FLOW_GROUNDING;
        circe_terminal_nav_enable(false);
        begin_content_column();
        circe_regulation_body_anchor_start(&s_regulation_result, s_column, regulation_action_cb, NULL);
        break;

    case CIRCE_FLOW_REG_54321:
        setup_terminal_shell(CIRCE_FLOW_GROUNDING, "5-4-3-2-1");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_REG_54321_TITLE));
        circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_REG_PRESS_NEXT));
        s_nav_back_step = CIRCE_FLOW_GROUNDING;
        circe_terminal_nav_enable(false);
        begin_content_column();
        circe_regulation_54321_start(&s_regulation_result, s_column, regulation_action_cb, NULL);
        break;

    case CIRCE_FLOW_SENSORY_RESET:
        setup_terminal_shell(CIRCE_FLOW_GROUNDING, "sensory reset");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_REG_SENSORY_TITLE));
        circe_hud_set_subline(&s_hud, circe_copy_get(CIRCE_PATTERN_REG_LONG_END));
        s_nav_back_step = CIRCE_FLOW_GROUNDING;
        circe_terminal_nav_enable(false);
        begin_content_column();
        circe_regulation_sensory_start(&s_regulation_result, s_column, regulation_action_cb, NULL);
        break;

    case CIRCE_FLOW_BILATERAL_TAP:
        setup_terminal_shell(CIRCE_FLOW_GROUNDING, "bilateral tap");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_REG_BILATERAL_TITLE));
        circe_hud_set_subline(&s_hud, "PRESS PAUSE  TURN SPEED  DBL BACK  HOLD END");
        s_nav_back_step = CIRCE_FLOW_GROUNDING;
        circe_terminal_nav_enable(false);
        begin_content_column();
        circe_regulation_bilateral_start(&s_regulation_result, s_column, regulation_action_cb, NULL);
        break;

    case CIRCE_FLOW_REGULATION_SAVE: {
        setup_terminal_shell(CIRCE_FLOW_GROUNDING, "session done");
        show_terminal_prompt_text(circe_copy_get(CIRCE_PATTERN_REG_SAVE_PROMPT));
        char line[64];
        char type_label[32];
        format_regulation_type_label(s_regulation_result.regulation_type, type_label, sizeof(type_label));
        if (s_regulation_result.steps_completed > 0) {
            snprintf(line, sizeof(line), "%s %ds / %d steps", type_label, s_regulation_result.duration_seconds,
                     s_regulation_result.steps_completed);
        } else if (strcmp(s_regulation_result.regulation_type, "bilateral_tap") == 0) {
            snprintf(line, sizeof(line), "%s %ds / %d cycles", type_label, s_regulation_result.duration_seconds,
                     s_regulation_result.rounds_completed);
        } else {
            snprintf(line, sizeof(line), "%s %ds rounds %d", type_label, s_regulation_result.duration_seconds,
                     s_regulation_result.rounds_completed);
        }
        circe_hud_set_subline(&s_hud, line);
        s_nav_back_step = CIRCE_FLOW_GROUNDING;
        add_btn("SAVE", "reg_save");
        add_btn("SKIP", "reg_skip");
        add_back_btn(CIRCE_FLOW_GROUNDING);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_STRAND: {
        setup_terminal_shell(CIRCE_FLOW_DIAGNOSTICS, "strand");
        show_terminal_prompt_text("strand unavailable");
        s_nav_back_step = CIRCE_FLOW_DIAGNOSTICS;
        add_back_btn(CIRCE_FLOW_DIAGNOSTICS);
        focus_first_obj(s_first_row);
        break;
    }

    case CIRCE_FLOW_DIAGNOSTICS: {
        setup_terminal_shell(CIRCE_FLOW_HOME, "diagnostics");
        diagnostics_show_health_feed();
        s_nav_back_step = CIRCE_FLOW_HOME;
        circe_terminal_nav_enable(false);
        circe_selector_create(&s_selector, s_content, "DIAGNOSTICS", s_diagnostics_menu_items, DIAG_MENU_COUNT, 0);
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
    return enqueue_save_async(CIRCE_FLOW_REFLECTION, CIRCE_MSG_NONE, false);
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
