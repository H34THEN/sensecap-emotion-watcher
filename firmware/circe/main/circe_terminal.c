#include "circe_terminal.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "circe_fonts.h"
#include "circe_encoder.h"
#include "circe_theme.h"
#include "esp_log.h"

#define CURSOR_PERIOD_MS 530
#define DOUBLE_PRESS_MS  450
#define LONG_PRESS_MS    800

static const char *TAG = "circe_terminal";

static circe_terminal_nav_cb_t s_on_back;
static circe_terminal_nav_cb_t s_on_sysmenu;
static circe_terminal_nav_cb_t s_on_triple_home;
static void *s_nav_ctx;
static circe_flow_step_t s_back_step = CIRCE_FLOW_HOME;
static bool s_nav_enabled;
static circe_encoder_state_t s_enc;

static bool feed_is_live(const circe_terminal_feed_t *feed)
{
    return feed && feed->panel != NULL;
}

static void cursor_timer_cb(lv_timer_t *timer)
{
    circe_terminal_feed_t *feed = (circe_terminal_feed_t *)timer->user_data;
    if (!feed || !feed->cursor) {
        ESP_LOGE(TAG, "terminal_feed_cursor_show: cursor null in timer feed=%p", (void *)feed);
        return;
    }
    feed->cursor_on = !feed->cursor_on;
    lv_label_set_text(feed->cursor, feed->cursor_on ? "_" : " ");
}

static void row_focus_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED) {
        return;
    }
    lv_obj_t *row = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(row, 0);
    const char *base = (const char *)lv_obj_get_user_data(row);
    if (!lbl || !base) {
        return;
    }
    bool focused = lv_obj_has_state(row, LV_STATE_FOCUSED) || lv_obj_has_state(row, LV_STATE_FOCUS_KEY);
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%s", focused ? "> " : "  ", base);
    lv_label_set_text(lbl, buf);
    circe_terminal_style_row_label(lbl, focused);
    if (focused) {
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        const circe_theme_palette_t *p = circe_theme_get_palette();
        lv_obj_set_style_border_color(row, circe_theme_color(p->focus), 0);
        lv_obj_set_style_border_opa(row, LV_OPA_70, 0);
    } else {
        lv_obj_set_style_border_width(row, 0, 0);
    }
}

void circe_terminal_to_upper(char *dest, size_t dest_len, const char *src)
{
    if (!dest || dest_len == 0) {
        return;
    }
    size_t i = 0;
    for (; src && src[i] && i + 1 < dest_len; i++) {
        dest[i] = (char)toupper((unsigned char)src[i]);
    }
    dest[i] = '\0';
}

void circe_terminal_style_row(lv_obj_t *row)
{
    if (!row) {
        return;
    }
    lv_obj_set_width(row, 248);
    lv_obj_set_height(row, CIRCE_TERMINAL_ROW_H);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_set_style_pad_left(row, 4, 0);
    lv_obj_set_style_pad_right(row, 4, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_add_event_cb(row, row_focus_cb, LV_EVENT_ALL, NULL);
}

void circe_terminal_style_row_label(lv_obj_t *lbl, bool focused)
{
    if (!lbl) {
        return;
    }
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(focused ? p->text : p->muted), 0);
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_PROMPT);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
}

void circe_terminal_feed_destroy(circe_terminal_feed_t *feed)
{
    if (!feed) {
        ESP_LOGE(TAG, "terminal_feed_destroy: feed null");
        return;
    }
    ESP_LOGI(TAG, "terminal_feed_destroy feed=%p panel=%p cursor=%p timer=%p", (void *)feed, (void *)feed->panel,
             (void *)feed->cursor, (void *)feed->cursor_timer);
    if (feed->cursor_timer) {
        lv_timer_del(feed->cursor_timer);
        feed->cursor_timer = NULL;
    }
    feed->cursor = NULL;
    for (int i = 0; i < CIRCE_TERMINAL_FEED_LINES; i++) {
        feed->lines[i] = NULL;
    }
    if (feed->panel) {
        lv_obj_del(feed->panel);
        feed->panel = NULL;
    }
    feed->cursor_on = false;
    memset(feed, 0, sizeof(*feed));
}

void circe_terminal_feed_init(circe_terminal_feed_t *feed, lv_obj_t *parent)
{
    if (!feed) {
        ESP_LOGE(TAG, "terminal_feed_create: feed null");
        return;
    }
    if (!parent) {
        ESP_LOGE(TAG, "terminal_feed_create: parent null feed=%p", (void *)feed);
        return;
    }
    circe_terminal_feed_destroy(feed);

    feed->panel = lv_obj_create(parent);
    if (!feed->panel) {
        ESP_LOGE(TAG, "terminal_feed_create: panel create failed parent=%p", (void *)parent);
        return;
    }
    ESP_LOGI(TAG, "terminal_feed_create feed=%p panel=%p parent=%p", (void *)feed, (void *)feed->panel, (void *)parent);

    lv_obj_set_size(feed->panel, CIRCE_UI_TERMINAL_FEED_PANEL_W, CIRCE_UI_TERMINAL_FEED_PANEL_H);
    lv_obj_align(feed->panel, LV_ALIGN_TOP_MID, 0, CIRCE_UI_TERMINAL_FEED_Y_OFS);
    lv_obj_set_style_bg_opa(feed->panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(feed->panel, 0, 0);
    lv_obj_set_style_pad_all(feed->panel, 0, 0);
    lv_obj_clear_flag(feed->panel, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < CIRCE_TERMINAL_FEED_LINES; i++) {
        feed->lines[i] = lv_label_create(feed->panel);
        if (!feed->lines[i]) {
            ESP_LOGE(TAG, "terminal_feed_create: line %d create failed", i);
            continue;
        }
        lv_obj_set_width(feed->lines[i], CIRCE_UI_TERMINAL_LINE_W);
        lv_label_set_long_mode(feed->lines[i], LV_LABEL_LONG_DOT);
        lv_obj_align(feed->lines[i], LV_ALIGN_TOP_LEFT, CIRCE_UI_TERMINAL_LINE_X_PAD,
                     i * CIRCE_UI_TERMINAL_LINE_Y_STEP);
        lv_obj_add_flag(feed->lines[i], LV_OBJ_FLAG_HIDDEN);
        circe_fonts_apply_label(feed->lines[i], CIRCE_FONT_ROLE_PROMPT);
        lv_obj_set_style_text_align(feed->lines[i], LV_TEXT_ALIGN_LEFT, 0);
    }

    feed->cursor = lv_label_create(feed->panel);
    if (!feed->cursor) {
        ESP_LOGE(TAG, "terminal_feed_cursor_create: failed feed=%p panel=%p", (void *)feed, (void *)feed->panel);
        return;
    }
    ESP_LOGI(TAG, "terminal_feed_cursor_create feed=%p cursor=%p", (void *)feed, (void *)feed->cursor);
    lv_label_set_text(feed->cursor, "_");
    circe_fonts_apply_label(feed->cursor, CIRCE_FONT_ROLE_PROMPT);
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(feed->cursor, circe_theme_color(p->accent_primary), 0);
    lv_obj_align(feed->cursor, LV_ALIGN_BOTTOM_LEFT, CIRCE_UI_TERMINAL_LINE_X_PAD, CIRCE_UI_TERMINAL_CURSOR_Y_OFS);
    lv_obj_add_flag(feed->cursor, LV_OBJ_FLAG_HIDDEN);
}

void circe_terminal_feed_clear(circe_terminal_feed_t *feed)
{
    if (!feed) {
        ESP_LOGE(TAG, "terminal_feed_clear: feed null");
        return;
    }
    if (!feed_is_live(feed)) {
        ESP_LOGW(TAG, "terminal_feed_clear: feed not live feed=%p panel=%p", (void *)feed, (void *)feed->panel);
        return;
    }
    ESP_LOGI(TAG, "terminal_feed_clear feed=%p panel=%p cursor=%p", (void *)feed, (void *)feed->panel,
             (void *)feed->cursor);
    for (int i = 0; i < CIRCE_TERMINAL_FEED_LINES; i++) {
        if (!feed->lines[i]) {
            continue;
        }
        lv_label_set_text(feed->lines[i], "");
        lv_obj_add_flag(feed->lines[i], LV_OBJ_FLAG_HIDDEN);
    }
    circe_terminal_feed_show_cursor(feed, false);
}

void circe_terminal_feed_set(circe_terminal_feed_t *feed, const char *lines[], int count)
{
    if (!feed) {
        ESP_LOGE(TAG, "terminal_feed_set: feed null");
        return;
    }
    if (!feed_is_live(feed)) {
        ESP_LOGW(TAG, "terminal_feed_set: feed not live feed=%p", (void *)feed);
        return;
    }
    circe_terminal_feed_clear(feed);
    if (count > CIRCE_TERMINAL_FEED_LINES) {
        count = CIRCE_TERMINAL_FEED_LINES;
    }
    const circe_theme_palette_t *p = circe_theme_get_palette();
    for (int i = 0; i < count; i++) {
        if (!feed->lines[i]) {
            ESP_LOGE(TAG, "terminal_feed_set: line %d null", i);
            continue;
        }
        char buf[96];
        snprintf(buf, sizeof(buf), "> %s", lines[i] ? lines[i] : "");
        lv_label_set_text(feed->lines[i], buf);
        lv_obj_clear_flag(feed->lines[i], LV_OBJ_FLAG_HIDDEN);
        int age = count - 1 - i;
        lv_opa_t opa = LV_OPA_COVER;
        if (age == 1) {
            opa = LV_OPA_80;
        } else if (age == 2) {
            opa = LV_OPA_60;
        } else if (age >= 3) {
            opa = LV_OPA_40;
        }
        lv_obj_set_style_text_color(feed->lines[i], circe_theme_color(p->text), 0);
        lv_obj_set_style_text_opa(feed->lines[i], opa, 0);
    }
}

void circe_terminal_feed_show_cursor(circe_terminal_feed_t *feed, bool show)
{
    if (!feed) {
        ESP_LOGE(TAG, "terminal_feed_cursor_%s: feed null", show ? "show" : "hide");
        return;
    }
    if (!feed->cursor) {
        ESP_LOGE(TAG, "terminal_feed_cursor_%s: cursor null feed=%p panel=%p", show ? "show" : "hide", (void *)feed,
                 (void *)feed->panel);
        return;
    }
    ESP_LOGI(TAG, "terminal_feed_cursor_%s feed=%p cursor=%p timer=%p", show ? "show" : "hide", (void *)feed,
             (void *)feed->cursor, (void *)feed->cursor_timer);
    if (show) {
        lv_obj_clear_flag(feed->cursor, LV_OBJ_FLAG_HIDDEN);
        feed->cursor_on = true;
        lv_label_set_text(feed->cursor, "_");
        if (!feed->cursor_timer) {
            feed->cursor_timer = lv_timer_create(cursor_timer_cb, CURSOR_PERIOD_MS, feed);
            if (!feed->cursor_timer) {
                ESP_LOGE(TAG, "terminal_feed_cursor_show: timer create failed");
            }
        }
    } else {
        lv_obj_add_flag(feed->cursor, LV_OBJ_FLAG_HIDDEN);
        if (feed->cursor_timer) {
            lv_timer_del(feed->cursor_timer);
            feed->cursor_timer = NULL;
        }
    }
}

lv_obj_t *circe_terminal_add_row(lv_obj_t *parent, const char *label, const char *action_id, lv_event_cb_t cb,
                                 lv_group_t *group, int stack_index)
{
    if (!parent) {
        ESP_LOGE(TAG, "terminal_add_row: parent null");
        return NULL;
    }
    lv_obj_t *row = lv_btn_create(parent);
    if (!row) {
        ESP_LOGE(TAG, "terminal_add_row: row create failed");
        return NULL;
    }
    circe_terminal_style_row(row);
    lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, (void *)action_id);
    lv_obj_set_user_data(row, (void *)label);

    lv_obj_t *lbl = lv_label_create(row);
    char buf[64];
    snprintf(buf, sizeof(buf), "  %s", label);
    lv_label_set_text(lbl, buf);
    circe_terminal_style_row_label(lbl, false);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, stack_index * (CIRCE_UI_TERMINAL_ROW_H + CIRCE_UI_TERMINAL_BTN_ROW_GAP));
    if (group) {
        lv_group_add_obj(group, row);
    }
    return row;
}

void circe_terminal_nav_init(circe_terminal_nav_cb_t on_back, circe_terminal_nav_cb_t on_sysmenu,
                             circe_terminal_nav_cb_t on_triple_home, void *ctx)
{
    s_on_back = on_back;
    s_on_sysmenu = on_sysmenu;
    s_on_triple_home = on_triple_home;
    s_nav_ctx = ctx;
    s_nav_enabled = true;
    circe_encoder_state_reset(&s_enc);
}

void circe_terminal_nav_set_back_step(circe_flow_step_t step)
{
    s_back_step = step;
}

void circe_terminal_nav_enable(bool enabled)
{
    s_nav_enabled = enabled;
}

void circe_terminal_nav_poll(void)
{
    if (!s_nav_enabled) {
        return;
    }
    int diff = circe_encoder_read_diff();
    bool pressed = circe_encoder_read_pressed();
    int action = circe_encoder_poll(&s_enc, diff, pressed);
    if (action == CIRCE_ENC_ACTION_TRIPLE && s_on_triple_home) {
        s_on_triple_home(s_nav_ctx);
    } else if (action == CIRCE_ENC_ACTION_DOUBLE && s_on_back) {
        s_on_back(s_nav_ctx);
    } else if (action == CIRCE_ENC_ACTION_LONG && s_on_sysmenu) {
        s_on_sysmenu(s_nav_ctx);
    }
}
