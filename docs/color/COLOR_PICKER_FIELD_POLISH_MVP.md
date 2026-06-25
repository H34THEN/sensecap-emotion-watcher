# Color Picker Field Polish MVP

**Phase:** Color Picker Field Polish MVP  
**Firmware:** `firmware/circe`  
**Status:** Implemented

---

## Purpose

Add a visible HSV color field to the Mood Color picker while preserving the lightweight touch-mapping architecture. Replaces the prior flat single-color panel with a memory-safe canvas gradient.

---

## Rendering strategy

**Option A — low-resolution LVGL canvas gradient**

| Parameter | Value |
|-----------|--------|
| Canvas buffer | 130×100 pixels |
| Display size | 260×200 px (2× scale, antialiased) |
| Format | RGB565 (`LV_IMG_CF_TRUE_COLOR`, depth 16) |
| Saturation | Fixed 85% (`CIRCE_COLOR_PICKER_SAT`) |
| X axis | Hue 0–359 |
| Y axis | Value/brightness 100→5 (top bright, bottom dim) |

Gradient is filled **once** at picker create. No per-drag canvas regeneration.

---

## Memory cost

| Item | Size |
|------|------|
| Canvas buffer | 130×100×2 = **26,000 bytes** (~25 KB) |
| Allocation | `circe_buf_alloc()` heap, freed on destroy |

No full-screen buffer. No ARGB8888.

---

## LVGL object budget

| Object | Role |
|--------|------|
| root | Container |
| canvas (field) | Gradient + touch target |
| cross_h / cross_v | Selector lines |
| magnifier | Selected-color orb |
| preview | Small swatch |
| hex_label | `#RRGGBB` + near-preset hint |
| trait_label | Live `family temperature brightness` |

**Total: 8 objects** (target under 15)

---

## Forbidden patterns (guardrail)

Do **not** use:

- 13×13 LVGL object grid (169 cells — prior crash)
- Per-frame canvas rebuild during drag
- Worker/SD calls during drag
- Save during drag

See [COLOR_PICKER_GRID_CRASH.md](../bugs/COLOR_PICKER_GRID_CRASH.md).

---

## Touch mapping

Unchanged from v2:

- Drag on field → update hue (X) and value (Y)
- Hex, preview, trait label refresh live
- Magnifier follows finger; hidden on release
- Crosshair tracks selection
- Encoder still fine-tunes hue or value

---

## Magnifier

- 40 px circle, created once
- Filled with selected RGB
- 2 px theme focus border (terminal magenta/green accent)
- Clamped inside field

---

## Trait preview

Live label via `circe_color_intel_from_hex()`:

```text
purple cool bright
```

Compact — no emotional meaning assigned.

---

## Save integration

Unchanged: traits derived in `circe_entry_prepare_for_save()`. Preset fallback unchanged.

---

## Known limitations

- Gradient is coarse at 130×100 (acceptable for round display)
- No true pixel zoom in magnifier (orb shows selected color only)
- REVIEW → TODAY browse display bug unchanged
