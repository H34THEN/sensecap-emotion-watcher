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
