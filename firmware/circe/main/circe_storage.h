#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "circe_entry.h"

typedef struct {
    bool sd_mounted;
    bool writable;
    bool storage_ready;
    bool probe_passed;
    bool entries_dir_ok;
    bool index_dir_ok;
    bool index_dirty;
    int entry_count;
    long card_size_mb;
    char base_path[64];
    char entries_path[80];
    char index_path[96];
    char last_error[128];
    char probe_detail[64];
} circe_storage_health_t;

typedef struct {
    bool json_ok;
    bool index_ok;
    bool load_ok;
    bool delete_ok;
    char summary[160];
} circe_save_self_test_result_t;

typedef struct {
    bool passed;
    bool write_ok;
    bool read_ok;
    bool delete_ok;
    char detail[96];
} circe_storage_probe_result_t;

bool circe_storage_init(void);
void circe_storage_deinit(void);
bool circe_storage_reinit(void);
bool circe_storage_is_ready(void);
bool circe_storage_ensure_ready(void);
bool circe_storage_run_probe(circe_storage_probe_result_t *out);

bool circe_entry_create(circe_entry_t *entry, circe_entry_mode_t mode);
bool circe_entry_save_json_atomic(const circe_entry_t *entry);
bool circe_storage_last_save_non_atomic(void);
bool circe_entry_index_insert(const circe_entry_t *entry);
bool circe_entry_index_insert_best_effort(const circe_entry_t *entry);
bool circe_entry_load(const char *id, circe_entry_t *entry);
bool circe_entry_delete_hard(const char *id);
bool circe_rebuild_index_from_json(int *out_count);
bool circe_storage_rebuild_index_if_dirty(int *out_count);
bool circe_storage_health_check(circe_storage_health_t *health);

bool circe_storage_get_latest_entry_id(char *id_out, size_t id_len);
bool circe_storage_find_json_path_for_id(const char *id, char *path_out, size_t path_len);
void circe_storage_entry_json_path(const circe_entry_t *entry, char *path, size_t len);
bool circe_storage_run_self_test(void);
bool circe_storage_run_save_self_test(circe_save_self_test_result_t *out);
bool circe_entry_update(circe_entry_t *entry);

typedef struct {
    char color_hex[CIRCE_MAX_COLOR];
} circe_strand_block_t;

bool circe_storage_today_strand(circe_strand_block_t *blocks, int max_blocks, int *out_count);
void circe_storage_set_last_error(const char *msg);
const char *circe_storage_get_last_error(void);
