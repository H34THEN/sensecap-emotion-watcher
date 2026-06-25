#include "circe_photo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "circe_storage.h"
#include "circe_storage_paths.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "circe_photo";
static const char *PHOTO_NVS_NS = "circe_photo";
static const char *PHOTO_NVS_CONSENT = "consent_ok";

bool circe_photo_entry_eligible(const circe_entry_t *entry)
{
    if (!entry || entry->id[0] == '\0') {
        return false;
    }
    if (entry->entry_mode == CIRCE_ENTRY_MODE_REGULATION || entry->has_regulation) {
        return false;
    }
    return true;
}

bool circe_photo_build_path(const circe_entry_t *entry, char *path, size_t path_len)
{
    if (!entry || !path || path_len == 0 || entry->id[0] == '\0') {
        return false;
    }
    char folder[16] = {0};
    if (entry->local_date[0]) {
        circe_storage_path_entry_folder(entry->local_date, folder, sizeof(folder));
    }
    if (folder[0] == '\0') {
        strncpy(folder, "UNSET", sizeof(folder) - 1);
    }

    char dir[CIRCE_PHOTO_PATH_MAX];
    if (!circe_storage_path_join(dir, sizeof(dir), circe_storage_path_photos(), folder)) {
        return false;
    }

    char name[16];
    snprintf(name, sizeof(name), "%s.JPG", entry->id);
    return circe_storage_path_join(path, path_len, dir, name);
}

static bool ensure_photo_dir(const char *file_path)
{
    if (!file_path) {
        return false;
    }
    char dir[CIRCE_PHOTO_PATH_MAX];
    strncpy(dir, file_path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    char *slash = strrchr(dir, '/');
    if (!slash) {
        return false;
    }
    *slash = '\0';

    struct stat st;
    if (stat(dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    if (mkdir(dir, 0755) != 0) {
        ESP_LOGE(TAG, "mkdir failed: %s", dir);
        return false;
    }
    ESP_LOGI(TAG, "created photo dir %s", dir);
    return true;
}

bool circe_photo_write_jpeg(const char *path, const uint8_t *data, size_t len)
{
    if (!path || !data || len == 0) {
        return false;
    }
    if (!ensure_photo_dir(path)) {
        return false;
    }

    char tmp[CIRCE_PHOTO_PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s.TMP", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) {
        ESP_LOGE(TAG, "fopen failed: %s", tmp);
        return false;
    }
    size_t wrote = fwrite(data, 1, len, f);
    fclose(f);
    if (wrote != len) {
        unlink(tmp);
        ESP_LOGE(TAG, "write incomplete: %s", tmp);
        return false;
    }
    unlink(path);
    if (rename(tmp, path) != 0) {
        ESP_LOGE(TAG, "rename failed: %s -> %s", tmp, path);
        unlink(tmp);
        return false;
    }
    ESP_LOGI(TAG, "photo saved %s (%u bytes)", path, (unsigned)len);
    return true;
}

void circe_photo_clear_metadata(circe_entry_t *entry)
{
    if (!entry) {
        return;
    }
    entry->photo_attached = false;
    entry->photo_consent = false;
    entry->photo_training_ok = false;
    entry->photo_id[0] = '\0';
    entry->photo_path[0] = '\0';
    entry->photo_created_at[0] = '\0';
}

bool circe_photo_attach_metadata(circe_entry_t *entry, const char *path, bool consent)
{
    if (!entry || !path || path[0] == '\0') {
        return false;
    }
    entry->photo_attached = true;
    entry->photo_consent = consent;
    entry->photo_training_ok = false;
    strncpy(entry->photo_id, entry->id, sizeof(entry->photo_id) - 1);
    entry->photo_id[sizeof(entry->photo_id) - 1] = '\0';
    strncpy(entry->photo_path, path, sizeof(entry->photo_path) - 1);
    entry->photo_path[sizeof(entry->photo_path) - 1] = '\0';
    circe_entry_touch_updated(entry);
    strncpy(entry->photo_created_at, entry->updated_at, sizeof(entry->photo_created_at) - 1);
    entry->photo_created_at[sizeof(entry->photo_created_at) - 1] = '\0';
    return true;
}

bool circe_photo_update_entry_json(circe_entry_t *entry)
{
    if (!entry || entry->id[0] == '\0') {
        return false;
    }
    return circe_entry_save_json_atomic(entry);
}

bool circe_photo_delete_file_for_entry(const circe_entry_t *entry)
{
    if (!entry || !entry->photo_attached || entry->photo_path[0] == '\0') {
        return true;
    }
    if (unlink(entry->photo_path) != 0) {
        ESP_LOGW(TAG, "photo delete failed: %s", entry->photo_path);
        return false;
    }
    ESP_LOGI(TAG, "photo deleted %s", entry->photo_path);
    return true;
}

bool circe_photo_consent_was_given(void)
{
    nvs_handle_t h;
    if (nvs_open(PHOTO_NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        return false;
    }
    uint8_t v = 0;
    esp_err_t err = nvs_get_u8(h, PHOTO_NVS_CONSENT, &v);
    nvs_close(h);
    return err == ESP_OK && v == 1;
}

void circe_photo_mark_consent_given(void)
{
    nvs_handle_t h;
    if (nvs_open(PHOTO_NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGW(TAG, "nvs open failed for photo consent");
        return;
    }
    nvs_set_u8(h, PHOTO_NVS_CONSENT, 1);
    nvs_commit(h);
    nvs_close(h);
}

circe_camera_status_t circe_camera_capture_jpeg(uint8_t **out_data, size_t *out_len, char *err, size_t err_len)
{
    if (out_data) {
        *out_data = NULL;
    }
    if (out_len) {
        *out_len = 0;
    }
    const char *msg =
        "SSCMA/Himax camera pipeline not integrated in CIRCE standalone (SPI shared with SD)";
    ESP_LOGW(TAG, "%s", msg);
    if (err && err_len > 0) {
        snprintf(err, err_len, "%s", msg);
    }
    return CIRCE_CAMERA_STATUS_UNAVAILABLE;
}

void circe_camera_release_buffer(uint8_t *data)
{
    if (data) {
        free(data);
    }
}

circe_photo_result_t circe_photo_capture_and_attach(circe_entry_t *entry)
{
    if (!entry || !circe_photo_entry_eligible(entry)) {
        return CIRCE_PHOTO_RESULT_UNAVAILABLE;
    }
    if (!circe_storage_is_ready()) {
        ESP_LOGW(TAG, "storage not ready for photo");
        return CIRCE_PHOTO_RESULT_SAVE_FAILED;
    }

    uint8_t *jpeg = NULL;
    size_t jpeg_len = 0;
    char err[96] = {0};
    circe_camera_status_t cam = circe_camera_capture_jpeg(&jpeg, &jpeg_len, err, sizeof(err));
    if (cam != CIRCE_CAMERA_STATUS_OK || !jpeg || jpeg_len == 0) {
        circe_camera_release_buffer(jpeg);
        return CIRCE_PHOTO_RESULT_UNAVAILABLE;
    }

    char path[CIRCE_PHOTO_PATH_MAX];
    if (!circe_photo_build_path(entry, path, sizeof(path))) {
        circe_camera_release_buffer(jpeg);
        return CIRCE_PHOTO_RESULT_SAVE_FAILED;
    }

    bool wrote = circe_photo_write_jpeg(path, jpeg, jpeg_len);
    circe_camera_release_buffer(jpeg);
    if (!wrote) {
        return CIRCE_PHOTO_RESULT_SAVE_FAILED;
    }

    if (!circe_photo_attach_metadata(entry, path, true)) {
        return CIRCE_PHOTO_RESULT_SAVE_FAILED;
    }
    if (!circe_photo_update_entry_json(entry)) {
        return CIRCE_PHOTO_RESULT_SAVE_FAILED;
    }
    return CIRCE_PHOTO_RESULT_SAVED;
}
