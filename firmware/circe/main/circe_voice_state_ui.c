#include "circe_voice_state_ui.h"

#include "circe_theme.h"

/*
 * Visual-only voice states — presence orb reflects aliveness.
 * No microphone, STT, or TTS.
 */
void circe_voice_state_ui_init(circe_voice_state_ui_t *ui, lv_obj_t *presence_orb)
{
    ui->orb = presence_orb;
    ui->state = CIRCE_VOICE_UI_IDLE;
    circe_voice_state_ui_apply_theme(ui);
    circe_voice_state_ui_set(ui, CIRCE_VOICE_UI_IDLE);
}

void circe_voice_state_ui_apply_theme(circe_voice_state_ui_t *ui)
{
    if (!ui || !ui->orb) {
        return;
    }
    circe_voice_state_ui_set(ui, ui->state);
}

const char *circe_voice_state_ui_label(circe_voice_ui_state_t state)
{
    switch (state) {
    case CIRCE_VOICE_UI_LISTENING:
        return "Listening";
    case CIRCE_VOICE_UI_THINKING:
        return "Thinking";
    case CIRCE_VOICE_UI_SPEAKING:
        return "Speaking";
    case CIRCE_VOICE_UI_MUTED:
        return "Muted";
    case CIRCE_VOICE_UI_IDLE:
    default:
        return "";
    }
}

void circe_voice_state_ui_set(circe_voice_state_ui_t *ui, circe_voice_ui_state_t state)
{
    if (!ui || !ui->orb) {
        return;
    }
    ui->state = state;
    const circe_theme_palette_t *p = circe_theme_get_palette();
    uint32_t fill = p->surface_alt;
    uint32_t border = p->accent_muted;
    lv_opa_t fill_opa = LV_OPA_COVER;
    lv_opa_t border_opa = LV_OPA_50;
    int border_w = 2;

    switch (state) {
    case CIRCE_VOICE_UI_LISTENING:
        fill = p->accent_primary;
        border = p->focus;
        fill_opa = LV_OPA_30;
        border_opa = LV_OPA_90;
        border_w = 3;
        break;
    case CIRCE_VOICE_UI_THINKING:
        fill = p->accent_secondary;
        border = p->accent_primary;
        fill_opa = LV_OPA_40;
        border_opa = LV_OPA_80;
        border_w = 3;
        break;
    case CIRCE_VOICE_UI_SPEAKING:
        fill = p->accent_primary;
        border = p->accent_secondary;
        fill_opa = LV_OPA_50;
        border_opa = LV_OPA_COVER;
        border_w = 3;
        break;
    case CIRCE_VOICE_UI_MUTED:
        fill = p->surface;
        border = p->muted;
        fill_opa = LV_OPA_COVER;
        border_opa = LV_OPA_60;
        border_w = 2;
        break;
    case CIRCE_VOICE_UI_IDLE:
    default:
        fill = p->surface_alt;
        border = p->accent_primary;
        fill_opa = LV_OPA_COVER;
        border_opa = LV_OPA_60;
        border_w = 2;
        break;
    }

    lv_obj_set_style_bg_color(ui->orb, circe_theme_color(fill), 0);
    lv_obj_set_style_bg_opa(ui->orb, fill_opa, 0);
    lv_obj_set_style_border_color(ui->orb, circe_theme_color(border), 0);
    lv_obj_set_style_border_opa(ui->orb, border_opa, 0);
    lv_obj_set_style_border_width(ui->orb, border_w, 0);
    (void)circe_voice_state_ui_label(state);
}
