#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

typedef enum {
    CIRCE_THEME_CLASSIC = 0,
    CIRCE_THEME_GHOST_IN_THE_CODE,
    CIRCE_THEME_TERMINAL_KITTY,
    CIRCE_THEME_EVA_01,
    CIRCE_THEME_FALL_OUT_OF_TIME,
    CIRCE_THEME_MOONLIT_OBSIDIAN,
    CIRCE_THEME_SUNRISE_RECOVERY,
    CIRCE_THEME_FOREST_SIGNAL,
    CIRCE_THEME_OCEAN_DEPTHS,
    CIRCE_THEME_HIGH_VISIBILITY,
    CIRCE_THEME_NEON_TERMINAL,
    CIRCE_THEME_COUNT
} circe_theme_id_t;

typedef struct {
    const char *id;
    const char *display_name;
    uint32_t bg;
    uint32_t surface;
    uint32_t surface_alt;
    uint32_t text;
    uint32_t muted;
    uint32_t accent_primary;
    uint32_t accent_secondary;
    uint32_t accent_muted;
    uint32_t border;
    uint32_t focus;
    uint32_t strand_void;
    uint32_t danger;
    uint8_t btn_min_h;
    uint8_t btn_radius;
} circe_theme_palette_t;

void circe_theme_init(void);
circe_theme_id_t circe_theme_get_active(void);
circe_theme_id_t circe_theme_get_committed(void);
bool circe_theme_set_active(circe_theme_id_t id);
void circe_theme_preview(circe_theme_id_t id);
void circe_theme_revert_preview(void);
bool circe_theme_commit_preview(void);
const circe_theme_palette_t *circe_theme_get_palette(void);
const circe_theme_palette_t *circe_theme_get_palette_by_id(circe_theme_id_t id);
int circe_theme_count(void);
bool circe_theme_is_high_visibility(void);

lv_color_t circe_theme_color(uint32_t hex);
void circe_theme_apply_screen(lv_obj_t *scr);
void circe_theme_style_viewport(lv_obj_t *obj);
void circe_theme_style_hud_line(lv_obj_t *obj);
void circe_theme_style_button(lv_obj_t *btn);
void circe_theme_style_button_label(lv_obj_t *lbl);
void circe_theme_style_action_label(lv_obj_t *lbl, bool primary);
void circe_theme_style_primary_button(lv_obj_t *btn);
void circe_theme_style_heading(lv_obj_t *lbl);
void circe_theme_style_hero(lv_obj_t *lbl);
void circe_theme_style_subline(lv_obj_t *lbl);
void circe_theme_style_prompt(lv_obj_t *lbl);
void circe_theme_style_status(lv_obj_t *lbl);
void circe_theme_style_card(lv_obj_t *obj);
void circe_theme_style_slider(lv_obj_t *slider);
void circe_theme_style_strand_block(lv_obj_t *block, uint32_t entry_color_hex, bool is_void);
