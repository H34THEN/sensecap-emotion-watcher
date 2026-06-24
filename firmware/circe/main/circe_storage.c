#include "circe_storage.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "circe_index.h"
#include "esp_log.h"

static const char *TAG = "circe_storage";

const char *CIRCE_BASE_PATH = "/sdcard/circe";
#define CIRCE_ENTRIES      "/sdcard/circe/entries"
#define CIRCE_INDEX_DIR    "/sdcard/circe/index"

static bool s_ready = false;
static char s_last_error[128] = {0};

void circe_storage_set_last_error(const char *msg)
{
    if (!msg) {
        s_last_error[0] = '\0';
        return;
    }
    strncpy(s_last_error, msg, sizeof(s_last_error) - 1);
    s_last_error[sizeof(s_last_error) - 1] = '\0';
}

const char *circe_storage_get_last_error(void)
{
    return s_last_error;
}

static bool mkdir_p(const char *path)
{
    char tmp[128];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            *p = '/';
        }
    }
    return mkdir(tmp, 0755) == 0 || errno == EEXIST;
}

static void entry_json_path(const circe_entry_t *entry, char *path, size_t len)
{
    snprintf(path, len, CIRCE_ENTRIES "/%s/%s.json", entry->local_date, entry->id);
}

bool circe_storage_init(void)
{
    if (s_ready) {
        return true;
    }
    if (!mkdir_p(CIRCE_BASE_PATH) || !mkdir_p(CIRCE_ENTRIES) || !mkdir_p(CIRCE_INDEX_DIR)) {
        ESP_LOGE(TAG, "mkdir failed");
        circe_storage_set_last_error("Failed to create storage directories");
        return false;
    }
    if (!circe_index_open()) {
        circe_storage_set_last_error("Failed to open index");
        return false;
    }
    s_ready = true;
    circe_storage_set_last_error("");
    ESP_LOGI(TAG, "storage ready at %s", CIRCE_BASE_PATH);
    return true;
}

void circe_storage_deinit(void)
{
    circe_index_close();
    s_ready = false;
}

bool circe_entry_create(circe_entry_t *entry, circe_entry_mode_t mode)
{
    circe_entry_init_defaults(entry, mode);
    return true;
}

bool circe_entry_save_json_atomic(const circe_entry_t *entry)
{
    if (!s_ready || !entry) {
        return false;
    }

    char dir_path[96];
    snprintf(dir_path, sizeof(dir_path), CIRCE_ENTRIES "/%s", entry->local_date);
    if (!mkdir_p(dir_path)) {
        ESP_LOGE(TAG, "mkdir day failed");
        return false;
    }

    char final_path[128];
    char temp_path[140];
    entry_json_path(entry, final_path, sizeof(final_path));
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", final_path);

    char json[2048];
    if (!circe_entry_to_json(entry, json, sizeof(json))) {
        return false;
    }

    FILE *f = fopen(temp_path, "w");
    if (!f) {
        ESP_LOGE(TAG, "open temp failed");
        circe_storage_set_last_error("Failed to write entry file");
        return false;
    }
    size_t n = fwrite(json, 1, strlen(json), f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    if (n != strlen(json)) {
        unlink(temp_path);
        return false;
    }
    if (rename(temp_path, final_path) != 0) {
        ESP_LOGE(TAG, "rename failed");
        unlink(temp_path);
        circe_storage_set_last_error("Failed to finalize entry file");
        return false;
    }
    return true;
}

bool circe_entry_update(circe_entry_t *entry)
{
    if (!s_ready || !entry || !entry->id[0]) {
        circe_storage_set_last_error("Invalid entry for update");
        return false;
    }
    entry->revision++;
    circe_entry_touch_updated(entry);
    entry->training_ok = false;
    entry->private_locked = true;
    if (!circe_entry_save_json_atomic(entry)) {
        circe_storage_set_last_error("Update save failed");
        return false;
    }
    if (!circe_entry_index_insert(entry)) {
        circe_storage_set_last_error("Update index failed");
        return false;
    }
    circe_storage_set_last_error("");
    return true;
}

bool circe_entry_index_insert(const circe_entry_t *entry)
{
    char json_path[128];
    entry_json_path(entry, json_path, sizeof(json_path));
    return circe_index_insert(entry, json_path);
}

bool circe_entry_load(const char *id, circe_entry_t *entry)
{
    if (!id || !entry) {
        return false;
    }
    char json_path[128];
    if (!circe_index_get_json_path(id, json_path, sizeof(json_path))) {
        return false;
    }
    FILE *f = fopen(json_path, "r");
    if (!f) {
        return false;
    }
    char json[2048];
    size_t n = fread(json, 1, sizeof(json) - 1, f);
    json[n] = '\0';
    fclose(f);
    if (!circe_entry_from_json(json, entry)) {
        return false;
    }
    strncpy(entry->json_path, json_path, sizeof(entry->json_path) - 1);
    return true;
}

bool circe_entry_delete_hard(const char *id)
{
    if (!id) {
        return false;
    }
    char json_path[128] = {0};
    circe_index_get_json_path(id, json_path, sizeof(json_path));
    if (json_path[0]) {
        unlink(json_path);
    }
    return circe_index_delete(id);
}

static bool index_one_json_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        return false;
    }
    char json[2048];
    size_t n = fread(json, 1, sizeof(json) - 1, f);
    json[n] = '\0';
    fclose(f);

    circe_entry_t entry;
    if (!circe_entry_from_json(json, &entry)) {
        return false;
    }
    if (entry.lifecycle_state != CIRCE_LIFECYCLE_ACTIVE) {
        return true;
    }
    return circe_entry_index_insert(&entry);
}

static void scan_dir_recursive(const char *dir, int *count)
{
    DIR *d = opendir(dir);
    if (!d) {
        return;
    }
    struct dirent *ent;
    char path[256];
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(path, &st) != 0) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            scan_dir_recursive(path, count);
        } else if (S_ISREG(st.st_mode) && strstr(ent->d_name, ".json") && !strstr(ent->d_name, ".tmp")) {
            if (index_one_json_file(path)) {
                (*count)++;
            }
        }
    }
    closedir(d);
}

bool circe_rebuild_index_from_json(int *out_count)
{
    if (!circe_index_clear()) {
        return false;
    }
    int count = 0;
    scan_dir_recursive(CIRCE_ENTRIES, &count);
    if (out_count) {
        *out_count = count;
    }
    ESP_LOGI(TAG, "rebuilt index: %d entries", count);
    return true;
}

bool circe_storage_health_check(circe_storage_health_t *health)
{
    if (!health) {
        return false;
    }
    memset(health, 0, sizeof(*health));
    strncpy(health->base_path, CIRCE_BASE_PATH, sizeof(health->base_path) - 1);
    health->sd_mounted = (access("/sdcard", W_OK) == 0);
    health->index_open = s_ready;
    circe_index_count(&health->entry_count);
    if (!health->sd_mounted) {
        strncpy(health->last_error, "SD card not mounted", sizeof(health->last_error) - 1);
    } else if (!health->index_open) {
        strncpy(health->last_error, "Storage not initialized", sizeof(health->last_error) - 1);
    } else if (s_last_error[0]) {
        strncpy(health->last_error, s_last_error, sizeof(health->last_error) - 1);
    }
    return health->sd_mounted && health->index_open;
}

bool circe_storage_today_strand(circe_strand_block_t *blocks, int max_blocks, int *out_count)
{
    if (!blocks || max_blocks <= 0 || !out_count) {
        return false;
    }
    *out_count = 0;
    char today[CIRCE_MAX_DATE];
    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    strftime(today, sizeof(today), "%Y-%m-%d", &tm_local);

    circe_index_row_t rows[32];
    int row_count = 0;
    if (!circe_index_list_for_date(today, rows, 32, &row_count)) {
        return false;
    }
    for (int i = 0; i < row_count && *out_count < max_blocks; i++) {
        circe_entry_t entry;
        if (!circe_entry_load(rows[i].id, &entry)) {
            continue;
        }
        strncpy(blocks[*out_count].color_hex, entry.color_hex, sizeof(blocks[0].color_hex) - 1);
        blocks[*out_count].color_hex[sizeof(blocks[0].color_hex) - 1] = '\0';
        (*out_count)++;
    }
    return true;
}

bool circe_storage_get_latest_entry_id(char *id_out, size_t id_len)
{
    return circe_index_get_latest_id(id_out, id_len);
}

static bool save_and_index(circe_entry_t *entry)
{
    if (!circe_entry_save_json_atomic(entry)) {
        return false;
    }
    return circe_entry_index_insert(entry);
}

bool circe_storage_run_self_test(void)
{
    circe_entry_t e;
    circe_entry_create(&e, CIRCE_ENTRY_MODE_QUICK);
    strncpy(e.body_areas[0], "chest", sizeof(e.body_areas[0]) - 1);
    e.body_areas[0][sizeof(e.body_areas[0]) - 1] = '\0';
    e.body_area_count = 1;
    strncpy(e.body_sensations[0], "tight", sizeof(e.body_sensations[0]) - 1);
    e.body_sensations[0][sizeof(e.body_sensations[0]) - 1] = '\0';
    e.body_sensation_count = 1;
    snprintf(e.color_hex, sizeof(e.color_hex), "#4A5568");

    if (!save_and_index(&e)) {
        ESP_LOGE(TAG, "self test save failed");
        return false;
    }

    circe_entry_t loaded;
    if (!circe_entry_load(e.id, &loaded)) {
        ESP_LOGE(TAG, "self test load failed");
        return false;
    }
    if (!loaded.private_locked || loaded.training_ok || strcmp(loaded.emotion, CIRCE_EMOTION_UNKNOWN) != 0) {
        ESP_LOGE(TAG, "self test defaults failed");
        return false;
    }
    if (!circe_entry_delete_hard(e.id)) {
        ESP_LOGE(TAG, "self test delete failed");
        return false;
    }
    ESP_LOGI(TAG, "storage self test passed");
    return true;
}
