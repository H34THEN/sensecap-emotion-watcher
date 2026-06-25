#include "esp_log.h"
#include "nvs_flash.h"
#include "sensecap-watcher.h"

#include "circe_conversation_engine.h"
#include "circe_storage.h"
#include "circe_strand_cache.h"
#include "circe_theme.h"
#include "circe_fonts.h"
#include "circe_time.h"
#include "circe_ui.h"
#include "circe_voice.h"

static const char *TAG = "circe_main";

static circe_conversation_engine_t s_engine;

static esp_err_t circe_nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erase: %s", esp_err_to_name(ret));
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "nvs_flash_erase failed: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "NVS initialized");
    }
    return ret;
}

void app_main(void)
{
    ESP_LOGI(TAG, "CIRCE standalone MVP starting");

    if (circe_nvs_init() != ESP_OK) {
        ESP_LOGW(TAG, "NVS unavailable — theme/time settings may not persist");
    }

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
    circe_time_init();

    circe_conversation_init(&s_engine);
    s_engine.storage_ready = circe_storage_init();
    circe_fonts_init();
    circe_theme_init();
    circe_voice_init();
    if (s_engine.storage_ready) {
        circe_strand_cache_init();
    }

    lv_disp_t *disp = bsp_lvgl_init();
    if (!disp) {
        ESP_LOGE(TAG, "LVGL init failed");
        return;
    }

    circe_ui_set_engine(&s_engine);

    if (lvgl_port_lock(-1)) {
        circe_ui_init();
        circe_ui_show_step(CIRCE_FLOW_HOME);
        circe_ui_apply_boot_strand();
        lvgl_port_unlock();
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
