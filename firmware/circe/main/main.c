#include "esp_log.h"
#include "sensecap-watcher.h"

#include "circe_conversation_engine.h"
#include "circe_storage.h"
#include "circe_ui.h"

static const char *TAG = "circe_main";

static circe_conversation_engine_t s_engine;

void app_main(void)
{
    ESP_LOGI(TAG, "CIRCE standalone MVP starting");

    esp_io_expander_handle_t io_expander = bsp_io_expander_init();
    if (!io_expander) {
        ESP_LOGE(TAG, "IO expander init failed");
        return;
    }

    if (bsp_sdcard_init_default() != ESP_OK) {
        ESP_LOGW(TAG, "SD card init failed — storage will not work");
    } else {
        ESP_LOGI(TAG, "SD card mounted at /sdcard");
    }

    bsp_rtc_init();

    lv_disp_t *disp = bsp_lvgl_init();
    if (!disp) {
        ESP_LOGE(TAG, "LVGL init failed");
        return;
    }

    circe_conversation_init(&s_engine);
    s_engine.storage_ready = circe_storage_init();
    if (s_engine.storage_ready) {
        circe_storage_run_self_test();
    }

    circe_ui_set_engine(&s_engine);

    if (lvgl_port_lock(0)) {
        circe_ui_init();
        circe_ui_show_step(CIRCE_FLOW_HOME);
        lvgl_port_unlock();
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
