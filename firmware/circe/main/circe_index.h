#pragma once

#include "circe_entry.h"

bool circe_index_open(void);
void circe_index_close(void);
bool circe_index_insert(const circe_entry_t *entry, const char *json_path);
bool circe_index_delete(const char *id);
bool circe_index_get_json_path(const char *id, char *path, size_t path_len);
bool circe_index_get_latest_id(char *id_out, size_t id_len);
bool circe_index_count(int *count);
bool circe_index_clear(void);

typedef struct {
    char id[CIRCE_MAX_ID];
    char json_path[CIRCE_MAX_JSON_PATH];
    char created_at[32];
} circe_index_row_t;

bool circe_index_list_for_date(const char *local_date, circe_index_row_t *rows, int max_rows, int *out_count);
