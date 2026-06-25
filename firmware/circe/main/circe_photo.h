#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "circe_entry.h"

typedef enum {
    CIRCE_PHOTO_RESULT_NONE = 0,
    CIRCE_PHOTO_RESULT_SAVED,
    CIRCE_PHOTO_RESULT_UNAVAILABLE,
    CIRCE_PHOTO_RESULT_SAVE_FAILED,
} circe_photo_result_t;

typedef enum {
    CIRCE_CAMERA_STATUS_UNAVAILABLE = 0,
    CIRCE_CAMERA_STATUS_OK,
    CIRCE_CAMERA_STATUS_ERROR,
} circe_camera_status_t;

bool circe_photo_entry_eligible(const circe_entry_t *entry);
bool circe_photo_build_path(const circe_entry_t *entry, char *path, size_t path_len);
bool circe_photo_write_jpeg(const char *path, const uint8_t *data, size_t len);
bool circe_photo_attach_metadata(circe_entry_t *entry, const char *path, bool consent);
void circe_photo_clear_metadata(circe_entry_t *entry);
bool circe_photo_delete_file_for_entry(const circe_entry_t *entry);
bool circe_photo_update_entry_json(circe_entry_t *entry);

bool circe_photo_consent_was_given(void);
void circe_photo_mark_consent_given(void);

circe_camera_status_t circe_camera_capture_jpeg(uint8_t **out_data, size_t *out_len, char *err, size_t err_len);
void circe_camera_release_buffer(uint8_t *data);

circe_photo_result_t circe_photo_capture_and_attach(circe_entry_t *entry);
