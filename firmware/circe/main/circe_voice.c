#include "circe_voice.h"

#include <math.h>
#include <string.h>

#include "esp_codec_dev.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "sensecap-watcher.h"

static const char *TAG = "circe_voice";
static const char *VOICE_NVS_NS = "circe_voice";
static const char *VOICE_NVS_MODE = "mode";

#define VOICE_QUEUE_LEN   4
#define VOICE_TASK_STACK  4096
#define VOICE_TASK_PRIO   2
#define VOICE_SAMPLE_RATE DRV_AUDIO_SAMPLE_RATE
#define VOICE_AMP_SOFT    900

static circe_voice_mode_t s_mode = CIRCE_VOICE_MODE_OFF;
static bool s_hw_probed;
static bool s_hw_available;
static bool s_spk_open;
static esp_codec_dev_handle_t s_speaker;
static SemaphoreHandle_t s_play_mutex;
static QueueHandle_t s_queue;
static TaskHandle_t s_task;

static bool ensure_play_mutex(void)
{
    if (!s_play_mutex) {
        s_play_mutex = xSemaphoreCreateMutex();
    }
    return s_play_mutex != NULL;
}

static bool voice_hw_init(void)
{
    if (s_hw_probed) {
        return s_hw_available;
    }
    s_hw_probed = true;

    if (!ensure_play_mutex()) {
        ESP_LOGW(TAG, "voice mutex unavailable");
        return false;
    }

    s_speaker = bsp_audio_codec_speaker_init();
    if (!s_speaker) {
        ESP_LOGW(TAG, "speaker codec init failed");
        return false;
    }

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = VOICE_SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = DRV_AUDIO_SAMPLE_BITS,
    };
    if (esp_codec_dev_open(s_speaker, &fs) != ESP_OK) {
        ESP_LOGW(TAG, "speaker open failed");
        s_speaker = NULL;
        return false;
    }
    s_spk_open = true;
    esp_codec_dev_set_out_mute(s_speaker, false);
    esp_codec_dev_set_out_vol(s_speaker, 22);

    s_hw_available = true;
    ESP_LOGI(TAG, "speaker ready (output only, no microphone)");
    return true;
}

static bool play_tone(int freq_hz, int duration_ms, int amplitude)
{
    if (!s_hw_available || !s_speaker || !s_spk_open || freq_hz <= 0 || duration_ms <= 0) {
        return false;
    }

    int samples = (VOICE_SAMPLE_RATE * duration_ms) / 1000;
    if (samples < 1) {
        return false;
    }

    size_t bytes = (size_t)samples * sizeof(int16_t);
    int16_t *buf = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) {
        buf = heap_caps_malloc(bytes, MALLOC_CAP_8BIT);
    }
    if (!buf) {
        ESP_LOGW(TAG, "tone buffer alloc failed");
        return false;
    }

    const float fade_samples = (float)VOICE_SAMPLE_RATE * 0.012f;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / (float)VOICE_SAMPLE_RATE;
        float env = 1.0f;
        if (fade_samples > 1.0f) {
            if ((float)i < fade_samples) {
                env = (float)i / fade_samples;
            } else if ((float)(samples - i) < fade_samples) {
                env = (float)(samples - i) / fade_samples;
            }
        }
        float sample = sinf(2.0f * (float)M_PI * (float)freq_hz * t);
        buf[i] = (int16_t)(sample * (float)amplitude * env);
    }

    xSemaphoreTake(s_play_mutex, portMAX_DELAY);
    esp_err_t err = esp_codec_dev_write(s_speaker, buf, bytes);
    xSemaphoreGive(s_play_mutex);
    heap_caps_free(buf);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "tone write failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

static void play_event_blocking(circe_voice_event_t event)
{
    if (s_mode != CIRCE_VOICE_MODE_SOFT) {
        return;
    }
    if (!voice_hw_init()) {
        return;
    }

    switch (event) {
    case CIRCE_VOICE_EVENT_SAVE_OK:
        play_tone(523, 70, VOICE_AMP_SOFT);
        break;
    case CIRCE_VOICE_EVENT_REGULATION_START:
        play_tone(392, 90, VOICE_AMP_SOFT);
        break;
    case CIRCE_VOICE_EVENT_BREATHE_INHALE:
        play_tone(440, 55, VOICE_AMP_SOFT / 2);
        break;
    case CIRCE_VOICE_EVENT_BREATHE_HOLD:
        play_tone(494, 45, VOICE_AMP_SOFT / 2);
        break;
    case CIRCE_VOICE_EVENT_BREATHE_EXHALE:
        play_tone(349, 55, VOICE_AMP_SOFT / 2);
        break;
    case CIRCE_VOICE_EVENT_SESSION_COMPLETE:
        play_tone(440, 100, VOICE_AMP_SOFT);
        break;
    case CIRCE_VOICE_EVENT_ERROR_SOFT:
        play_tone(330, 60, VOICE_AMP_SOFT / 3);
        break;
    case CIRCE_VOICE_EVENT_TEST:
        play_tone(440, 120, VOICE_AMP_SOFT);
        break;
    default:
        break;
    }
    ESP_LOGI(TAG, "voice event played type=%d", (int)event);
}

static void voice_task(void *arg)
{
    (void)arg;
    circe_voice_event_t event;
    while (1) {
        if (xQueueReceive(s_queue, &event, portMAX_DELAY) == pdTRUE) {
            play_event_blocking(event);
        }
    }
}

static void ensure_task(void)
{
    if (s_task) {
        return;
    }
    if (!s_queue) {
        s_queue = xQueueCreate(VOICE_QUEUE_LEN, sizeof(circe_voice_event_t));
    }
    if (!s_queue) {
        return;
    }
    xTaskCreate(voice_task, "circe_voice", VOICE_TASK_STACK, NULL, VOICE_TASK_PRIO, &s_task);
}

static void load_mode_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(VOICE_NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        s_mode = CIRCE_VOICE_MODE_OFF;
        return;
    }
    uint8_t v = CIRCE_VOICE_MODE_OFF;
    if (nvs_get_u8(h, VOICE_NVS_MODE, &v) == ESP_OK && v <= CIRCE_VOICE_MODE_SOFT) {
        s_mode = (circe_voice_mode_t)v;
    }
    nvs_close(h);
}

static void save_mode_to_nvs(circe_voice_mode_t mode)
{
    nvs_handle_t h;
    if (nvs_open(VOICE_NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGW(TAG, "nvs open failed for voice mode");
        return;
    }
    nvs_set_u8(h, VOICE_NVS_MODE, (uint8_t)mode);
    nvs_commit(h);
    nvs_close(h);
}

void circe_voice_init(void)
{
    load_mode_from_nvs();
    ensure_play_mutex();
    ensure_task();
    ESP_LOGI(TAG, "voice init mode=%s (default off until user enables soft)",
             s_mode == CIRCE_VOICE_MODE_SOFT ? "soft" : "off");
}

bool circe_voice_is_available(void)
{
    if (s_hw_probed) {
        return s_hw_available;
    }
    return voice_hw_init();
}

bool circe_voice_is_enabled(void)
{
    return s_mode == CIRCE_VOICE_MODE_SOFT && circe_voice_is_available();
}

circe_voice_mode_t circe_voice_get_mode(void)
{
    return s_mode;
}

void circe_voice_set_mode(circe_voice_mode_t mode)
{
    if (mode > CIRCE_VOICE_MODE_SOFT) {
        mode = CIRCE_VOICE_MODE_OFF;
    }
    s_mode = mode;
    save_mode_to_nvs(mode);
    ESP_LOGI(TAG, "voice mode changed to %s", mode == CIRCE_VOICE_MODE_SOFT ? "soft" : "off");
    if (mode == CIRCE_VOICE_MODE_SOFT) {
        if (!voice_hw_init()) {
            ESP_LOGW(TAG, "speaker init failed after enabling soft mode");
        }
    }
}

bool circe_voice_play_test_tone(void)
{
    if (s_mode != CIRCE_VOICE_MODE_SOFT) {
        ESP_LOGI(TAG, "voice test skipped: mode off");
        return false;
    }
    if (!voice_hw_init()) {
        ESP_LOGW(TAG, "voice test failed: hardware unavailable");
        return false;
    }
    ESP_LOGI(TAG, "voice test tone queued");
    ensure_task();
    if (!s_queue) {
        return play_tone(440, 120, VOICE_AMP_SOFT);
    }
    circe_voice_event_t ev = CIRCE_VOICE_EVENT_TEST;
    if (xQueueSend(s_queue, &ev, 0) != pdTRUE) {
        ESP_LOGW(TAG, "voice test queue full, playing inline");
        return play_tone(440, 120, VOICE_AMP_SOFT);
    }
    return true;
}

void circe_voice_play_event(circe_voice_event_t event)
{
    if (s_mode != CIRCE_VOICE_MODE_SOFT) {
        return;
    }
    ensure_task();
    if (!s_queue) {
        ESP_LOGD(TAG, "voice queue unavailable event=%d", (int)event);
        return;
    }
    if (xQueueSend(s_queue, &event, 0) != pdTRUE) {
        ESP_LOGD(TAG, "voice queue full event=%d", (int)event);
    } else {
        ESP_LOGI(TAG, "voice event queued type=%d", (int)event);
    }
}
