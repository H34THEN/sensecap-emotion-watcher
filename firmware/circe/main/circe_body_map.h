#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CIRCE_BODY_MAP_MAX_ROWS   5
#define CIRCE_BODY_MAP_SCAN_MAX   16
#define CIRCE_BODY_MAP_MIN_ENTRIES 2

typedef struct {
    char body_area[24];
    int count;
    int max_intensity;
    int avg_intensity;
    int score;
} circe_body_map_row_t;

typedef enum {
    CIRCE_BODY_MAP_STATE_OK = 0,
    CIRCE_BODY_MAP_STATE_EMPTY,
    CIRCE_BODY_MAP_STATE_STORAGE,
    CIRCE_BODY_MAP_STATE_ERROR,
} circe_body_map_state_t;

typedef struct {
    bool valid;
    circe_body_map_state_t state;
    int total_entries;
    int row_count;
    bool truncated;
    circe_body_map_row_t rows[CIRCE_BODY_MAP_MAX_ROWS];
    char detail_line[64];
} circe_body_map_summary_t;

bool circe_body_map_load(circe_body_map_summary_t *out);
void circe_body_map_format_row(char *buf, size_t len, const circe_body_map_row_t *row, int max_score);
void circe_body_map_format_detail(char *buf, size_t len, const circe_body_map_row_t *row);
