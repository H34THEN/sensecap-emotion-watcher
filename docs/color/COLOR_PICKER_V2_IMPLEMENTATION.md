# Color Picker v2 — Implementation

**Status:** Implemented in `firmware/circe/main/circe_color_picker.c`

Touch-drag color field on a dedicated **Color Field** step, separate from Emotional Tone.

**Crash fix (2026-06-25):** Removed 169-cell LVGL grid — see [COLOR_PICKER_GRID_CRASH.md](../bugs/COLOR_PICKER_GRID_CRASH.md).

**Field polish (2026-06-25):** Visible canvas gradient — see [COLOR_PICKER_FIELD_POLISH_MVP.md](COLOR_PICKER_FIELD_POLISH_MVP.md).

---

## Module

| File | Role |
|------|------|
| `circe_color_picker.h` | Public API, field dimensions, object budget |
| `circe_color_picker.c` | Lightweight touch surface, HSV math, magnifier |
| `circe_entry_modes.c` | Preset table + `apply_color_*` helpers |
| `circe_hud.c` | `circe_hud_show_color_field_layout()` |
| `circe_ui.c` | `CIRCE_FLOW_COLOR_PICKER`, `CIRCE_FLOW_COLOR_PRESETS` |

---

## Touch field (lightweight MVP)

| Property | Value |
|----------|-------|
| Surface | **260×200 px** canvas display (130×100 RGB565 buffer, 2× scale) |
| LVGL objects | **8** fixed (`CIRCE_COLOR_PICKER_LVGL_OBJS`) |
| Hue | X axis (0–360°) |
| Value | Y axis (top = bright) |
| Saturation | Fixed 85% |
| Gradient | **130×100 canvas**, filled once at create (~26 KB heap) |
| Trait label | Live `family temperature brightness` via `circe_color_intel` |

### Rendering

Single LVGL canvas + crosshair + magnifier. No object grid. Gradient generated once; no per-drag canvas rebuild.

| Cost | Value |
|------|-------|
| Objects at create | 8 |
| Canvas buffer | ~26 KB RGB565 (130×100) |
| Heap spike | Minimal |
| Drag allocations | 0 |
| Drag redraw | crosshair position, magnifier style/pos, hex text, preview color, trait label |

### Magnifier

40 px circle, created once, filled with selected color during drag, clamped inside field.

### Encoder

Rotate: hue. Press encoder: toggle brightness tuning.

---

## UI actions

| Button | Behavior |
|--------|----------|
| PRESETS | Opens preset list |
| SAVE | Applies touch hex → confirm |
| SKIP | Skips color |
| BACK | Returns to tone step |

---

## Preset colors

CYAN `#7DF9FF`, GREEN `#68D391`, RED `#F56565`, GRAY `#A0AEC0`, YELLOW `#F6E05E`, PINK `#F687B3`, PURPLE `#B794F4`, SLATE `#4A5568`

---

## Performance / safety

- No SD or worker during drag
- No LVGL object creation during drag
- Destroy on screen exit via `circe_color_picker_destroy()`

---

## Related

- [COLOR_PICKER_V2_TOUCH_FIELD.md](COLOR_PICKER_V2_TOUCH_FIELD.md)
- [PHASE-COLOR-PICKER-CRASH-FIX-REPORT.md](../PHASE-COLOR-PICKER-CRASH-FIX-REPORT.md)
