#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "circe_entry.h"

#define CIRCE_TIMELINE_MAX_ITEMS 16
#define CIRCE_PATTERN_CONTEXT_MAX  10

typedef enum {
    CIRCE_TIMELINE_CAT_TODAY = 0,
    CIRCE_TIMELINE_CAT_YESTERDAY,
    CIRCE_TIMELINE_CAT_THIS_WEEK,
    CIRCE_TIMELINE_CAT_ALL,
} circe_timeline_category_t;

typedef struct {
    char entry_id[CIRCE_MAX_ID];
    char time_label[8];
    char date_label[CIRCE_MAX_DATE];
    circe_entry_mode_t entry_mode;
    char body_area[24];
    char body_sensation[32];
    int intensity;
    char emotion_label[CIRCE_MAX_EMOTION];
    char color_hex[CIRCE_MAX_COLOR];
    char color_label[CIRCE_MAX_COLOR_LABEL];
    bool color_skipped;
    bool emotion_skipped;
    char regulation_type[CIRCE_MAX_REGULATION_TYPE];
    int regulation_duration_seconds;
    int regulation_rounds_completed;
    bool regulation_session_completed;
    bool has_regulation;
} circe_timeline_item_t;

typedef struct {
    circe_timeline_category_t category;
    circe_timeline_item_t items[CIRCE_TIMELINE_MAX_ITEMS];
    int count;
    bool truncated;
    bool index_error;
    bool time_unset_note;
    char status_msg[72];
} circe_timeline_cache_t;

const char *circe_timeline_category_title(circe_timeline_category_t category);
void circe_timeline_empty_copy(circe_timeline_category_t category, char *line1, size_t l1, char *line2, size_t l2);

bool circe_timeline_load_category(circe_timeline_category_t category, circe_timeline_cache_t *cache);
bool circe_timeline_load_pattern_context(circe_timeline_item_t *items, int max_items, int *out_count);
const circe_timeline_cache_t *circe_timeline_get_cache(void);

void circe_timeline_item_format_lines(const circe_timeline_item_t *item, char *line1, size_t l1, char *line2,
                                      size_t l2, char *line3, size_t l3, char *line4, size_t l4);
