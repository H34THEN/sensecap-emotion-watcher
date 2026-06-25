#include "circe_memory_browser.h"

#include <stdio.h>

#include "circe_timeline.h"

#define MEM_DOUBLE_PRESS_MS 450

static lv_indev_t *find_encoder_indev(void)
{
    lv_indev_t *indev = NULL;
    while ((indev = lv_indev_get_next(indev)) != NULL) {
        if (indev->driver->type == LV_INDEV_TYPE_ENCODER) {
            return indev;
        }
    }
    return NULL;
}

void circe_memory_browser_begin(circe_memory_browser_t *browser, int count, int initial_index)
{
    if (!browser) {
        return;
    }
    browser->active = true;
    browser->count = count > 0 ? count : 0;
    browser->selected = 0;
    if (initial_index >= 0 && initial_index < browser->count) {
        browser->selected = initial_index;
    }
    browser->enc_pressed = false;
    browser->last_release_ms = 0;
}

void circe_memory_browser_destroy(circe_memory_browser_t *browser)
{
    if (!browser) {
        return;
    }
    browser->active = false;
    browser->count = 0;
    browser->selected = 0;
}

void circe_memory_browser_refresh(circe_memory_browser_t *browser, circe_terminal_feed_t *feed, circe_hud_t *hud)
{
    if (!browser || !browser->active || browser->count <= 0) {
        return;
    }
    const circe_timeline_cache_t *cache = circe_timeline_get_cache();
    if (!cache || browser->selected >= cache->count) {
        return;
    }
    const circe_timeline_item_t *item = &cache->items[browser->selected];
    char l1[32], l2[48], l3[48], l4[48];
    circe_timeline_item_format_lines(item, l1, sizeof(l1), l2, sizeof(l2), l3, sizeof(l3), l4, sizeof(l4));
    const char *lines[] = {l1, l2, l3, l4};
    circe_terminal_feed_set(feed, lines, 4);
    circe_terminal_feed_show_cursor(feed, true);
    if (hud) {
        char sub[48];
        snprintf(sub, sizeof(sub), "entry %d / %d", browser->selected + 1, browser->count);
        if (cache->truncated) {
            strncat(sub, "  recent", sizeof(sub) - strlen(sub) - 1);
        }
        circe_hud_set_subline(hud, sub);
    }
}

const char *circe_memory_browser_selected_id(const circe_memory_browser_t *browser)
{
    if (!browser || !browser->active || browser->count <= 0) {
        return NULL;
    }
    const circe_timeline_cache_t *cache = circe_timeline_get_cache();
    if (!cache || browser->selected >= cache->count) {
        return NULL;
    }
    return cache->items[browser->selected].entry_id;
}

void circe_memory_browser_poll(circe_memory_browser_t *browser, int *action_out)
{
    if (action_out) {
        *action_out = CIRCE_MEMORY_BROWSER_ACTION_NONE;
    }
    if (!browser || !browser->active || browser->count <= 0) {
        return;
    }

    lv_indev_t *enc = find_encoder_indev();
    if (!enc || !enc->driver || !enc->driver->read_cb) {
        return;
    }
    lv_indev_data_t data;
    lv_memset_00(&data, sizeof(data));
    enc->driver->read_cb(enc->driver, &data);

    if (data.enc_diff != 0) {
        browser->selected += data.enc_diff;
        while (browser->selected < 0) {
            browser->selected += browser->count;
        }
        while (browser->selected >= browser->count) {
            browser->selected -= browser->count;
        }
    }

    uint32_t now = lv_tick_get();
    bool pressed = (data.state == LV_INDEV_STATE_PRESSED);

    if (pressed && !browser->enc_pressed) {
        browser->enc_pressed = true;
        browser->press_start_ms = now;
    } else if (!pressed && browser->enc_pressed) {
        browser->enc_pressed = false;
        if (browser->last_release_ms != 0 && (now - browser->last_release_ms) < MEM_DOUBLE_PRESS_MS) {
            browser->last_release_ms = 0;
        } else {
            if (action_out) {
                *action_out = CIRCE_MEMORY_BROWSER_ACTION_OPEN;
            }
            browser->last_release_ms = now;
        }
    }
}
