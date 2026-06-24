#include "circe_index.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cJSON.h"
#include "esp_log.h"

static const char *TAG = "circe_index";

#define CIRCE_INDEX_PATH "/sdcard/circe/index/entry_index.jsonl"

static bool s_open = false;

bool circe_index_open(void)
{
    s_open = true;
    return true;
}

void circe_index_close(void)
{
    s_open = false;
}

static bool rewrite_index_filter(bool (*drop_line)(cJSON *obj, void *ctx), void *ctx)
{
    FILE *in = fopen(CIRCE_INDEX_PATH, "r");
    FILE *out = fopen(CIRCE_INDEX_PATH ".tmp", "w");
    if (!out) {
        if (in) {
            fclose(in);
        }
        return false;
    }
    char line[640];
    if (in) {
        while (fgets(line, sizeof(line), in)) {
            cJSON *obj = cJSON_Parse(line);
            if (!obj) {
                continue;
            }
            if (!drop_line || !drop_line(obj, ctx)) {
                char *printed = cJSON_PrintUnformatted(obj);
                if (printed) {
                    fprintf(out, "%s\n", printed);
                    cJSON_free(printed);
                }
            }
            cJSON_Delete(obj);
        }
        fclose(in);
    }
    fflush(out);
    fsync(fileno(out));
    fclose(out);
    rename(CIRCE_INDEX_PATH ".tmp", CIRCE_INDEX_PATH);
    return true;
}

static bool drop_matching_id(cJSON *obj, void *ctx)
{
    const char *id = (const char *)ctx;
    cJSON *jid = cJSON_GetObjectItem(obj, "id");
    return cJSON_IsString(jid) && jid->valuestring && strcmp(jid->valuestring, id) == 0;
}

bool circe_index_insert(const circe_entry_t *entry, const char *json_path)
{
    if (!s_open || !entry) {
        return false;
    }
    rewrite_index_filter(drop_matching_id, (void *)entry->id);

    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "id", entry->id);
    cJSON_AddStringToObject(obj, "local_date", entry->local_date);
    cJSON_AddStringToObject(obj, "created_at", entry->created_at);
    cJSON_AddStringToObject(obj, "json_path", json_path);
    cJSON_AddStringToObject(obj, "lifecycle_state", entry->lifecycle_state == CIRCE_LIFECYCLE_ACTIVE ? "active" : "deleted");
    char *line = cJSON_PrintUnformatted(obj);
    cJSON_Delete(obj);
    if (!line) {
        return false;
    }

    FILE *f = fopen(CIRCE_INDEX_PATH, "a");
    if (!f) {
        cJSON_free(line);
        return false;
    }
    fprintf(f, "%s\n", line);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    cJSON_free(line);
    return true;
}

bool circe_index_delete(const char *id)
{
    if (!id) {
        return false;
    }
    return rewrite_index_filter(drop_matching_id, (void *)id);
}

static bool scan_index(const char *want_id, char *path, size_t path_len, char *latest_id, size_t latest_id_len)
{
    FILE *f = fopen(CIRCE_INDEX_PATH, "r");
    if (!f) {
        return false;
    }
    char line[640];
    char best_at[32] = {0};
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        cJSON *obj = cJSON_Parse(line);
        if (!obj) {
            continue;
        }
        cJSON *jid = cJSON_GetObjectItem(obj, "id");
        cJSON *jpath = cJSON_GetObjectItem(obj, "json_path");
        cJSON *jstate = cJSON_GetObjectItem(obj, "lifecycle_state");
        cJSON *jcreated = cJSON_GetObjectItem(obj, "created_at");
        if (!cJSON_IsString(jid) || !cJSON_IsString(jpath)) {
            cJSON_Delete(obj);
            continue;
        }
        if (cJSON_IsString(jstate) && strcmp(jstate->valuestring, "active") != 0) {
            cJSON_Delete(obj);
            continue;
        }
        if (want_id && strcmp(jid->valuestring, want_id) == 0) {
            strncpy(path, jpath->valuestring, path_len - 1);
            path[path_len - 1] = '\0';
            found = true;
        }
        if (latest_id && jcreated && cJSON_IsString(jcreated)) {
            if (!best_at[0] || strcmp(jcreated->valuestring, best_at) > 0) {
                strncpy(best_at, jcreated->valuestring, sizeof(best_at) - 1);
                strncpy(latest_id, jid->valuestring, latest_id_len - 1);
                latest_id[latest_id_len - 1] = '\0';
            }
        }
        cJSON_Delete(obj);
    }
    fclose(f);
    return found;
}

bool circe_index_get_json_path(const char *id, char *path, size_t path_len)
{
    return scan_index(id, path, path_len, NULL, 0);
}

bool circe_index_get_latest_id(char *id_out, size_t id_len)
{
    if (!id_out) {
        return false;
    }
    id_out[0] = '\0';
    scan_index(NULL, NULL, 0, id_out, id_len);
    return id_out[0] != '\0';
}

bool circe_index_count(int *count)
{
    if (!count) {
        return false;
    }
    *count = 0;
    FILE *f = fopen(CIRCE_INDEX_PATH, "r");
    if (!f) {
        return true;
    }
    char line[640];
    while (fgets(line, sizeof(line), f)) {
        cJSON *obj = cJSON_Parse(line);
        if (!obj) {
            continue;
        }
        cJSON *jstate = cJSON_GetObjectItem(obj, "lifecycle_state");
        if (!cJSON_IsString(jstate) || strcmp(jstate->valuestring, "active") == 0) {
            (*count)++;
        }
        cJSON_Delete(obj);
    }
    fclose(f);
    return true;
}

bool circe_index_clear(void)
{
    FILE *f = fopen(CIRCE_INDEX_PATH, "w");
    if (!f) {
        return false;
    }
    fclose(f);
    return true;
}
