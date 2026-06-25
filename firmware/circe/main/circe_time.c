#include "circe_time.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "circe_time";
static const char *NVS_NS = "circe_time";

static bool s_time_set;
static circe_time_source_t s_source = CIRCE_TIME_SOURCE_NONE;
static bool s_manual_nvs;

static bool system_time_valid(void)
{
    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    int year = tm_local.tm_year + 1900;
    return year >= CIRCE_TIME_MIN_VALID_YEAR;
}

static bool load_manual_nvs(int *year, int *month, int *day, int *hour, int *minute)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        return false;
    }
    uint8_t flag = 0;
    if (nvs_get_u8(h, "manual_set", &flag) != ESP_OK || flag == 0) {
        nvs_close(h);
        return false;
    }
    uint16_t y = 0;
    uint8_t mo = 0;
    uint8_t d = 0;
    uint8_t hr = 0;
    uint8_t mi = 0;
    if (nvs_get_u16(h, "year", &y) != ESP_OK || nvs_get_u8(h, "month", &mo) != ESP_OK ||
        nvs_get_u8(h, "day", &d) != ESP_OK || nvs_get_u8(h, "hour", &hr) != ESP_OK ||
        nvs_get_u8(h, "minute", &mi) != ESP_OK) {
        nvs_close(h);
        return false;
    }
    nvs_close(h);
    if (year) {
        *year = y;
    }
    if (month) {
        *month = mo;
    }
    if (day) {
        *day = d;
    }
    if (hour) {
        *hour = hr;
    }
    if (minute) {
        *minute = mi;
    }
    return y >= CIRCE_TIME_MIN_VALID_YEAR && mo >= 1 && mo <= 12 && d >= 1 && d <= 31;
}

static bool save_manual_nvs(int year, int month, int day, int hour, int minute)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        return false;
    }
    esp_err_t err = nvs_set_u8(h, "manual_set", 1);
    if (err == ESP_OK) {
        err = nvs_set_u16(h, "year", (uint16_t)year);
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(h, "month", (uint8_t)month);
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(h, "day", (uint8_t)day);
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(h, "hour", (uint8_t)hour);
    }
    if (err == ESP_OK) {
        err = nvs_set_u8(h, "minute", (uint8_t)minute);
    }
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err == ESP_OK;
}

static bool apply_system_tm(int year, int month, int day, int hour, int minute)
{
    struct tm tm_local = {0};
    tm_local.tm_year = year - 1900;
    tm_local.tm_mon = month - 1;
    tm_local.tm_mday = day;
    tm_local.tm_hour = hour;
    tm_local.tm_min = minute;
    tm_local.tm_sec = 0;
    time_t secs = mktime(&tm_local);
    if (secs < 0) {
        ESP_LOGE(TAG, "mktime failed y=%d m=%d d=%d", year, month, day);
        return false;
    }
    struct timeval tv = {.tv_sec = secs, .tv_usec = 0};
    if (settimeofday(&tv, NULL) != 0) {
        ESP_LOGE(TAG, "settimeofday failed");
        return false;
    }
    ESP_LOGI(TAG, "time applied %04d-%02d-%02d %02d:%02d", year, month, day, hour, minute);
    return true;
}

void circe_time_init(void)
{
    s_time_set = false;
    s_source = CIRCE_TIME_SOURCE_NONE;
    s_manual_nvs = false;

    if (system_time_valid()) {
        s_time_set = true;
        s_source = CIRCE_TIME_SOURCE_SYSTEM;
        ESP_LOGI(TAG, "time set from system/RTC");
        return;
    }

    int y = 2026;
    int mo = 6;
    int d = 24;
    int h = 12;
    int mi = 0;
    if (load_manual_nvs(&y, &mo, &d, &h, &mi)) {
        s_manual_nvs = true;
        if (apply_system_tm(y, mo, d, h, mi)) {
            s_time_set = true;
            s_source = CIRCE_TIME_SOURCE_MANUAL;
            ESP_LOGI(TAG, "time restored from NVS manual %04d-%02d-%02d %02d:%02d", y, mo, d, h, mi);
            return;
        }
    }

    ESP_LOGW(TAG, "time unset — entries use folder %s", CIRCE_TIME_FOLDER_UNSET);
}

bool circe_time_is_set(void)
{
    return s_time_set;
}

circe_time_source_t circe_time_get_source(void)
{
    return s_source;
}

bool circe_time_has_manual_nvs(void)
{
    return s_manual_nvs;
}

void circe_time_get_local(int *year, int *month, int *day, int *hour, int *minute)
{
    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    if (year) {
        *year = tm_local.tm_year + 1900;
    }
    if (month) {
        *month = tm_local.tm_mon + 1;
    }
    if (day) {
        *day = tm_local.tm_mday;
    }
    if (hour) {
        *hour = tm_local.tm_hour;
    }
    if (minute) {
        *minute = tm_local.tm_min;
    }
}

void circe_time_format_date(char *buf, size_t len)
{
    if (!buf || len == 0) {
        return;
    }
    if (!s_time_set) {
        snprintf(buf, len, "--");
        return;
    }
    int y, m, d, h, mi;
    circe_time_get_local(&y, &m, &d, &h, &mi);
    snprintf(buf, len, "%04d-%02d-%02d", y, m, d);
}

void circe_time_format_time(char *buf, size_t len)
{
    if (!buf || len == 0) {
        return;
    }
    if (!s_time_set) {
        snprintf(buf, len, "--:--");
        return;
    }
    int y, m, d, h, mi;
    circe_time_get_local(&y, &m, &d, &h, &mi);
    (void)y;
    (void)m;
    (void)d;
    snprintf(buf, len, "%02d:%02d", h, mi);
}

void circe_time_format_status(char *buf, size_t len)
{
    if (!buf || len == 0) {
        return;
    }
    snprintf(buf, len, "%s", s_time_set ? "SET" : "UNSET");
}

bool circe_time_apply(int year, int month, int day, int hour, int minute)
{
    if (year < CIRCE_TIME_MIN_VALID_YEAR || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 ||
        hour > 23 || minute < 0 || minute > 59) {
        ESP_LOGE(TAG, "invalid manual time");
        return false;
    }
    if (!apply_system_tm(year, month, day, hour, minute)) {
        return false;
    }
    if (!save_manual_nvs(year, month, day, hour, minute)) {
        ESP_LOGW(TAG, "time set but NVS save failed");
    } else {
        s_manual_nvs = true;
    }
    s_time_set = true;
    s_source = CIRCE_TIME_SOURCE_MANUAL;
    return true;
}

static void iso8601_from_now(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
}

static void local_date_from_now(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    strftime(buf, len, "%Y-%m-%d", &tm_local);
}

void circe_time_fill_entry_timestamps(circe_entry_t *entry, bool is_create)
{
    if (!entry) {
        return;
    }
    if (!s_time_set) {
        if (is_create) {
            strncpy(entry->created_at, "unset", sizeof(entry->created_at) - 1);
            entry->created_at[sizeof(entry->created_at) - 1] = '\0';
        }
        strncpy(entry->updated_at, "unset", sizeof(entry->updated_at) - 1);
        entry->updated_at[sizeof(entry->updated_at) - 1] = '\0';
        entry->local_date[0] = '\0';
        return;
    }
    if (is_create) {
        iso8601_from_now(entry->created_at, sizeof(entry->created_at));
    }
    iso8601_from_now(entry->updated_at, sizeof(entry->updated_at));
    local_date_from_now(entry->local_date, sizeof(entry->local_date));
}

void circe_time_touch_entry_updated(circe_entry_t *entry)
{
    circe_time_fill_entry_timestamps(entry, false);
}

bool circe_time_entry_storage_folder(const circe_entry_t *entry, char *folder, size_t folder_len)
{
    if (!folder || folder_len == 0) {
        return false;
    }
    folder[0] = '\0';
    if (!s_time_set || !entry || entry->local_date[0] == '\0' || strcmp(entry->local_date, "1970-01-01") == 0) {
        strncpy(folder, CIRCE_TIME_FOLDER_UNSET, folder_len - 1);
        folder[folder_len - 1] = '\0';
        return true;
    }
    if (strlen(entry->local_date) == 10 && entry->local_date[4] == '-' && entry->local_date[7] == '-') {
        snprintf(folder, folder_len, "%.4s%.2s%.2s", entry->local_date, entry->local_date + 5, entry->local_date + 8);
        return true;
    }
    strncpy(folder, entry->local_date, folder_len - 1);
    folder[folder_len - 1] = '\0';
    return folder[0] != '\0';
}
