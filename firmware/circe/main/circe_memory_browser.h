#pragma once

#include <stdbool.h>

#include "circe_hud.h"
#include "circe_terminal.h"

#define CIRCE_MEMORY_BROWSER_ACTION_NONE (-1)
#define CIRCE_MEMORY_BROWSER_ACTION_OPEN 200

typedef struct {
    bool active;
    int selected;
    int count;
    bool enc_pressed;
    uint32_t press_start_ms;
    uint32_t last_release_ms;
} circe_memory_browser_t;

void circe_memory_browser_begin(circe_memory_browser_t *browser, int count, int initial_index);
void circe_memory_browser_destroy(circe_memory_browser_t *browser);
void circe_memory_browser_refresh(circe_memory_browser_t *browser, circe_terminal_feed_t *feed, circe_hud_t *hud);
void circe_memory_browser_poll(circe_memory_browser_t *browser, int *action_out);
const char *circe_memory_browser_selected_id(const circe_memory_browser_t *browser);
