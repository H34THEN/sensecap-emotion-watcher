#include "circe_selector.h"

#include <stdio.h>

#include "circe_copy.h"
#include "circe_fonts.h"
#include "circe_theme.h"
#include "circe_ui_tokens.h"

static int wrap_index(int idx, int count)
{
    if (count <= 0) {
        return 0;
    }
    while (idx < 0) {
        idx += count;
    }
    while (idx >= count) {
        idx -= count;
    }
    return idx;
}

static void style_label(lv_obj_t *lbl, circe_font_role_t role, lv_opa_t opa, uint32_t color_hex)
{
    circe_fonts_apply_label(lbl, role);
    lv_obj_set_style_text_color(lbl, circe_theme_color(color_hex), 0);
    lv_obj_set_style_text_opa(lbl, opa, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
}

void circe_selector_create(circe_selector_t *sel, lv_obj_t *parent, const char *title,
                           const circe_selector_item_t *items, int count, int initial_index)
{
    if (!sel || !parent || !items || count <= 0) {
        return;
    }
    if (count > CIRCE_SELECTOR_MAX_ITEMS) {
        count = CIRCE_SELECTOR_MAX_ITEMS;
    }

    circe_selector_destroy(sel);
    sel->items = items;
    sel->count = count;
    sel->selected = wrap_index(initial_index, count);
    sel->active = true;
    circe_encoder_state_reset(&sel->enc);

    const circe_theme_palette_t *p = circe_theme_get_palette();

    sel->root = lv_obj_create(parent);
    lv_obj_set_size(sel->root, CIRCE_UI_SELECTOR_ROOT_W, CIRCE_UI_SELECTOR_ROOT_H);
    lv_obj_align(sel->root, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(sel->root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sel->root, 0, 0);
    lv_obj_clear_flag(sel->root, LV_OBJ_FLAG_SCROLLABLE);

    sel->title_lbl = lv_label_create(sel->root);
    lv_obj_set_width(sel->title_lbl, CIRCE_UI_SELECTOR_LABEL_W);
    lv_obj_align(sel->title_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_SELECTOR_TITLE_Y);
    style_label(sel->title_lbl, CIRCE_FONT_ROLE_CAPTION, CIRCE_UI_SELECTOR_TITLE_OPA, p->muted);
    lv_label_set_text(sel->title_lbl, title ? title : "");

    sel->current_lbl = lv_label_create(sel->root);
    lv_obj_set_width(sel->current_lbl, CIRCE_UI_SELECTOR_LABEL_W);
    lv_obj_align(sel->current_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_SELECTOR_CURRENT_Y);
    style_label(sel->current_lbl, CIRCE_FONT_ROLE_HERO, LV_OPA_COVER, p->text);

    sel->index_lbl = lv_label_create(sel->root);
    lv_obj_set_width(sel->index_lbl, CIRCE_UI_SELECTOR_LABEL_W);
    lv_obj_align(sel->index_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_SELECTOR_INDEX_Y);
    style_label(sel->index_lbl, CIRCE_FONT_ROLE_CAPTION, CIRCE_UI_SELECTOR_INDEX_OPA, p->accent_primary);

    sel->hint_lbl = lv_label_create(sel->root);
    lv_obj_set_width(sel->hint_lbl, CIRCE_UI_SELECTOR_LABEL_W);
    lv_obj_align(sel->hint_lbl, LV_ALIGN_TOP_MID, 0, CIRCE_UI_SELECTOR_HINT_Y);
    style_label(sel->hint_lbl, CIRCE_FONT_ROLE_CAPTION, CIRCE_UI_SELECTOR_HINT_OPA, p->muted);
    lv_label_set_text(sel->hint_lbl, circe_copy_get(CIRCE_PATTERN_NAV_ROTATE_CHOOSE));

    circe_selector_refresh(sel);
}

void circe_selector_destroy(circe_selector_t *sel)
{
    if (!sel) {
        return;
    }
    if (sel->root) {
        lv_obj_del(sel->root);
    }
    sel->root = NULL;
    sel->title_lbl = NULL;
    sel->current_lbl = NULL;
    sel->index_lbl = NULL;
    sel->hint_lbl = NULL;
    sel->items = NULL;
    sel->count = 0;
    sel->selected = 0;
    sel->active = false;
    circe_encoder_state_reset(&sel->enc);
}

void circe_selector_refresh(circe_selector_t *sel)
{
    if (!sel || !sel->active || sel->count <= 0 || !sel->items) {
        return;
    }
    if (sel->current_lbl) {
        lv_label_set_text(sel->current_lbl, sel->items[sel->selected].label ? sel->items[sel->selected].label : "");
    }
    if (sel->index_lbl) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%d / %d", sel->selected + 1, sel->count);
        lv_label_set_text(sel->index_lbl, buf);
    }
    if (sel->hint_lbl) {
        char hint[96];
        snprintf(hint, sizeof(hint), "%s · %s · %s", circe_copy_get(CIRCE_PATTERN_NAV_ROTATE_CHOOSE),
                 circe_copy_get(CIRCE_PATTERN_NAV_PRESS_SELECT), circe_copy_get(CIRCE_PATTERN_NAV_TRIPLE_HOME));
        lv_label_set_text(sel->hint_lbl, hint);
    }
}

int circe_selector_poll(circe_selector_t *sel)
{
    if (!sel || !sel->active || sel->count <= 0) {
        return CIRCE_SELECTOR_ACTION_NONE;
    }

    int diff = circe_encoder_read_diff();
    if (diff != 0) {
        sel->selected = wrap_index(sel->selected + diff, sel->count);
        circe_selector_refresh(sel);
    }

    bool pressed = circe_encoder_read_pressed();
    return circe_encoder_poll(&sel->enc, diff, pressed);
}

const char *circe_selector_selected_action(const circe_selector_t *sel)
{
    if (!sel || !sel->active || sel->count <= 0 || !sel->items) {
        return NULL;
    }
    return sel->items[sel->selected].action_id;
}

const char *circe_selector_selected_label(const circe_selector_t *sel)
{
    if (!sel || !sel->active || sel->count <= 0 || !sel->items) {
        return NULL;
    }
    return sel->items[sel->selected].label;
}
