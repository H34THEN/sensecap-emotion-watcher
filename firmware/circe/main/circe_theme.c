#include "circe_theme.h"

#include <string.h>

#include "circe_fonts.h"
#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "circe_theme";

#define NVS_NS   "circe_ui"
#define NVS_KEY  "theme_id"

static circe_theme_id_t s_active = CIRCE_THEME_NEON_TERMINAL;
static circe_theme_id_t s_committed = CIRCE_THEME_NEON_TERMINAL;

/*
 * Phase 6A rebuilt palettes — accent_primary/secondary/muted, border, focus.
 * surface_alt: subtle lift on focus (never bright fill on High Visibility).
 */
static const circe_theme_palette_t s_palettes[CIRCE_THEME_COUNT] = {
    /* Classic — deep slate, lavender, warm ivory */
    {"classic", "Circe Classic",
     0x1A202C, 0x3D4A5C, 0x4A5568, 0xFFF5F0, 0xA0AEC0,
     0xB794F4, 0xD6BCFA, 0x6B46C1, 0x5A6578, 0xB794F4,
     0x141820, 0xC05656, 52, 14},
    /* Ghost in the Code — UI reset dark benchmark */
    {"ghost", "Ghost in the Code",
     0x05080D, 0x0B0F14, 0x111827, 0xE6EDF3, 0x9FB3C8,
     0x7DF9FF, 0x00E5FF, 0x1E5A66, 0x1E5A66, 0x7DF9FF,
     0x05080D, 0x8B4040, 56, 12},
    /* Terminal Kitty */
    {"kitty", "Terminal Kitty",
     0x0D0D0D, 0x1A1A1A, 0x252525, 0xECECEC, 0x888888,
     0xFF6EC7, 0x7DF9FF, 0xCBA6F7, 0x333333, 0xFF6EC7,
     0x141414, 0x994444, 52, 10},
    /* EVA-01 */
    {"eva01", "Eva-01",
     0x0D0518, 0x1E0F3A, 0x2A1650, 0xF0F0F0, 0x9B8FB8,
     0x4AFF4A, 0x7B5EA7, 0x5A4578, 0x3D2660, 0x4AFF4A,
     0x120820, 0xAA4444, 52, 10},
    /* Fall Out of Time */
    {"fallout", "Fall Out of Time",
     0x0A0F0A, 0x142014, 0x1A2A1A, 0x33FF33, 0x228822,
     0xFFB000, 0x33FF33, 0x1A501A, 0x1A301A, 0xFFB000,
     0x081008, 0xAA5500, 52, 8},
    /* Moonlit Obsidian */
    {"moonlit", "Moonlit Obsidian",
     0x0B0E14, 0x151A24, 0x1E2433, 0xF4F6FA, 0x7A8499,
     0xC0C8D8, 0x8B9DC3, 0x5C6B8A, 0x2A3344, 0xC0C8D8,
     0x080A10, 0x884444, 52, 12},
    /* Sunrise Recovery */
    {"sunrise", "Sunrise Recovery",
     0x3D2E28, 0x5C4A42, 0x6E5A50, 0xFFF5EE, 0xD4B8A8,
     0xF6AD55, 0xFBD38D, 0xC05621, 0x6E5A50, 0xF6AD55,
     0x2A201C, 0xB56565, 52, 12},
    /* Forest Signal */
    {"forest", "Forest Signal",
     0x1A2F1A, 0x2D4A2D, 0x3A5C3A, 0xE8F0E8, 0x8FA88F,
     0x4FD1C5, 0x68D391, 0x2F855A, 0x3A5C3A, 0x4FD1C5,
     0x122012, 0x996655, 52, 12},
    /* Ocean Depths */
    {"ocean", "Ocean Depths",
     0x0A1628, 0x1A3050, 0x243D60, 0xE6F0FA, 0x7A9BB8,
     0x4ECDC4, 0x63B3ED, 0x2C5282, 0x243D60, 0x4ECDC4,
     0x081020, 0x886666, 52, 12},
    /* High Visibility — yellow focus ring only */
    {"hivis", "High Visibility",
     0x000000, 0x1A1A1A, 0x2A2A2A, 0xFFFFFF, 0xCCCCCC,
     0xFFFF00, 0xFFFF00, 0x888800, 0x444444, 0xFFFF00,
     0x333333, 0xFF6666, 56, 8},
    /* Neon Terminal — black / magenta / terminal green starter */
    {"neon", "Neon Terminal",
     0x000000, 0x050505, 0x0B0B0F, 0xE8FFE8, 0x8FAF9A,
     0x39FF14, 0xFF2BD6, 0x243024, 0x243024, 0x39FF14,
     0x000000, 0xFF3B3B, 52, 10},
};

static void style_focus_ring(lv_obj_t *obj)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_border_width(obj, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(obj, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(obj, 0, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(obj, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_STATE_FOCUSED);
    (void)p;
}

lv_color_t circe_theme_color(uint32_t hex)
{
    return lv_color_hex(hex);
}

const circe_theme_palette_t *circe_theme_get_palette(void)
{
    return &s_palettes[s_active];
}

const circe_theme_palette_t *circe_theme_get_palette_by_id(circe_theme_id_t id)
{
    if (id >= CIRCE_THEME_COUNT) {
        return &s_palettes[CIRCE_THEME_CLASSIC];
    }
    return &s_palettes[id];
}

circe_theme_id_t circe_theme_get_active(void)
{
    return s_active;
}

int circe_theme_count(void)
{
    return CIRCE_THEME_COUNT;
}

bool circe_theme_is_high_visibility(void)
{
    return s_active == CIRCE_THEME_HIGH_VISIBILITY;
}

static esp_err_t save_theme_id(circe_theme_id_t id)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "theme save nvs_open(%s) failed: %s", NVS_NS, esp_err_to_name(err));
        return err;
    }
    err = nvs_set_u8(h, NVS_KEY, (uint8_t)id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "theme save nvs_set_u8(%s=%u) failed: %s", NVS_KEY, (unsigned)id, esp_err_to_name(err));
        nvs_close(h);
        return err;
    }
    err = nvs_commit(h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "theme save nvs_commit failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "theme saved to NVS: %s (%u)", s_palettes[id].display_name, (unsigned)id);
    }
    nvs_close(h);
    return err;
}

static circe_theme_id_t load_theme_id(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "theme load nvs_open(%s) failed: %s", NVS_NS, esp_err_to_name(err));
        }
        return CIRCE_THEME_NEON_TERMINAL;
    }
    uint8_t id = (uint8_t)CIRCE_THEME_NEON_TERMINAL;
    err = nvs_get_u8(h, NVS_KEY, &id);
    nvs_close(h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "theme load: no saved theme, using default");
        return CIRCE_THEME_NEON_TERMINAL;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "theme load nvs_get_u8(%s) failed: %s", NVS_KEY, esp_err_to_name(err));
        return CIRCE_THEME_NEON_TERMINAL;
    }
    if (id >= CIRCE_THEME_COUNT) {
        ESP_LOGW(TAG, "theme load: invalid id %u, using default", (unsigned)id);
        return CIRCE_THEME_NEON_TERMINAL;
    }
    return (circe_theme_id_t)id;
}

circe_theme_id_t circe_theme_get_committed(void)
{
    return s_committed;
}

void circe_theme_init(void)
{
    s_committed = load_theme_id();
    s_active = s_committed;
    ESP_LOGI(TAG, "theme active: %s", s_palettes[s_active].display_name);
}

bool circe_theme_set_active(circe_theme_id_t id)
{
    if (id >= CIRCE_THEME_COUNT) {
        return false;
    }
    s_active = id;
    s_committed = id;
    esp_err_t save_err = save_theme_id(id);
    if (save_err != ESP_OK) {
        ESP_LOGW(TAG, "theme active in RAM only (NVS save failed: %s)", esp_err_to_name(save_err));
    }
    ESP_LOGI(TAG, "theme set: %s", s_palettes[s_active].display_name);
    return true;
}

void circe_theme_preview(circe_theme_id_t id)
{
    if (id >= CIRCE_THEME_COUNT) {
        return;
    }
    s_active = id;
}

void circe_theme_revert_preview(void)
{
    s_active = s_committed;
}

bool circe_theme_commit_preview(void)
{
    if (s_active == s_committed) {
        return true;
    }
    return circe_theme_set_active(s_active);
}

void circe_theme_apply_screen(lv_obj_t *scr)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_bg_color(scr, circe_theme_color(p->bg), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(scr, 0, 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
}

void circe_theme_style_viewport(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
}

void circe_theme_style_primary_button(lv_obj_t *btn)
{
    circe_theme_style_button(btn);
}

void circe_theme_style_action_label(lv_obj_t *lbl, bool primary)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->text), 0);
    circe_fonts_apply_label(lbl, primary ? CIRCE_FONT_ROLE_PROMPT : CIRCE_FONT_ROLE_PROMPT);
    if (circe_theme_is_high_visibility()) {
        lv_obj_set_style_text_font(lbl, circe_fonts_get(CIRCE_FONT_ROLE_PROMPT), 0);
    }
}

void circe_theme_style_hud_line(lv_obj_t *obj)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_bg_color(obj, circe_theme_color(p->accent_muted), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_40, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
}

void circe_theme_style_button(lv_obj_t *btn)
{
    lv_obj_set_height(btn, 30);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    style_focus_ring(btn);
}

void circe_theme_style_button_label(lv_obj_t *lbl)
{
    circe_theme_style_action_label(lbl, false);
}

void circe_theme_style_subline(lv_obj_t *lbl)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->muted), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_CAPTION);
}

void circe_theme_style_heading(lv_obj_t *lbl)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->text), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_HEADING);
    if (circe_theme_is_high_visibility()) {
        lv_obj_set_style_text_font(lbl, circe_fonts_get(CIRCE_FONT_ROLE_HERO), 0);
    }
}

void circe_theme_style_hero(lv_obj_t *lbl)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->text), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(lbl, 6, 0);
    circe_fonts_apply_label(lbl, circe_theme_is_high_visibility() ? CIRCE_FONT_ROLE_HERO : CIRCE_FONT_ROLE_HERO);
}

void circe_theme_style_prompt(lv_obj_t *lbl)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->text), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_line_space(lbl, 4, 0);
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_PROMPT);
    if (circe_theme_is_high_visibility()) {
        lv_obj_set_style_text_font(lbl, circe_fonts_get(CIRCE_FONT_ROLE_PROMPT), 0);
    }
}

void circe_theme_style_status(lv_obj_t *lbl)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_text_color(lbl, circe_theme_color(p->muted), 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    circe_fonts_apply_label(lbl, CIRCE_FONT_ROLE_CAPTION);
}

void circe_theme_style_card(lv_obj_t *obj)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_text_color(obj, circe_theme_color(p->text), 0);
}

void circe_theme_style_slider(lv_obj_t *slider)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    lv_obj_set_style_bg_color(slider, circe_theme_color(p->surface), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, circe_theme_color(p->accent_primary), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, circe_theme_color(p->accent_primary), LV_PART_KNOB);
}

void circe_theme_style_strand_block(lv_obj_t *block, uint32_t entry_color_hex, bool is_void)
{
    const circe_theme_palette_t *p = circe_theme_get_palette();
    uint32_t c = is_void ? p->strand_void : entry_color_hex;
    lv_obj_set_style_bg_color(block, circe_theme_color(c), 0);
    lv_obj_set_style_border_width(block, is_void ? 1 : 0, 0);
    if (is_void) {
        lv_obj_set_style_border_color(block, circe_theme_color(p->accent_muted), 0);
        lv_obj_set_style_border_opa(block, LV_OPA_50, 0);
    }
    lv_obj_set_style_radius(block, 4, 0);
}
