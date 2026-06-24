#include "circe_index.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "circe_buf.h"
#include "circe_storage_paths.h"
#include "esp_log.h"

static const char *TAG = "circe_index";

static bool s_open = false;
static bool s_index_dirty = false;

static bool ensure_index_dir(void)
{
    return mkdir(circe_storage_path_index_dir(), 0755) == 0 || errno == EEXIST;
}

void circe_index_mark_dirty(void)
{
    s_index_dirty = true;
    ensure_index_dir();
    FILE *f = fopen(circe_storage_path_index_dirty(), "w");
    if (f) {
        fputc('1', f);
        fclose(f);
    }
    ESP_LOGW(TAG, "index marked dirty");
}

void circe_index_clear_dirty(void)
{
    s_index_dirty = false;
    unlink(circe_storage_path_index_dirty());
}

bool circe_index_is_dirty(void)
{
    return s_index_dirty;
}

void circe_index_load_dirty_state(void)
{
    s_index_dirty = (access(circe_storage_path_index_dirty(), F_OK) == 0);
    if (s_index_dirty) {
        ESP_LOGW(TAG, "index dirty flag present on boot");
    }
}

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
    const char *index_path = circe_storage_path_index_file();
    char tmp_path[120];
    if (!circe_storage_path_join(tmp_path, sizeof(tmp_path), circe_storage_path_index_dir(), "ENTRY.TMP")) {
        return false;
    }

    FILE *in = fopen(index_path, "r");
    FILE *out = fopen(tmp_path, "w");
    if (!out) {
        if (in) {
            fclose(in);
        }
        return false;
    }
    char *line = circe_buf_alloc(CIRCE_INDEX_LINE_SIZE);
    if (!line) {
        if (in) {
            fclose(in);
        }
        fclose(out);
        unlink(tmp_path);
        return false;
    }
    if (in) {
        while (fgets(line, CIRCE_INDEX_LINE_SIZE, in)) {
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
    circe_buf_free(line);
    fflush(out);
    fsync(fileno(out));
    fclose(out);
    rename(tmp_path, index_path);
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
    if (!ensure_index_dir()) {
        ESP_LOGE(TAG, "index dir missing");
        return false;
    }

    char existing[128];
    if (circe_index_get_json_path(entry->id, existing, sizeof(existing))) {
        if (!rewrite_index_filter(drop_matching_id, (void *)entry->id)) {
            ESP_LOGE(TAG, "index rewrite failed for id=%s", entry->id);
            return false;
        }
    }

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

    FILE *f = fopen(circe_storage_path_index_file(), "a");
    if (!f) {
        cJSON_free(line);
        ESP_LOGE(TAG, "index append open failed path=%s errno=%d", circe_storage_path_index_file(), errno);
        return false;
    }
    fprintf(f, "%s\n", line);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    cJSON_free(line);
    return true;
}

bool circe_index_append_best_effort(const circe_entry_t *entry, const char *json_path)
{
    if (circe_index_insert(entry, json_path)) {
        return true;
    }
    circe_index_mark_dirty();
    ESP_LOGW(TAG, "index append best-effort failed id=%s path=%s", entry ? entry->id : "?",
             json_path ? json_path : "?");
    return false;
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
    FILE *f = fopen(circe_storage_path_index_file(), "r");
    if (!f) {
        return false;
    }
    char *line = circe_buf_alloc(CIRCE_INDEX_LINE_SIZE);
    if (!line) {
        fclose(f);
        return false;
    }
    char best_at[32] = {0};
    bool found = false;
    while (fgets(line, CIRCE_INDEX_LINE_SIZE, f)) {
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
    circe_buf_free(line);
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
    FILE *f = fopen(circe_storage_path_index_file(), "r");
    if (!f) {
        return true;
    }
    char *line = circe_buf_alloc(CIRCE_INDEX_LINE_SIZE);
    if (!line) {
        fclose(f);
        return false;
    }
    while (fgets(line, CIRCE_INDEX_LINE_SIZE, f)) {
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
    circe_buf_free(line);
    fclose(f);
    return true;
}

bool circe_index_clear(void)
{
    FILE *f = fopen(circe_storage_path_index_file(), "w");
    if (!f) {
        return false;
    }
    fclose(f);
    return true;
}

bool circe_index_list_for_date(const char *local_date, circe_index_row_t *rows, int max_rows, int *out_count)
{
    if (!local_date || !rows || max_rows <= 0 || !out_count) {
        return false;
    }
    *out_count = 0;
    FILE *f = fopen(circe_storage_path_index_file(), "r");
    if (!f) {
        return true;
    }
    char *line = circe_buf_alloc(CIRCE_INDEX_LINE_SIZE);
    if (!line) {
        fclose(f);
        return false;
    }
    while (fgets(line, CIRCE_INDEX_LINE_SIZE, f)) {
        cJSON *obj = cJSON_Parse(line);
        if (!obj) {
            continue;
        }
        cJSON *jdate = cJSON_GetObjectItem(obj, "local_date");
        cJSON *jstate = cJSON_GetObjectItem(obj, "lifecycle_state");
        cJSON *jid = cJSON_GetObjectItem(obj, "id");
        cJSON *jpath = cJSON_GetObjectItem(obj, "json_path");
        cJSON *jcreated = cJSON_GetObjectItem(obj, "created_at");
        if (!cJSON_IsString(jdate) || strcmp(jdate->valuestring, local_date) != 0) {
            cJSON_Delete(obj);
            continue;
        }
        if (cJSON_IsString(jstate) && strcmp(jstate->valuestring, "active") != 0) {
            cJSON_Delete(obj);
            continue;
        }
        if (*out_count >= max_rows || !cJSON_IsString(jid) || !cJSON_IsString(jpath)) {
            cJSON_Delete(obj);
            continue;
        }
        circe_index_row_t *row = &rows[*out_count];
        strncpy(row->id, jid->valuestring, sizeof(row->id) - 1);
        row->id[sizeof(row->id) - 1] = '\0';
        strncpy(row->json_path, jpath->valuestring, sizeof(row->json_path) - 1);
        row->json_path[sizeof(row->json_path) - 1] = '\0';
        if (cJSON_IsString(jcreated)) {
            strncpy(row->created_at, jcreated->valuestring, sizeof(row->created_at) - 1);
            row->created_at[sizeof(row->created_at) - 1] = '\0';
        } else {
            row->created_at[0] = '\0';
        }
        (*out_count)++;
        cJSON_Delete(obj);
    }
    circe_buf_free(line);
    fclose(f);

    for (int i = 0; i < *out_count - 1; i++) {
        for (int j = i + 1; j < *out_count; j++) {
            if (strcmp(rows[j].created_at, rows[i].created_at) < 0) {
                circe_index_row_t tmp = rows[i];
                rows[i] = rows[j];
                rows[j] = tmp;
            }
        }
    }
    return true;
}
