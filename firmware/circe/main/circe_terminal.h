#pragma once

#include "circe_conversation_engine.h"
#include "circe_ui_tokens.h"
#include "lvgl.h"

#define CIRCE_TERMINAL_FEED_LINES   CIRCE_UI_TERMINAL_FEED_LINES
#define CIRCE_TERMINAL_ROW_H        CIRCE_UI_TERMINAL_ROW_H
#define CIRCE_TERMINAL_FEED_Y_OFS   CIRCE_UI_TERMINAL_FEED_Y_OFS
#define CIRCE_TERMINAL_FEED_PANEL_H CIRCE_UI_TERMINAL_FEED_PANEL_H

typedef struct {
    lv_obj_t *panel;
    lv_obj_t *lines[CIRCE_TERMINAL_FEED_LINES];
    lv_obj_t *cursor;
    lv_timer_t *cursor_timer;
    bool cursor_on;
} circe_terminal_feed_t;

typedef void (*circe_terminal_nav_cb_t)(void *ctx);

void circe_terminal_feed_init(circe_terminal_feed_t *feed, lv_obj_t *parent);
void circe_terminal_feed_destroy(circe_terminal_feed_t *feed);
void circe_terminal_feed_clear(circe_terminal_feed_t *feed);
void circe_terminal_feed_set(circe_terminal_feed_t *feed, const char *lines[], int count);
void circe_terminal_feed_show_cursor(circe_terminal_feed_t *feed, bool show);

void circe_terminal_style_row(lv_obj_t *row);
void circe_terminal_style_row_label(lv_obj_t *lbl, bool focused);

lv_obj_t *circe_terminal_add_row(lv_obj_t *parent, const char *label, const char *action_id, lv_event_cb_t cb,
                                 lv_group_t *group, int stack_index);

void circe_terminal_nav_init(circe_terminal_nav_cb_t on_back, circe_terminal_nav_cb_t on_sysmenu,
                             circe_terminal_nav_cb_t on_triple_home, void *ctx);
void circe_terminal_nav_set_back_step(circe_flow_step_t step);
void circe_terminal_nav_enable(bool enabled);
void circe_terminal_nav_poll(void);

void circe_terminal_to_upper(char *dest, size_t dest_len, const char *src);
