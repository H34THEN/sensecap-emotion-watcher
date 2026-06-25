#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "circe_entry.h"
#include "circe_timeline.h"

#define CIRCE_REFLECTION_MAIN_MAX  280
#define CIRCE_REFLECTION_SUB_MAX   128

typedef struct {
    char main_text[CIRCE_REFLECTION_MAIN_MAX];
    char subline[CIRCE_REFLECTION_SUB_MAX];
    bool suggest_regulate;
    bool is_regulation;
    bool is_recent_pattern;
} circe_reflection_t;

void circe_reflection_clear(circe_reflection_t *reflection);
void circe_reflection_clear_recent_context(void);
bool circe_reflection_load_recent_context(void);
bool circe_reflection_generate(const circe_entry_t *entry, circe_reflection_t *out);
