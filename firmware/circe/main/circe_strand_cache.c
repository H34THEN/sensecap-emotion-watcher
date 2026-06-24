#include "circe_strand_cache.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "circe_storage_paths.h"
#include "esp_log.h"
#include <sys/stat.h>

static const char *TAG = "circe_strand_cache";

static char s_cache_path[96];

static circe_strand_block_t s_cached[CIRCE_STRAND_CACHE_MAX];
static int s_cached_count = 0;

static void today_string(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    strftime(buf, len, "%Y-%m-%d", &tm_local);
}

static bool ensure_cache_dir(void)
{
    struct stat st;
    const char *dir = circe_storage_path_cache_dir();
    snprintf(s_cache_path, sizeof(s_cache_path), "%s/today_strand.json", dir);
    if (stat(dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return mkdir(dir, 0755) == 0 || stat(dir, &st) == 0;
}

static bool parse_cache_file(circe_strand_block_t *blocks, int max_blocks, int *out_count)
{
    *out_count = 0;
    FILE *f = fopen(s_cache_path, "r");
    if (!f) {
        return true;
    }

    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    if (n == 0) {
        return true;
    }
    buf[n] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        return false;
    }

    char today[16];
    today_string(today, sizeof(today));
    cJSON *jdate = cJSON_GetObjectItem(root, "date");
    cJSON *jcolors = cJSON_GetObjectItem(root, "colors");
    if (!cJSON_IsString(jdate) || strcmp(jdate->valuestring, today) != 0 || !cJSON_IsArray(jcolors)) {
        cJSON_Delete(root);
        return true;
    }

    int arr = cJSON_GetArraySize(jcolors);
    for (int i = 0; i < arr && *out_count < max_blocks; i++) {
        cJSON *item = cJSON_GetArrayItem(jcolors, i);
        if (!cJSON_IsString(item)) {
            continue;
        }
        strncpy(blocks[*out_count].color_hex, item->valuestring, sizeof(blocks[0].color_hex) - 1);
        blocks[*out_count].color_hex[sizeof(blocks[0].color_hex) - 1] = '\0';
        (*out_count)++;
    }
    cJSON_Delete(root);
    return true;
}

static bool write_cache_file(void)
{
    if (!ensure_cache_dir()) {
        return false;
    }

    cJSON *root = cJSON_CreateObject();
    char today[16];
    today_string(today, sizeof(today));
    cJSON_AddStringToObject(root, "date", today);

    cJSON *colors = cJSON_CreateArray();
    for (int i = 0; i < s_cached_count; i++) {
        cJSON_AddItemToArray(colors, cJSON_CreateString(s_cached[i].color_hex));
    }
    cJSON_AddItemToObject(root, "colors", colors);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) {
        return false;
    }

    FILE *f = fopen(s_cache_path, "w");
    if (!f) {
        cJSON_free(json);
        return false;
    }
    fputs(json, f);
    fclose(f);
    cJSON_free(json);
    return true;
}

bool circe_strand_cache_init(void)
{
    circe_storage_paths_resolve();
    ensure_cache_dir();
    s_cached_count = 0;
    if (!parse_cache_file(s_cached, CIRCE_STRAND_CACHE_MAX, &s_cached_count)) {
        ESP_LOGW(TAG, "cache parse failed");
        s_cached_count = 0;
    }
    ESP_LOGI(TAG, "strand cache: %d colors for today", s_cached_count);
    return true;
}

bool circe_strand_cache_load_today(circe_strand_block_t *blocks, int max_blocks, int *out_count)
{
    return parse_cache_file(blocks, max_blocks, out_count);
}

void circe_strand_cache_get_loaded(circe_strand_block_t *blocks, int max_blocks, int *out_count)
{
    if (!out_count) {
        return;
    }
    *out_count = 0;
    if (!blocks || max_blocks <= 0) {
        return;
    }
    int n = s_cached_count;
    if (n > max_blocks) {
        n = max_blocks;
    }
    for (int i = 0; i < n; i++) {
        blocks[i] = s_cached[i];
    }
    *out_count = n;
}

bool circe_strand_cache_append_color(const char *color_hex)
{
    if (!color_hex || color_hex[0] != '#') {
        return false;
    }
    if (s_cached_count >= CIRCE_STRAND_CACHE_MAX) {
        return write_cache_file();
    }
    strncpy(s_cached[s_cached_count].color_hex, color_hex, sizeof(s_cached[0].color_hex) - 1);
    s_cached[s_cached_count].color_hex[sizeof(s_cached[0].color_hex) - 1] = '\0';
    s_cached_count++;
    return write_cache_file();
}
