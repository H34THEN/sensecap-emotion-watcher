#pragma once

#include "circe_conversation_engine.h"
#include "circe_storage.h"

void circe_ui_init(void);
void circe_ui_set_engine(circe_conversation_engine_t *engine);
void circe_ui_show_step(circe_flow_step_t step);

bool circe_ui_save_draft(void);
bool circe_ui_delete_latest(void);
void circe_ui_run_rebuild_test(void);
void circe_ui_refresh_strand_from_storage(void);
void circe_ui_refresh_strand_in_context(void);
void circe_ui_apply_strand_blocks(const circe_strand_block_t *blocks, int count);
void circe_ui_apply_boot_strand(void);
