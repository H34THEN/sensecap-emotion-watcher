#pragma once

#include <stdbool.h>
#include <stddef.h>

#define CIRCE_PATTERNS_SCAN_MAX     16
#define CIRCE_PATTERNS_MIN_ENTRIES  3
#define CIRCE_PATTERNS_DETECT_MAX   5
#define CIRCE_PATTERNS_DISPLAY_MAX  3

typedef enum {
    CIRCE_PATTERN_KIND_BODY_AREA = 0,
    CIRCE_PATTERN_KIND_SENSATION,
    CIRCE_PATTERN_KIND_TONE,
    CIRCE_PATTERN_KIND_HIGH_INTENSITY,
    CIRCE_PATTERN_KIND_REGULATION,
    CIRCE_PATTERN_KIND_COLOR_FAMILY,
} circe_pattern_kind_t;

typedef struct {
    circe_pattern_kind_t kind;
    char label[32];
    int count;
    int total_entries;
    char primary[96];
    char subline[96];
    bool suggest_regulate;
} circe_pattern_summary_t;

typedef struct {
    circe_pattern_summary_t items[CIRCE_PATTERNS_DISPLAY_MAX];
    int count;
    int total_entries;
    bool scan_ok;
    bool not_enough_entries;
    bool no_patterns;
    bool storage_unavailable;
    bool index_error;
} circe_patterns_result_t;

bool circe_patterns_scan(circe_patterns_result_t *out);
const circe_patterns_result_t *circe_patterns_get_cache(void);
bool circe_patterns_any_suggest_regulate(const circe_patterns_result_t *result);
