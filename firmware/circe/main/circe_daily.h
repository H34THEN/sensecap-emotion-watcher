#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool valid;
    bool time_set;
    int local_hour;
    int entries_today;
    int regulation_today;
    int high_intensity_today;
    char repeated_body_area[24];
    int repeated_body_count;
    char primary_line[64];
    char subline[64];
} circe_daily_summary_t;

typedef enum {
    CIRCE_DAILY_PERIOD_NEUTRAL = 0,
    CIRCE_DAILY_PERIOD_MORNING,
    CIRCE_DAILY_PERIOD_AFTERNOON,
    CIRCE_DAILY_PERIOD_EVENING,
    CIRCE_DAILY_PERIOD_NIGHT,
} circe_daily_period_t;

circe_daily_period_t circe_daily_period_from_hour(int hour);
void circe_daily_build_copy(circe_daily_summary_t *summary, bool index_error, bool storage_unavailable);
bool circe_daily_load(circe_daily_summary_t *out);
