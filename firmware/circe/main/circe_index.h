#pragma once

#include "circe_entry.h"

bool circe_index_open(void);
void circe_index_close(void);
bool circe_index_insert(const circe_entry_t *entry, const char *json_path);
bool circe_index_append_best_effort(const circe_entry_t *entry, const char *json_path);
bool circe_index_delete(const char *id);
bool circe_index_get_json_path(const char *id, char *path, size_t path_len);
bool circe_index_get_latest_id(char *id_out, size_t id_len);
bool circe_index_count(int *count);
bool circe_index_clear(void);

void circe_index_mark_dirty(void);
void circe_index_clear_dirty(void);
bool circe_index_is_dirty(void);
void circe_index_load_dirty_state(void);

typedef struct {
    char id[CIRCE_MAX_ID];
    char json_path[CIRCE_MAX_JSON_PATH];
    char created_at[32];
    char local_date[CIRCE_MAX_DATE];
} circe_index_row_t;

bool circe_index_list_for_date(const char *local_date, circe_index_row_t *rows, int max_rows, int *out_count);

typedef bool (*circe_index_row_filter_fn)(const circe_index_row_t *row, void *ctx);

bool circe_index_list_collect(circe_index_row_t *rows, int max_rows, int *out_count, bool *more_exist,
                              circe_index_row_filter_fn accept, void *ctx);
