#include "circe_hud.h"

#include "circe_copy.h"
#include "circe_fonts.h"
#include "circe_theme.h"
#include "circe_voice_state_ui.h"

#define HUD_TOP_H     72
#define HUD_ACTIONS_H 132

static bool s_reset_mode = true;

static void style_telemetry_segment(lv_obj_t *arc, uint16_t start, uint16_t end, uint32_t color_hex, lv_opa_t opa)
{
    if (!arc) {
        return;
    }
    lv_obj_set_size(arc, 400, 400);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(arc, start, end);
    lv_arc_set_angles(arc, start, end);
    lv_obj_set_style_arc_width(arc, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, circe_theme_color(color_hex), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, opa, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 0, LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);
}

static void style_arc_chrome(lv_obj_t *arc, uint16_t start, uint16_t end, uint32_t color_hex, lv_opa_t opa)
{
    if (!arc) {
        return;
    }
    lv_obj_set_size(arc, 388, 388);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
    lv_arc_set_bg_angles(arc, start, end);
    lv_arc_set_angles(arc, start, end);
    lv_obj_set_style_arc_width(arc, 1, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, circe_theme_color(color_hex), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, opa, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 0, LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(arc, 0, 0);
}

static void set_hidden(lv_obj_t *obj, bool hidden)
{
    if (!obj) {
        return;
    }
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

void circe_hud_set_reset_mode(circe_hud_t *hud, bool enabled)
{
    s_reset_mode = enabled;
    if (!hud) {
        return;
    }
    set_hidden(hud->left_arc, true);
    set_hidden(hud->right_arc, true);
    set_hidden(hud->top_line, true);
    set_hidden(hud->bottom_line, true);
    set_hidden(hud->status, enabled);
    for (int i = 0; i < 4; i++) {
        set_hidden(hud->telemetry[i], enabled);
    }
    if (enabled) {
        lv_obj_clean(hud->top_arc);
    }
}

void circe_hud_create(lv_obj_t *scr, circe_hud_t *hud)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();

    hud->safe_ring = lv_arc_create(scr);
    style_arc_chrome(hud->safe_ring, 0, 360, p->accent_muted, LV_OPA_10);

    hud->left_arc = lv_arc_create(scr);
    style_arc_chrome(hud->left_arc, 155, 225, p->accent_muted, LV_OPA_40);

    hud->right_arc = lv_arc_create(scr);
    style_arc_chrome(hud->right_arc, 315, 45, p->accent_secondary, LV_OPA_40);

    hud->top_line = lv_obj_create(scr);
    lv_obj_set_size(hud->top_line, CIRCE_HUD_VIEWPORT_W + 20, 1);
    lv_obj_align(hud->top_line, LV_ALIGN_TOP_MID, 0, 88);
    lv_obj_clear_flag(hud->top_line, LV_OBJ_FLAG_SCROLLABLE);

    hud->top_arc = lv_obj_create(scr);
    lv_obj_set_size(hud->top_arc, 412, HUD_TOP_H);
    lv_obj_align(hud->top_arc, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(hud->top_arc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hud->top_arc, 0, 0);
    lv_obj_clear_flag(hud->top_arc, LV_OBJ_FLAG_SCROLLABLE);

    hud->viewport = lv_obj_create(scr);
    lv_obj_set_size(hud->viewport, CIRCE_HUD_VIEWPORT_W, CIRCE_HUD_VIEWPORT_H);
    lv_obj_align(hud->viewport, LV_ALIGN_TOP_MID, 0, 78);
    lv_obj_set_style_pad_all(hud->viewport, 8, 0);
    lv_obj_clear_flag(hud->viewport, LV_OBJ_FLAG_SCROLLABLE);

    hud->presence = lv_obj_create(hud->viewport);
    lv_obj_set_size(hud->presence, CIRCE_PRESENCE_SIZE, CIRCE_PRESENCE_SIZE);
    lv_obj_align(hud->presence, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(hud->presence, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(hud->presence, LV_OBJ_FLAG_SCROLLABLE);

    hud->heading = lv_label_create(hud->viewport);
    lv_obj_set_width(hud->heading, CIRCE_HUD_VIEWPORT_W - 16);
    lv_label_set_long_mode(hud->heading, LV_LABEL_LONG_WRAP);
    lv_obj_align(hud->heading, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_add_flag(hud->heading, LV_OBJ_FLAG_HIDDEN);

    hud->prompt = lv_label_create(hud->viewport);
    lv_obj_set_width(hud->prompt, CIRCE_HUD_VIEWPORT_W - 16);
    lv_label_set_long_mode(hud->prompt, LV_LABEL_LONG_WRAP);
    lv_obj_align(hud->prompt, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_add_flag(hud->prompt, LV_OBJ_FLAG_HIDDEN);

    hud->response = lv_label_create(hud->viewport);
    lv_obj_set_width(hud->response, CIRCE_HUD_VIEWPORT_W - 16);
    lv_label_set_long_mode(hud->response, LV_LABEL_LONG_WRAP);
    lv_obj_align(hud->response, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_add_flag(hud->response, LV_OBJ_FLAG_HIDDEN);

    hud->bottom_line = lv_obj_create(scr);
    lv_obj_set_size(hud->bottom_line, CIRCE_HUD_VIEWPORT_W + 20, 1);
    lv_obj_align(hud->bottom_line, LV_ALIGN_CENTER, 0, 28);
    lv_obj_clear_flag(hud->bottom_line, LV_OBJ_FLAG_SCROLLABLE);

    hud->actions = lv_obj_create(scr);
    lv_obj_set_size(hud->actions, 412, HUD_ACTIONS_H);
    lv_obj_align(hud->actions, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_pad_all(hud->actions, 0, 0);
    lv_obj_set_style_bg_opa(hud->actions, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hud->actions, 0, 0);
    lv_obj_clear_flag(hud->actions, LV_OBJ_FLAG_SCROLLABLE);

    hud->status = lv_label_create(scr);
    lv_obj_set_width(hud->status, CIRCE_HUD_VIEWPORT_W);
    lv_label_set_long_mode(hud->status, LV_LABEL_LONG_WRAP);
    lv_obj_align(hud->status, LV_ALIGN_BOTTOM_MID, 0, -2);

    const uint16_t seg_starts[] = {200, 250, 290, 330};
    const uint16_t seg_ends[] = {230, 280, 320, 360};
    for (int i = 0; i < 4; i++) {
        hud->telemetry[i] = lv_arc_create(scr);
        style_telemetry_segment(hud->telemetry[i], seg_starts[i], seg_ends[i], p->accent_muted, LV_OPA_20);
    }

    circe_voice_state_ui_init(&hud->voice, hud->presence);
    circe_hud_apply_theme(hud);
    circe_hud_set_reset_mode(hud, true);
}

void circe_hud_apply_theme(circe_hud_t *hud)
{
    if (!hud) {
        return;
    }
    const circe_theme_palette_t *p = circe_theme_get_palette();
    if (hud->safe_ring) {
        style_arc_chrome(hud->safe_ring, 0, 360, p->accent_muted, s_reset_mode ? LV_OPA_10 : LV_OPA_20);
    }
    if (hud->left_arc) {
        style_arc_chrome(hud->left_arc, 155, 225, p->accent_muted, LV_OPA_40);
    }
    if (hud->right_arc) {
        style_arc_chrome(hud->right_arc, 315, 45, p->accent_secondary, LV_OPA_40);
    }
    circe_theme_style_viewport(hud->viewport);
    circe_theme_style_hud_line(hud->top_line);
    circe_theme_style_hud_line(hud->bottom_line);
    circe_theme_style_hero(hud->heading);
    circe_theme_style_hero(hud->prompt);
    circe_theme_style_subline(hud->response);
    circe_theme_style_status(hud->status);
    if (hud->top_arc) {
        lv_obj_set_style_bg_opa(hud->top_arc, LV_OPA_TRANSP, 0);
    }
    if (hud->actions) {
        lv_obj_set_style_bg_opa(hud->actions, LV_OPA_TRANSP, 0);
    }
    for (int i = 0; i < 4; i++) {
        if (hud->telemetry[i]) {
            style_telemetry_segment(hud->telemetry[i], 200 + i * 50, 230 + i * 50, p->accent_muted, LV_OPA_20);
        }
    }
    circe_voice_state_ui_apply_theme(&hud->voice);
    circe_hud_set_reset_mode(hud, s_reset_mode);
}

void circe_hud_set_telemetry(circe_hud_t *hud, int active_segment)
{
    if (!hud) {
        return;
    }
    const circe_theme_palette_t *p = circe_theme_get_palette();
    const uint16_t seg_starts[] = {200, 250, 290, 330};
    const uint16_t seg_ends[] = {230, 280, 320, 360};
    for (int i = 0; i < 4; i++) {
        if (!hud->telemetry[i]) {
            continue;
        }
        lv_opa_t opa = (i == active_segment) ? LV_OPA_80 : LV_OPA_20;
        uint32_t color = (i == active_segment) ? p->accent_primary : p->accent_muted;
        style_telemetry_segment(hud->telemetry[i], seg_starts[i], seg_ends[i], color, opa);
        lv_obj_clear_flag(hud->telemetry[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void circe_hud_show_terminal_shell(circe_hud_t *hud, const char *title, const char *status_line)
{
    if (!hud) {
        return;
    }
    circe_hud_set_reset_mode(hud, false);
    lv_obj_add_flag(hud->presence, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hud->prompt, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hud->heading, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hud->response, LV_OBJ_FLAG_HIDDEN);
    circe_hud_set_heading(hud, title ? title : "CIRCE");
    circe_hud_set_response(hud, status_line ? status_line : "");
    lv_obj_align(hud->heading, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_align(hud->response, LV_ALIGN_TOP_MID, 0, 38);
    lv_obj_set_style_text_align(hud->heading, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(hud->response, LV_TEXT_ALIGN_CENTER, 0);
    circe_theme_style_hero(hud->heading);
    circe_theme_style_status(hud->response);
    circe_hud_set_subline(hud, "");
    circe_hud_set_telemetry(hud, 0);
    if (hud->viewport) {
        lv_obj_set_style_bg_opa(hud->viewport, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hud->viewport, 0, 0);
        lv_obj_set_size(hud->viewport, CIRCE_HUD_VIEWPORT_W, CIRCE_HUD_VIEWPORT_H);
        lv_obj_align(hud->viewport, LV_ALIGN_TOP_MID, 0, 72);
    }
    if (hud->actions) {
        lv_obj_set_size(hud->actions, 280, 150);
        lv_obj_align(hud->actions, LV_ALIGN_BOTTOM_MID, 0, -36);
    }
}

void circe_hud_show_color_field_layout(circe_hud_t *hud, const char *title, const char *status_line)
{
    if (!hud) {
        return;
    }
    circe_hud_set_reset_mode(hud, false);
    lv_obj_add_flag(hud->presence, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hud->prompt, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hud->heading, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hud->response, LV_OBJ_FLAG_HIDDEN);
    circe_hud_set_heading(hud, title ? title : "color field");
    circe_hud_set_response(hud, status_line ? status_line : "");
    lv_obj_align(hud->heading, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_align(hud->response, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_set_style_text_align(hud->heading, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(hud->response, LV_TEXT_ALIGN_CENTER, 0);
    circe_theme_style_hero(hud->heading);
    circe_theme_style_status(hud->response);
    circe_hud_set_subline(hud, "");
    circe_hud_set_telemetry(hud, 0);
    if (hud->viewport) {
        lv_obj_set_style_bg_opa(hud->viewport, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(hud->viewport, 0, 0);
        lv_obj_set_size(hud->viewport, CIRCE_HUD_VIEWPORT_W, 48);
        lv_obj_align(hud->viewport, LV_ALIGN_TOP_MID, 0, 52);
    }
    if (hud->actions) {
        lv_obj_set_size(hud->actions, 300, 352);
        lv_obj_align(hud->actions, LV_ALIGN_BOTTOM_MID, 0, -6);
    }
}

void circe_hud_show_terminal_prompt(circe_hud_t *hud, const char *status_line)
{
    circe_hud_show_terminal_shell(hud, "CIRCE", status_line);
}

lv_obj_t *circe_hud_strand_layer(const circe_hud_t *hud)
{
    return hud ? hud->top_arc : NULL;
}

lv_obj_t *circe_hud_actions(const circe_hud_t *hud)
{
    return hud ? hud->actions : NULL;
}

circe_voice_state_ui_t *circe_hud_voice_state(circe_hud_t *hud)
{
    return hud ? &hud->voice : NULL;
}

void circe_hud_set_heading(circe_hud_t *hud, const char *text)
{
    if (!hud || !hud->heading) {
        return;
    }
    lv_label_set_text(hud->heading, text ? text : "");
}

void circe_hud_set_prompt(circe_hud_t *hud, const char *text)
{
    if (!hud || !hud->prompt) {
        return;
    }
    lv_label_set_text(hud->prompt, text ? text : "");
}

void circe_hud_set_response(circe_hud_t *hud, const char *text)
{
    if (!hud || !hud->response) {
        return;
    }
    lv_label_set_text(hud->response, text ? text : "");
}

void circe_hud_set_subline(circe_hud_t *hud, const char *text)
{
    if (!hud || !hud->status) {
        return;
    }
    lv_label_set_text(hud->status, text ? text : "");
    lv_obj_set_style_text_align(hud->status, LV_TEXT_ALIGN_CENTER, 0);
}

void circe_hud_show_minimal_home(circe_hud_t *hud, const char *heading, const char *subline)
{
    if (!hud) {
        return;
    }
    circe_hud_set_reset_mode(hud, true);
    lv_obj_clear_flag(hud->presence, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hud->heading, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hud->prompt, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hud->response, LV_OBJ_FLAG_HIDDEN);
    circe_hud_set_heading(hud, heading);
    circe_hud_set_response(hud, subline);
    lv_obj_set_style_text_align(hud->heading, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_align(hud->response, LV_TEXT_ALIGN_CENTER, 0);
    circe_hud_set_subline(hud, "");
    circe_voice_state_ui_set(&hud->voice, CIRCE_VOICE_UI_IDLE);
}

void circe_hud_show_companion_question(circe_hud_t *hud, const char *question, const char *subline)
{
    circe_hud_show_minimal_home(hud, question, subline);
}

void circe_hud_show_companion_home(circe_hud_t *hud, const char *heading, const char *prompt, const char *subline)
{
    (void)prompt;
    circe_hud_show_minimal_home(hud, heading, subline);
}

void circe_hud_show_companion_prompt(circe_hud_t *hud, const char *prompt, const char *response)
{
    if (!hud) {
        return;
    }
    circe_hud_set_reset_mode(hud, false);
    lv_obj_clear_flag(hud->presence, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hud->heading, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(hud->prompt, LV_OBJ_FLAG_HIDDEN);
    circe_hud_set_prompt(hud, prompt);
    circe_theme_style_prompt(hud->prompt);
    lv_obj_set_style_text_align(hud->prompt, LV_TEXT_ALIGN_CENTER, 0);
    if (response && response[0]) {
        lv_obj_clear_flag(hud->response, LV_OBJ_FLAG_HIDDEN);
        circe_hud_set_response(hud, response);
        circe_theme_style_subline(hud->response);
    } else {
        lv_obj_add_flag(hud->response, LV_OBJ_FLAG_HIDDEN);
        circe_hud_set_response(hud, "");
    }
    circe_hud_set_subline(hud, "");
}

void circe_hud_show_home(circe_hud_t *hud, const char *heading, const char *subline)
{
    circe_hud_show_minimal_home(hud, circe_copy_get(CIRCE_PATTERN_HOME_PROMPT),
                                circe_copy_get(CIRCE_PATTERN_HOME_SUBLINE));
}

void circe_hud_show_prompt(circe_hud_t *hud, const char *prompt)
{
    circe_hud_show_companion_prompt(hud, prompt, NULL);
}
