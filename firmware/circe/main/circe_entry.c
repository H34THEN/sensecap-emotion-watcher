#include "circe_entry.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cJSON.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "circe_entry";

static void iso8601_now(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
}

static void local_date_now(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    strftime(buf, len, "%Y-%m-%d", &tm_local);
}

void circe_entry_generate_id(circe_entry_t *entry)
{
    uint32_t rnd = esp_random();
    snprintf(entry->id, sizeof(entry->id), "%08X", (unsigned)rnd);
}

void circe_entry_init_defaults(circe_entry_t *entry, circe_entry_mode_t mode)
{
    memset(entry, 0, sizeof(*entry));
    circe_entry_generate_id(entry);
    circe_entry_set_timestamp_now(entry);
    entry->entry_mode = mode;
    entry->interaction_mode.short_answer = (mode == CIRCE_ENTRY_MODE_QUICK);
    strncpy(entry->emotion, CIRCE_EMOTION_UNKNOWN, sizeof(entry->emotion) - 1);
    entry->emotion[sizeof(entry->emotion) - 1] = '\0';
    strncpy(entry->color_hex, "#808080", sizeof(entry->color_hex) - 1);
    entry->color_hex[sizeof(entry->color_hex) - 1] = '\0';
    entry->intensity = 5;
    entry->training_ok = false;
    entry->private_locked = true;
    entry->lifecycle_state = CIRCE_LIFECYCLE_ACTIVE;
    entry->revision = 1;
    strncpy(entry->schema_version, CIRCE_SCHEMA_VERSION, sizeof(entry->schema_version) - 1);
    strncpy(entry->source, CIRCE_SOURCE, sizeof(entry->source) - 1);
    strncpy(entry->timezone_at_capture, "UTC", sizeof(entry->timezone_at_capture) - 1);
    entry->sleep = 0;
    entry->energy = 0;
    entry->stress = 0;
    entry->has_sleep = false;
    entry->has_energy = false;
    entry->has_stress = false;
}

void circe_entry_set_timestamp_now(circe_entry_t *entry)
{
    iso8601_now(entry->created_at, sizeof(entry->created_at));
    iso8601_now(entry->updated_at, sizeof(entry->updated_at));
    local_date_now(entry->local_date, sizeof(entry->local_date));
}

void circe_entry_touch_updated(circe_entry_t *entry)
{
    iso8601_now(entry->updated_at, sizeof(entry->updated_at));
}

const char *circe_entry_mode_str(circe_entry_mode_t mode)
{
    switch (mode) {
    case CIRCE_ENTRY_MODE_QUICK:
        return "quick";
    default:
        return "body_only";
    }
}

static cJSON *string_array_from_list(const char items[][32], int count, int max_item_len)
{
    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < count; i++) {
        cJSON_AddItemToArray(arr, cJSON_CreateString(items[i]));
    }
    (void)max_item_len;
    return arr;
}

bool circe_entry_to_json(const circe_entry_t *entry, char *out, size_t out_len)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "schema_version", entry->schema_version);
    cJSON_AddStringToObject(root, "id", entry->id);
    cJSON_AddStringToObject(root, "created_at", entry->created_at);
    cJSON_AddStringToObject(root, "updated_at", entry->updated_at);
    cJSON_AddStringToObject(root, "local_date", entry->local_date);
    cJSON_AddStringToObject(root, "timezone_at_capture", entry->timezone_at_capture);
    cJSON_AddStringToObject(root, "entry_mode", circe_entry_mode_str(entry->entry_mode));
    cJSON *im = cJSON_CreateObject();
    cJSON_AddBoolToObject(im, "short_answer", entry->interaction_mode.short_answer);
    cJSON_AddBoolToObject(im, "icon_first", entry->interaction_mode.icon_first);
    cJSON_AddBoolToObject(im, "low_energy", entry->interaction_mode.low_energy);
    cJSON_AddItemToObject(root, "interaction_mode", im);
    cJSON_AddStringToObject(root, "emotion", entry->emotion);
    cJSON_AddStringToObject(root, "emotion_family", entry->emotion_family[0] ? entry->emotion_family : "");
    cJSON_AddStringToObject(root, "color_hex", entry->color_hex);
    cJSON_AddNumberToObject(root, "intensity", entry->intensity);
    cJSON_AddItemToObject(root, "body_areas", string_array_from_list((const char (*)[32])entry->body_areas, entry->body_area_count, 24));
    cJSON_AddItemToObject(root, "body_sensations",
                          string_array_from_list((const char (*)[32])entry->body_sensations, entry->body_sensation_count, 32));
    if (entry->has_sleep) {
        cJSON_AddNumberToObject(root, "sleep", entry->sleep);
    } else {
        cJSON_AddNullToObject(root, "sleep");
    }
    if (entry->has_energy) {
        cJSON_AddNumberToObject(root, "energy", entry->energy);
    } else {
        cJSON_AddNullToObject(root, "energy");
    }
    if (entry->has_stress) {
        cJSON_AddNumberToObject(root, "stress", entry->stress);
    } else {
        cJSON_AddNullToObject(root, "stress");
    }
    cJSON *tags = cJSON_CreateArray();
    for (int i = 0; i < entry->context_tag_count; i++) {
        cJSON_AddItemToArray(tags, cJSON_CreateString(entry->context_tags[i]));
    }
    cJSON_AddItemToObject(root, "context_tags", tags);
    cJSON_AddStringToObject(root, "summary", entry->summary);
    cJSON_AddBoolToObject(root, "training_ok", entry->training_ok);
    cJSON_AddBoolToObject(root, "private_locked", entry->private_locked);
    cJSON_AddStringToObject(root, "lifecycle_state", entry->lifecycle_state == CIRCE_LIFECYCLE_ACTIVE ? "active" : "deleted");
    cJSON_AddNumberToObject(root, "revision", entry->revision);
    cJSON_AddStringToObject(root, "source", entry->source);
    cJSON_AddObjectToObject(root, "_extensions");

    char *printed = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!printed) {
        return false;
    }
    size_t need = strlen(printed) + 1;
    if (need > out_len) {
        ESP_LOGE(TAG, "JSON too large: need %u have %u", (unsigned)need, (unsigned)out_len);
        cJSON_free(printed);
        return false;
    }
    strncpy(out, printed, out_len - 1);
    out[out_len - 1] = '\0';
    cJSON_free(printed);
    return true;
}

static void copy_str_field(cJSON *obj, const char *key, char *dest, size_t dest_len)
{
    cJSON *item = cJSON_GetObjectItem(obj, key);
    if (cJSON_IsString(item) && item->valuestring) {
        strncpy(dest, item->valuestring, dest_len - 1);
        dest[dest_len - 1] = '\0';
    }
}

static void read_string_array(cJSON *arr, char items[][32], int max_items, int *count)
{
    *count = 0;
    if (!cJSON_IsArray(arr)) {
        return;
    }
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, arr)
    {
        if (*count >= max_items) {
            break;
        }
        if (cJSON_IsString(item) && item->valuestring) {
            strncpy(items[*count], item->valuestring, 31);
            items[*count][31] = '\0';
            (*count)++;
        }
    }
}

bool circe_entry_from_json(const char *json, circe_entry_t *entry)
{
    cJSON *root = cJSON_Parse(json);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed");
        return false;
    }
    memset(entry, 0, sizeof(*entry));
    copy_str_field(root, "id", entry->id, sizeof(entry->id));
    copy_str_field(root, "created_at", entry->created_at, sizeof(entry->created_at));
    copy_str_field(root, "updated_at", entry->updated_at, sizeof(entry->updated_at));
    copy_str_field(root, "local_date", entry->local_date, sizeof(entry->local_date));
    copy_str_field(root, "timezone_at_capture", entry->timezone_at_capture, sizeof(entry->timezone_at_capture));
    copy_str_field(root, "emotion", entry->emotion, sizeof(entry->emotion));
    copy_str_field(root, "emotion_family", entry->emotion_family, sizeof(entry->emotion_family));
    copy_str_field(root, "color_hex", entry->color_hex, sizeof(entry->color_hex));
    copy_str_field(root, "summary", entry->summary, sizeof(entry->summary));
    copy_str_field(root, "schema_version", entry->schema_version, sizeof(entry->schema_version));
    copy_str_field(root, "source", entry->source, sizeof(entry->source));

    cJSON *mode = cJSON_GetObjectItem(root, "entry_mode");
    if (cJSON_IsString(mode) && mode->valuestring && strcmp(mode->valuestring, "quick") == 0) {
        entry->entry_mode = CIRCE_ENTRY_MODE_QUICK;
    } else {
        entry->entry_mode = CIRCE_ENTRY_MODE_BODY_ONLY;
    }

    cJSON *im = cJSON_GetObjectItem(root, "interaction_mode");
    if (cJSON_IsObject(im)) {
        cJSON *v = cJSON_GetObjectItem(im, "short_answer");
        entry->interaction_mode.short_answer = cJSON_IsTrue(v);
        v = cJSON_GetObjectItem(im, "icon_first");
        entry->interaction_mode.icon_first = cJSON_IsTrue(v);
        v = cJSON_GetObjectItem(im, "low_energy");
        entry->interaction_mode.low_energy = cJSON_IsTrue(v);
    }

    cJSON *intensity = cJSON_GetObjectItem(root, "intensity");
    if (cJSON_IsNumber(intensity)) {
        entry->intensity = intensity->valueint;
    }

    read_string_array(cJSON_GetObjectItem(root, "body_areas"), (char (*)[32])entry->body_areas, CIRCE_MAX_BODY_AREAS,
                      &entry->body_area_count);
    read_string_array(cJSON_GetObjectItem(root, "body_sensations"), (char (*)[32])entry->body_sensations,
                      CIRCE_MAX_BODY_SENSATIONS, &entry->body_sensation_count);
    read_string_array(cJSON_GetObjectItem(root, "context_tags"), (char (*)[32])entry->context_tags, CIRCE_MAX_CONTEXT_TAGS,
                      &entry->context_tag_count);

    cJSON *sleep = cJSON_GetObjectItem(root, "sleep");
    if (cJSON_IsNumber(sleep)) {
        entry->sleep = sleep->valueint;
        entry->has_sleep = true;
    }
    cJSON *energy = cJSON_GetObjectItem(root, "energy");
    if (cJSON_IsNumber(energy)) {
        entry->energy = energy->valueint;
        entry->has_energy = true;
    }
    cJSON *stress = cJSON_GetObjectItem(root, "stress");
    if (cJSON_IsNumber(stress)) {
        entry->stress = stress->valueint;
        entry->has_stress = true;
    }

    cJSON *training = cJSON_GetObjectItem(root, "training_ok");
    entry->training_ok = cJSON_IsTrue(training);
    cJSON *priv = cJSON_GetObjectItem(root, "private_locked");
    entry->private_locked = cJSON_IsTrue(priv);
    if (!cJSON_IsBool(priv)) {
        entry->private_locked = true;
    }

    cJSON *rev = cJSON_GetObjectItem(root, "revision");
    if (cJSON_IsNumber(rev)) {
        entry->revision = rev->valueint;
    } else {
        entry->revision = 1;
    }

    cJSON *life = cJSON_GetObjectItem(root, "lifecycle_state");
    if (cJSON_IsString(life) && life->valuestring && strcmp(life->valuestring, "deleted") == 0) {
        entry->lifecycle_state = CIRCE_LIFECYCLE_DELETED;
    } else {
        entry->lifecycle_state = CIRCE_LIFECYCLE_ACTIVE;
    }

    cJSON_Delete(root);
    return entry->id[0] != '\0';
}
