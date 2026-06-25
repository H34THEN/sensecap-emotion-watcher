#pragma once

/*
 * CIRCE UI TUNING TOKENS
 *
 * Primary file for manual visual tuning on the 412×412 circular Watcher display.
 * Safe to edit: positions, sizes, spacing, opacities, banner dimensions, margins.
 *
 * Do NOT edit navigation, worker, storage, or NVS code when tuning layout here.
 * Colors: palette hex values live in circe_theme.c; banner accent below is visual-only.
 *
 * See docs/ui/MANUAL_UI_EDITING_WORKFLOW.md
 */

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Display + circular safe zones (see docs/ui/SAFE_AREA_SPEC.md)              */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_DISPLAY_W           412
#define CIRCE_UI_DISPLAY_H           412
#define CIRCE_UI_CENTER_X            206
#define CIRCE_UI_CENTER_Y            206

/* Zone B — safe readable content (12 px inset from bezel) */
#define CIRCE_UI_SAFE_LEFT_X         12
#define CIRCE_UI_SAFE_TOP_Y          12
#define CIRCE_UI_SAFE_RIGHT_X        400
#define CIRCE_UI_SAFE_BOTTOM_Y       400

/* Zone C — comfort zone for primary actions */
#define CIRCE_UI_COMFORT_LEFT_X      56
#define CIRCE_UI_COMFORT_TOP_Y       56
#define CIRCE_UI_COMFORT_RIGHT_X     356
#define CIRCE_UI_COMFORT_BOTTOM_Y    356

/* -------------------------------------------------------------------------- */
/* Content column (body check-in buttons, scroll panels)                       */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_CONTENT_COL_W       240
#define CIRCE_UI_CONTENT_COL_H       240
#define CIRCE_UI_CONTENT_COL_PAD     8

/* -------------------------------------------------------------------------- */
/* HUD chrome (terminal shell viewport + action area)                          */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_HUD_VIEWPORT_W      252
#define CIRCE_UI_HUD_VIEWPORT_H      120
#define CIRCE_UI_HUD_VIEWPORT_Y      78
#define CIRCE_UI_HUD_VIEWPORT_PAD    8
#define CIRCE_UI_HUD_PRESENCE_SIZE   32
#define CIRCE_UI_HUD_TOP_H           72
#define CIRCE_UI_HUD_ACTIONS_W       412
#define CIRCE_UI_HUD_ACTIONS_H       132
#define CIRCE_UI_HUD_ACTIONS_Y_OFS   (-10)
#define CIRCE_UI_HUD_HEADING_Y       38
#define CIRCE_UI_HUD_TOP_LINE_Y      88
#define CIRCE_UI_HUD_STATUS_Y_OFS    (-2)

#define CIRCE_UI_ACTION_PRIMARY_W    240
#define CIRCE_UI_ACTION_PRIMARY_H    56
#define CIRCE_UI_ACTION_SECONDARY_W  116
#define CIRCE_UI_ACTION_SECONDARY_H  52

/* -------------------------------------------------------------------------- */
/* Terminal feed (review browser, reflection, patterns text)                   */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_TERMINAL_FEED_LINES     5
#define CIRCE_UI_TERMINAL_FEED_PANEL_W   252
#define CIRCE_UI_TERMINAL_FEED_PANEL_H   62
#define CIRCE_UI_TERMINAL_FEED_Y_OFS     58
#define CIRCE_UI_TERMINAL_ROW_H            34
#define CIRCE_UI_TERMINAL_LINE_W           244
#define CIRCE_UI_TERMINAL_LINE_X_PAD       4
#define CIRCE_UI_TERMINAL_LINE_Y_STEP      28
#define CIRCE_UI_TERMINAL_CURSOR_Y_OFS     (-2)
#define CIRCE_UI_TERMINAL_BTN_ROW_GAP      2

/* -------------------------------------------------------------------------- */
/* Home slot-wheel selector                                                    */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_HOME_ROOT_W          280
#define CIRCE_UI_HOME_ROOT_H          148
#define CIRCE_UI_HOME_LABEL_W         260
#define CIRCE_UI_HOME_CURRENT_Y       24
#define CIRCE_UI_HOME_INDEX_Y         72
#define CIRCE_UI_HOME_HINT_Y          102

/* -------------------------------------------------------------------------- */
/* Single-focus menu selector (Review, Settings, Regulate, Voice, Diagnostics) */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_SELECTOR_ROOT_W      280
#define CIRCE_UI_SELECTOR_ROOT_H      168
#define CIRCE_UI_SELECTOR_LABEL_W     260
#define CIRCE_UI_SELECTOR_TITLE_Y     0
#define CIRCE_UI_SELECTOR_CURRENT_Y   28
#define CIRCE_UI_SELECTOR_INDEX_Y     88
#define CIRCE_UI_SELECTOR_HINT_Y      118

/* Label opacities (LV_OPA_* scale) — safe to tune */
#define CIRCE_UI_SELECTOR_TITLE_OPA   70
#define CIRCE_UI_SELECTOR_INDEX_OPA   80
#define CIRCE_UI_SELECTOR_HINT_OPA    60

/* -------------------------------------------------------------------------- */
/* Centered status banner (SAVING, LOADING, DELETING, etc.)                    */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_STATUS_BANNER_W      220
#define CIRCE_UI_STATUS_BANNER_H        56
#define CIRCE_UI_STATUS_BANNER_X_OFS    0
#define CIRCE_UI_STATUS_BANNER_Y_OFS    0
#define CIRCE_UI_STATUS_BANNER_RADIUS   4
#define CIRCE_UI_STATUS_BANNER_BORDER   2
#define CIRCE_UI_STATUS_BANNER_LABEL_PAD 16
/* Visual accent — full palette in circe_theme.c */
#define CIRCE_UI_STATUS_BANNER_BG_HEX   0xFF2BD6
#define CIRCE_UI_STATUS_BANNER_TEXT_HEX 0x000000
#define CIRCE_UI_STATUS_BANNER_BORDER_HEX 0x000000

/* -------------------------------------------------------------------------- */
/* Color picker field                                                          */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_COLOR_FIELD_W        260
#define CIRCE_UI_COLOR_FIELD_H        200
#define CIRCE_UI_COLOR_CANVAS_W       130
#define CIRCE_UI_COLOR_CANVAS_H       100
#define CIRCE_UI_COLOR_MAG_SIZE        40
#define CIRCE_UI_COLOR_ROOT_PAD_W      40
#define CIRCE_UI_COLOR_HEX_Y_OFS       (CIRCE_UI_COLOR_FIELD_H + 8)
#define CIRCE_UI_COLOR_TRAIT_Y_OFS     (CIRCE_UI_COLOR_HEX_Y_OFS + 26)
#define CIRCE_UI_COLOR_PREVIEW_SIZE     24
#define CIRCE_UI_COLOR_PREVIEW_X        8
#define CIRCE_UI_COLOR_HEX_LABEL_X      36
#define CIRCE_UI_COLOR_CROSS_INSET      8

/* -------------------------------------------------------------------------- */
/* Regulation screens                                                          */
/* -------------------------------------------------------------------------- */

#define CIRCE_UI_REG_ROOT_W           260
#define CIRCE_UI_REG_BREATH_ROOT_H    200
#define CIRCE_UI_REG_STEP_ROOT_H      160
#define CIRCE_UI_REG_BILATERAL_ROOT_H 180
#define CIRCE_UI_REG_LABEL_W          240
#define CIRCE_UI_REG_STEP_LABEL_W     248

/* Breathing */
#define CIRCE_UI_REG_RING_SIZE        120
#define CIRCE_UI_REG_RING_Y           8
#define CIRCE_UI_REG_ORB_SIZE          48
#define CIRCE_UI_REG_ORB_Y            44
#define CIRCE_UI_REG_PHASE_Y          128
#define CIRCE_UI_REG_COUNT_Y          158

/* Step-based (5-4-3-2-1, body anchor, sensory) */
#define CIRCE_UI_REG_STEP_TITLE_Y     4
#define CIRCE_UI_REG_STEP_BODY_Y      36

/* Bilateral tap */
#define CIRCE_UI_REG_DOT_SIZE          36
#define CIRCE_UI_REG_DOT_Y             24
#define CIRCE_UI_REG_DOT_X_OFS         56
#define CIRCE_UI_REG_SIDE_LABEL_Y      96

/* -------------------------------------------------------------------------- */
/* Boot log helper (optional, one line summary)                                */
/* -------------------------------------------------------------------------- */

void circe_ui_tokens_log_boot(void);
