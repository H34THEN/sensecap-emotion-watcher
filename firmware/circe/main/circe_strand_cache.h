#pragma once

#include <stdbool.h>

#include "circe_storage.h"

#define CIRCE_STRAND_CACHE_MAX 8

bool circe_strand_cache_init(void);
bool circe_strand_cache_load_today(circe_strand_block_t *blocks, int max_blocks, int *out_count);
bool circe_strand_cache_append_color(const char *color_hex);
void circe_strand_cache_get_loaded(circe_strand_block_t *blocks, int max_blocks, int *out_count);
