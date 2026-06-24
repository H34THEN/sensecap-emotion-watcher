#include "circe_ring_anchors.h"

#include "circe_copy.h"
#include "circe_fonts.h"
#include "circe_theme.h"

static lv_obj_t *make_anchor_label(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_CAPTION);
    return lbl;
}

void circe_ring_anchors_create(lv_obj_t *scr, circe_ring_anchors_t *ring)
{
    ring->calm = make_anchor_label(scr, circe_copy_get(CIRCE_PATTERN_RING_CALM));
    lv_obj_align(ring->calm, LV_ALIGN_LEFT_MID, 18, -24);

    ring->focus = make_anchor_label(scr, circe_copy_get(CIRCE_PATTERN_RING_FOCUS));
    lv_obj_align(ring->focus, LV_ALIGN_RIGHT_MID, -18, -24);

    ring->balance = make_anchor_label(scr, circe_copy_get(CIRCE_PATTERN_RING_BALANCE));
    lv_obj_align(ring->balance, LV_ALIGN_BOTTOM_LEFT, 36, -28);

    ring->energy = make_anchor_label(scr, circe_copy_get(CIRCE_PATTERN_RING_ENERGY));
    lv_obj_align(ring->energy, LV_ALIGN_BOTTOM_RIGHT, -36, -28);

    circe_ring_anchors_apply_theme(ring);
}

void circe_ring_anchors_apply_theme(circe_ring_anchors_t *ring)
{
    if (!ring) {
        return;
    }
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_t *labels[] = {ring->calm, ring->focus, ring->balance, ring->energy};
    for (int i = 0; i < 4; i++) {
        if (!labels[i]) {
            continue;
        }
        lv_obj_set_style_text_color(labels[i], circe_theme_color(p->accent_muted), 0);
        circe_fonts_apply_label(labels[i], CIRCE_FONT_ROLE_CAPTION);
    }
}
