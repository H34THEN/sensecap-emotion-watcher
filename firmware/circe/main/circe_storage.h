#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "circe_entry.h"

typedef struct {
    bool sd_mounted;
    bool index_open;
    int entry_count;
    char base_path[64];
    char last_error[128];
} circe_storage_health_t;

bool circe_storage_init(void);
void circe_storage_deinit(void);

bool circe_entry_create(circe_entry_t *entry, circe_entry_mode_t mode);
bool circe_entry_save_json_atomic(const circe_entry_t *entry);
bool circe_entry_index_insert(const circe_entry_t *entry);
bool circe_entry_load(const char *id, circe_entry_t *entry);
bool circe_entry_delete_hard(const char *id);
bool circe_rebuild_index_from_json(int *out_count);
bool circe_storage_health_check(circe_storage_health_t *health);

bool circe_storage_get_latest_entry_id(char *id_out, size_t id_len);
bool circe_storage_run_self_test(void);
