#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "circe_conversation_engine.h"
#include "circe_entry.h"
#include "circe_save.h"
#include "circe_storage.h"
#include "circe_patterns.h"
#include "circe_timeline.h"

typedef enum {
    CIRCE_WORKER_TEST_SAVE = 0,
    CIRCE_WORKER_SAVE_ENTRY,
    CIRCE_WORKER_DELETE_ENTRY,
    CIRCE_WORKER_REBUILD_INDEX,
    CIRCE_WORKER_STORAGE_PROBE,
    CIRCE_WORKER_REINIT_STORAGE,
    CIRCE_WORKER_LOAD_REVIEW,
    CIRCE_WORKER_LOAD_TIMELINE,
    CIRCE_WORKER_LOAD_ENTRY,
    CIRCE_WORKER_LOAD_PATTERNS,
    CIRCE_WORKER_HEALTH_CHECK,
    CIRCE_WORKER_STORAGE_STATUS,
    CIRCE_WORKER_DIAGNOSTICS_REFRESH,
} circe_worker_cmd_type_t;

typedef struct {
    circe_worker_cmd_type_t type;
    bool success;
    char summary[128];
    circe_save_result_t save_result;
    circe_save_report_t save_report;
    circe_save_self_test_result_t test_save;
    circe_entry_t entry;
    char entry_id[CIRCE_MAX_ID];
    char saved_color[CIRCE_MAX_COLOR];
    int rebuild_count;
    bool storage_ready;
    bool review_found;
    circe_storage_health_t health;
    circe_flow_step_t success_step;
    int success_message;
    bool show_quick_subline;
    circe_timeline_category_t timeline_category;
    bool timeline_ok;
    bool timeline_empty;
    bool timeline_index_error;
    bool timeline_truncated;
    int timeline_count;
    circe_patterns_result_t patterns;
} circe_worker_completion_t;

typedef void (*circe_worker_done_fn)(const circe_worker_completion_t *result, void *user_data);

void circe_worker_init(circe_worker_done_fn on_done, void *user_data);
bool circe_worker_is_busy(void);

bool circe_worker_post_test_save(void);
bool circe_worker_post_save_entry(const circe_entry_t *entry, bool editing_existing, circe_flow_step_t success_step,
                                  int success_message, bool show_quick_subline);
bool circe_worker_post_delete_entry(const char *id);
bool circe_worker_post_rebuild_index(void);
bool circe_worker_post_storage_probe(void);
bool circe_worker_post_reinit_storage(void);
bool circe_worker_post_load_review(void);
bool circe_worker_post_load_timeline(circe_timeline_category_t category);
bool circe_worker_post_load_entry(const char *id);
bool circe_worker_post_load_patterns(void);
bool circe_worker_post_health_check(void);
bool circe_worker_post_storage_status(void);
bool circe_worker_post_diagnostics_refresh(void);
