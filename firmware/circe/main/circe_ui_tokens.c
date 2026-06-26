#include "circe_ui_tokens.h"

#include "esp_log.h"

static const char *TAG = "circe_ui";

void circe_ui_tokens_log_boot(void)
{
    ESP_LOGI(TAG, "CIRCE UI tokens loaded");
    ESP_LOGI(TAG, "  safe area %d..%d x %d..%d center (%d,%d)",
             CIRCE_UI_SAFE_LEFT_X, CIRCE_UI_SAFE_RIGHT_X, CIRCE_UI_SAFE_TOP_Y, CIRCE_UI_SAFE_BOTTOM_Y,
             CIRCE_UI_CENTER_X, CIRCE_UI_CENTER_Y);
    ESP_LOGI(TAG, "  selector root %dx%d active_y=%d hint_y=%d",
             CIRCE_UI_SELECTOR_ROOT_W, CIRCE_UI_SELECTOR_ROOT_H, CIRCE_UI_SELECTOR_CURRENT_Y,
             CIRCE_UI_SELECTOR_HINT_Y);
    ESP_LOGI(TAG, "  banner %dx%d y_ofs=%d terminal feed %dx%d y_ofs=%d",
             CIRCE_UI_STATUS_BANNER_W, CIRCE_UI_STATUS_BANNER_H, CIRCE_UI_STATUS_BANNER_Y_OFS,
             CIRCE_UI_TERMINAL_FEED_PANEL_W, CIRCE_UI_TERMINAL_FEED_PANEL_H, CIRCE_UI_TERMINAL_FEED_Y_OFS);
#if CIRCE_UI_HOME_OUTER_RING
    ESP_LOGI(TAG, "  home outer ring %dpx on %dx%d", CIRCE_UI_HOME_OUTER_RING_W, CIRCE_UI_HOME_OUTER_RING_SIZE,
             CIRCE_UI_HOME_OUTER_RING_SIZE);
#endif
}
