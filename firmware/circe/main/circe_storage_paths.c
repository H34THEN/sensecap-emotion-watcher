#include "circe_storage_paths.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_log.h"

static const char *TAG = "circe_paths";

static char s_base[64] = "/sdcard/CIRCE";
static char s_entries[80];
static char s_index_dir[80];
static char s_index_file[96];
static char s_index_dirty[96];
static char s_cache_dir[80];
static char s_logs_dir[80];
static char s_photos_dir[80];
static char s_probe_tmp[96];

static bool dir_exists(const char *path)
{
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool circe_storage_path_join(char *out, size_t out_len, const char *dir, const char *name)
{
    if (!out || out_len == 0 || !dir || !name) {
        return false;
    }
    int n = snprintf(out, out_len, "%s/%s", dir, name);
    if (n < 0 || (size_t)n >= out_len) {
        ESP_LOGE(TAG, "PATH_TOO_LONG dir='%s' name='%s' need=%d have=%u", dir, name, n,
                 (unsigned)out_len);
        out[0] = '\0';
        return false;
    }
    out[n] = '\0';
    return true;
}

void circe_storage_path_entry_folder(const char *local_date, char *out, size_t out_len)
{
    if (!out || out_len == 0) {
        return;
    }
    out[0] = '\0';
    if (!local_date || local_date[0] == '\0') {
        return;
    }
    if (strlen(local_date) == 10 && local_date[4] == '-' && local_date[7] == '-') {
        snprintf(out, out_len, "%.4s%.2s%.2s", local_date, local_date + 5, local_date + 8);
    } else {
        strncpy(out, local_date, out_len - 1);
        out[out_len - 1] = '\0';
    }
}

static bool set_subdir(char *out, size_t out_len, const char *base, const char *const *names)
{
    for (int i = 0; names[i]; i++) {
        if (!circe_storage_path_join(out, out_len, base, names[i])) {
            return false;
        }
        if (dir_exists(out)) {
            ESP_LOGI(TAG, "using existing dir %s", out);
            return true;
        }
    }
    return circe_storage_path_join(out, out_len, base, names[0]);
}

void circe_storage_paths_resolve(void)
{
    static const char *base_candidates[] = {"/sdcard/CIRCE", "/sdcard/circe", "/sdcard/Circe", NULL};
    bool found = false;

    for (int i = 0; base_candidates[i]; i++) {
        if (dir_exists(base_candidates[i])) {
            strncpy(s_base, base_candidates[i], sizeof(s_base) - 1);
            s_base[sizeof(s_base) - 1] = '\0';
            found = true;
            ESP_LOGI(TAG, "resolved base from existing: %s", s_base);
            break;
        }
    }
    if (!found) {
        strncpy(s_base, "/sdcard/CIRCE", sizeof(s_base) - 1);
        s_base[sizeof(s_base) - 1] = '\0';
        ESP_LOGI(TAG, "using canonical base: %s", s_base);
    }

    static const char *entries_names[] = {"ENTRIES", "entries", NULL};
    static const char *index_names[] = {"INDEX", "index", NULL};
    static const char *cache_names[] = {"CACHE", "cache", NULL};
    static const char *logs_names[] = {"LOGS", "logs", NULL};
    static const char *photos_names[] = {"PHOTOS", "photos", NULL};

    set_subdir(s_entries, sizeof(s_entries), s_base, entries_names);
    set_subdir(s_index_dir, sizeof(s_index_dir), s_base, index_names);
    set_subdir(s_cache_dir, sizeof(s_cache_dir), s_base, cache_names);
    set_subdir(s_logs_dir, sizeof(s_logs_dir), s_base, logs_names);
    set_subdir(s_photos_dir, sizeof(s_photos_dir), s_base, photos_names);

    char index_new[96];
    char index_legacy[96];
    char dirty_new[96];
    char dirty_legacy[96];
    circe_storage_path_join(index_new, sizeof(index_new), s_index_dir, "ENTRY.IDX");
    circe_storage_path_join(index_legacy, sizeof(index_legacy), s_index_dir, "entry_index.jsonl");
    struct stat st;
    if (stat(index_legacy, &st) == 0) {
        strncpy(s_index_file, index_legacy, sizeof(s_index_file) - 1);
        s_index_file[sizeof(s_index_file) - 1] = '\0';
        ESP_LOGI(TAG, "using existing index %s", s_index_file);
    } else if (stat(index_new, &st) == 0) {
        strncpy(s_index_file, index_new, sizeof(s_index_file) - 1);
        s_index_file[sizeof(s_index_file) - 1] = '\0';
        ESP_LOGI(TAG, "using existing index %s", s_index_file);
    } else if (!circe_storage_path_join(s_index_file, sizeof(s_index_file), s_index_dir, "ENTRY.IDX")) {
        ESP_LOGE(TAG, "index file path build failed");
    } else {
        ESP_LOGI(TAG, "using new index path %s", s_index_file);
    }

    circe_storage_path_join(dirty_new, sizeof(dirty_new), s_index_dir, "DIRTY.TAG");
    circe_storage_path_join(dirty_legacy, sizeof(dirty_legacy), s_index_dir, ".dirty");
    if (stat(dirty_legacy, &st) == 0) {
        strncpy(s_index_dirty, dirty_legacy, sizeof(s_index_dirty) - 1);
        s_index_dirty[sizeof(s_index_dirty) - 1] = '\0';
        ESP_LOGI(TAG, "using existing dirty marker %s", s_index_dirty);
    } else if (!circe_storage_path_join(s_index_dirty, sizeof(s_index_dirty), s_index_dir, "DIRTY.TAG")) {
        ESP_LOGE(TAG, "index dirty path build failed");
    }
    if (!circe_storage_path_join(s_probe_tmp, sizeof(s_probe_tmp), s_logs_dir, "PROBE.TMP")) {
        ESP_LOGE(TAG, "probe path build failed");
    }

    ESP_LOGI(TAG, "paths probe='%s' len=%u", s_probe_tmp, (unsigned)strlen(s_probe_tmp));
}

const char *circe_storage_path_base(void)
{
    return s_base;
}

const char *circe_storage_path_entries(void)
{
    return s_entries;
}

const char *circe_storage_path_index_dir(void)
{
    return s_index_dir;
}

const char *circe_storage_path_index_file(void)
{
    return s_index_file;
}

const char *circe_storage_path_index_dirty(void)
{
    return s_index_dirty;
}

const char *circe_storage_path_cache_dir(void)
{
    return s_cache_dir;
}

const char *circe_storage_path_logs_dir(void)
{
    return s_logs_dir;
}

const char *circe_storage_path_photos(void)
{
    return s_photos_dir;
}

const char *circe_storage_path_probe_tmp(void)
{
    return s_probe_tmp;
}

const char *circe_storage_path_entry_ext(void)
{
    return CIRCE_ENTRY_FILE_EXT;
}
