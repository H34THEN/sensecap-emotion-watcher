#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CIRCE_SCHEMA_VERSION "1.1.0"
#define CIRCE_SOURCE         "sensecap-watcher"
#define CIRCE_EMOTION_UNKNOWN "unknown"

typedef struct {
    bool short_answer;
    bool icon_first;
    bool low_energy;
} circe_interaction_mode_t;

typedef enum {
    CIRCE_ENTRY_MODE_BODY_ONLY = 0,
    CIRCE_ENTRY_MODE_QUICK,
} circe_entry_mode_t;

typedef enum {
    CIRCE_LIFECYCLE_ACTIVE = 0,
    CIRCE_LIFECYCLE_DELETED,
} circe_lifecycle_state_t;

#define CIRCE_MAX_BODY_AREAS     4
#define CIRCE_MAX_BODY_SENSATIONS 8
#define CIRCE_MAX_CONTEXT_TAGS   8
#define CIRCE_MAX_SUMMARY        280
#define CIRCE_MAX_EMOTION        64
#define CIRCE_MAX_ID             40
#define CIRCE_MAX_DATE           16
#define CIRCE_MAX_TZ             64
#define CIRCE_MAX_COLOR          8
#define CIRCE_MAX_JSON_PATH      128

typedef struct {
    char id[CIRCE_MAX_ID];
    char created_at[32];
    char updated_at[32];
    char local_date[CIRCE_MAX_DATE];
    char timezone_at_capture[CIRCE_MAX_TZ];
    circe_entry_mode_t entry_mode;
    circe_interaction_mode_t interaction_mode;
    char emotion[CIRCE_MAX_EMOTION];
    char emotion_family[CIRCE_MAX_EMOTION];
    char color_hex[CIRCE_MAX_COLOR];
    int intensity;
    char body_areas[CIRCE_MAX_BODY_AREAS][24];
    int body_area_count;
    char body_sensations[CIRCE_MAX_BODY_SENSATIONS][32];
    int body_sensation_count;
    int sleep;
    int energy;
    int stress;
    char context_tags[CIRCE_MAX_CONTEXT_TAGS][32];
    int context_tag_count;
    char summary[CIRCE_MAX_SUMMARY];
    bool training_ok;
    bool private_locked;
    circe_lifecycle_state_t lifecycle_state;
    int revision;
    char schema_version[16];
    char source[32];
    char json_path[CIRCE_MAX_JSON_PATH];
    bool has_sleep;
    bool has_energy;
    bool has_stress;
} circe_entry_t;

void circe_entry_init_defaults(circe_entry_t *entry, circe_entry_mode_t mode);
void circe_entry_set_timestamp_now(circe_entry_t *entry);
void circe_entry_touch_updated(circe_entry_t *entry);
bool circe_entry_to_json(const circe_entry_t *entry, char *out, size_t out_len);
bool circe_entry_from_json(const char *json, circe_entry_t *entry);
void circe_entry_generate_id(circe_entry_t *entry);

const char *circe_entry_mode_str(circe_entry_mode_t mode);
