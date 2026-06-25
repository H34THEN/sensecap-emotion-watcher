#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "circe_entry.h"
#include "lvgl.h"

#define CIRCE_REGULATION_TYPE_MAX 24

typedef struct {
    char regulation_type[CIRCE_REGULATION_TYPE_MAX];
    int rounds_completed;
    int duration_seconds;
    bool session_completed;
} circe_regulation_result_t;

#define CIRCE_REG_ACT_BACK     0
#define CIRCE_REG_ACT_END      1
#define CIRCE_REG_ACT_COMPLETE 2

typedef void (*circe_regulation_action_cb_t)(int action, void *ctx);

void circe_regulation_result_clear(circe_regulation_result_t *result);
void circe_regulation_apply_to_entry(circe_entry_t *entry, const circe_regulation_result_t *result);

void circe_regulation_breathing_start(circe_regulation_result_t *result, lv_obj_t *parent,
                                      circe_regulation_action_cb_t cb, void *ctx);
void circe_regulation_body_anchor_start(circe_regulation_result_t *result, lv_obj_t *parent,
                                       circe_regulation_action_cb_t cb, void *ctx);
void circe_regulation_destroy(void);
bool circe_regulation_active(void);
void circe_regulation_poll(void);
