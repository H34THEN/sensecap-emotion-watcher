# CIRCE Theme Architecture

Local-only, switchable visual themes for LVGL on SenseCAP Watcher.

**No internet. No cloud sync. No telemetry.**

---

## Goals

1. Emotional comfort through palette choice
2. Circle-first layout unchanged across themes
3. Lightweight RAM/flash footprint
4. Encoder-readable focus states in every theme
5. Optional accessibility theme (High Visibility)

---

## Module layout (Phase 5 firmware)

```
firmware/circe/main/
  circe_theme.c/h          # Theme registry, apply, NVS load/save
  circe_theme_palettes.c/h # Static palette tables (10 themes)
  circe_ui.c               # Calls circe_theme_apply_to_screen()
```

**Not in Phase 4:** implementation — architecture only.

---

## Data model

```c
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
    CIRCE_THEME_COUNT
} circe_theme_id_t;

typedef struct {
    const char *id;           // "classic"
    const char *display_name; // "Circe Classic"
    uint32_t bg;
    uint32_t surface;
    uint32_t surface_focus;
    uint32_t text_primary;
    uint32_t text_muted;
    uint32_t accent;
    uint32_t accent_muted;
    uint32_t border;
    uint32_t strand_void;
    uint32_t danger_muted;    // delete only
    uint8_t  font_prompt;     // px
    uint8_t  font_button;
    uint8_t  btn_radius;
    uint8_t  btn_min_h;       // 48 default
} circe_theme_palette_t;
```

---

## LVGL integration

### Apply path

```
circe_theme_init()           // load NVS → active theme id
circe_theme_apply(lv_disp)   // lv_theme_default_init + custom styles
circe_ui_rebuild_styles()    // per-widget style refresh on switch
```

Use `lv_theme_t` base + `lv_obj_add_style` for:

- `circe_style_btn`
- `circe_style_btn_focused`
- `circe_style_prompt`
- `circe_style_status`
- `circe_style_strand_block`

### Performance budget

| Resource | Budget |
|----------|--------|
| Flash per theme | ~200 bytes palette + shared styles |
| RAM | ≤ 2 KB styles total (reuse, not duplicate per widget) |
| Theme switch time | < 200 ms fade |
| SD reads on switch | **0** |

---

## UI entry point

**More → Appearance**

```
Appearance
  Theme: Circe Classic ▼
  [ preview swatch row ]
  [ Apply ]
  Back
```

Or horizontal picker of 10 named chips (encoder-friendly).

Theme preview: small circle mock showing bg/surface/accent — no live full-screen flash.

---

## Persistence

See [THEME_SWITCHING_MODEL.md](THEME_SWITCHING_MODEL.md).

NVS namespace: `circe_ui`  
Key: `theme_id` (uint8)

Default on first boot: **CIRCE Classic**

---

## Relationship to entry colors

| Layer | Purpose |
|-------|---------|
| **UI theme** | Chrome — backgrounds, buttons, text |
| **Entry color_hex** | User moment color — strand blocks only |

Never conflate theme accent with "your mood color."

---

## Factory firmware coexistence

CIRCE runs from `ota_0`; factory themes in SPIFFS `storage` partition are **not used**.

CIRCE themes are self-contained in firmware binary.

---

## Testing matrix

| Theme | Boot | Home | Body list | Focus visible | Delete readable |
|-------|------|------|-----------|---------------|-----------------|
| Each of 10 | required | required | required | required | required |

Automated: screenshot compare optional; manual on Watcher required.

---

## Related

- [THEME_COLOR_PALETTES.md](THEME_COLOR_PALETTES.md)
- [THEME_SWITCHING_MODEL.md](THEME_SWITCHING_MODEL.md)
- [THEME_ACCESSIBILITY_GUIDE.md](THEME_ACCESSIBILITY_GUIDE.md)
- [../ui/CIRCE_VISUAL_LANGUAGE.md](../ui/CIRCE_VISUAL_LANGUAGE.md)
