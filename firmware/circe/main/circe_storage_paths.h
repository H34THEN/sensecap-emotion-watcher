#pragma once

#include <stdbool.h>
#include <stddef.h>

#define CIRCE_ENTRY_FILE_EXT ".JSN"

void circe_storage_paths_resolve(void);
const char *circe_storage_path_base(void);
const char *circe_storage_path_entries(void);
const char *circe_storage_path_index_dir(void);
const char *circe_storage_path_index_file(void);
const char *circe_storage_path_index_dirty(void);
const char *circe_storage_path_cache_dir(void);
const char *circe_storage_path_logs_dir(void);
const char *circe_storage_path_probe_tmp(void);
const char *circe_storage_path_entry_ext(void);

bool circe_storage_path_join(char *out, size_t out_len, const char *dir, const char *name);
void circe_storage_path_entry_folder(const char *local_date, char *out, size_t out_len);
