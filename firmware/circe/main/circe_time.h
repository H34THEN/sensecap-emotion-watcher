#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "circe_entry.h"

#define CIRCE_TIME_FOLDER_UNSET   "UNSET"
#define CIRCE_TIME_FOLDER_LEGACY   "19700101"
#define CIRCE_TIME_MIN_VALID_YEAR  2020

typedef enum {
    CIRCE_TIME_SOURCE_NONE = 0,
    CIRCE_TIME_SOURCE_MANUAL,
    CIRCE_TIME_SOURCE_SYSTEM,
} circe_time_source_t;

void circe_time_init(void);
bool circe_time_is_set(void);
circe_time_source_t circe_time_get_source(void);
bool circe_time_has_manual_nvs(void);

void circe_time_get_local(int *year, int *month, int *day, int *hour, int *minute);
void circe_time_format_date(char *buf, size_t len);
void circe_time_format_time(char *buf, size_t len);
void circe_time_format_status(char *buf, size_t len);
bool circe_time_offset_date(int day_delta, char *out, size_t out_len);

bool circe_time_apply(int year, int month, int day, int hour, int minute);
void circe_time_fill_entry_timestamps(circe_entry_t *entry, bool is_create);
void circe_time_touch_entry_updated(circe_entry_t *entry);
bool circe_time_entry_storage_folder(const circe_entry_t *entry, char *folder, size_t folder_len);
