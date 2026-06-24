#pragma once

#include <stdbool.h>

#include "circe_entry.h"

typedef enum {
    CIRCE_SAVE_OK = 0,
    CIRCE_SAVE_OK_INDEX_WARN,
    CIRCE_SAVE_STORAGE_NOT_READY,
    CIRCE_SAVE_JSON_WRITE_FAILED,
    CIRCE_SAVE_INVALID_COLOR,
    CIRCE_SAVE_INVALID_ENTRY_STATE,
    CIRCE_SAVE_INDEX_APPEND_FAILED,
    CIRCE_SAVE_MEMORY_ALLOCATION_FAILED,
    CIRCE_SAVE_OK_NON_ATOMIC,
    CIRCE_SAVE_UNKNOWN_FAILURE,
} circe_save_result_t;

typedef struct {
    circe_save_result_t result;
    bool json_ok;
    bool index_ok;
} circe_save_report_t;

circe_save_result_t circe_save_entry(circe_entry_t *entry, bool editing_existing);
circe_save_result_t circe_save_entry_report(circe_entry_t *entry, bool editing_existing, circe_save_report_t *report);
const char *circe_save_result_name(circe_save_result_t result);
bool circe_save_result_is_success(circe_save_result_t result);
bool circe_entry_validate_color_hex(const char *hex);
bool circe_entry_normalize_color_hex(char *hex, size_t hex_len);
bool circe_entry_prepare_for_save(circe_entry_t *entry);
bool circe_storage_ensure_ready(void);
