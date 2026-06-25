# Color Picker v2 — Touch Field Design

**Status:** Design only — not implemented in firmware yet.

CIRCE stable milestone uses **preset color rows** (`CALM`, `FOCUSED`, …) on `CIRCE_FLOW_COLOR_OPTIONAL`. v2 adds a precise touch-drag field while keeping presets for low-energy use.

---

## Goals

- Drag finger on a circular mood field to pick exact `#RRGGBB`
- Live hex readout in terminal style
- Magnifier bubble for fine control on 412×412 round display
- Rotary knob fine-tunes hue or brightness after touch pick
- Preset fallback always available

---

## Interaction

| Input | Action |
|-------|--------|
| Touch down on field | Start pick; show crosshair + magnifier |
| Touch drag | Update color under finger; live hex |
| Touch release | Confirm candidate color (preview stays) |
| Single press (encoder) | Confirm / advance to SAVE |
| Double press | Cancel / back to preset list |
| Rotate (encoder) | Fine-tune hue **or** brightness (field toggles) |
| Long press | Save as favorite (future) / confirm save |

### Flow placement

```
Body → Intensity → Color (optional)
                      ├─ PRESETS  (current, keep)
                      └─ TOUCH FIELD  (v2)
                           └─ SAVE / BACK
```

Entry from color screen: row **CUSTOM PICK** opens touch field. Presets remain first-class.

---

## UI style (CIRCE terminal/HUD)

No phone-style wheel. No full-screen color wash.

```
CIRCE
pick a color

        ( circular field ~180px )
              +
           ·  ·  ·
         ·    ●    ·   ← crosshair
           ·  ·  ·

  ┌──────── magnifier ────────┐
  │  zoomed patch + #AABBCC  │
  └───────────────────────────┘

> hex  #7B68EE
> preview ■

PRESETS   SAVE   BACK
```

- **Circular field:** subtle ring border (theme `accent_muted`), inner gradient or computed fill
- **Crosshair:** 1px lines, theme `focus`
- **Magnifier:** 64–80 px circle offset above touch point (clamp to safe viewport)
- **Hex line:** `CIRCE_FONT_ROLE_PROMPT` (26 px), left-aligned in feed
- **Preview swatch:** small square/circle (16–24 px), not a giant block
- **Commands:** terminal rows like existing menus

---

## Data model

Extend entry JSON (already has `color_hex`):

| Field | Example | Notes |
|-------|---------|-------|
| `color_hex` | `#7B68EE` | Required |
| `color_source` | `touch_picker` / `preset` | New |
| `color_label` | `CALM` | Set when near preset match (optional) |
| `color_favorite` | `false` | Future — long-press favorite |

Preset list for fallback (v2 spec):

- CALM
- GROUNDED
- TENSE
- NUMB
- HOPEFUL
- OVERWHELMED
- CUSTOM (opens touch field)

Map existing firmware presets where names overlap; add GROUNDED/TENSE/NUMB as aliases or replacements in a later migration phase.

---

## Accessibility / low energy

- **Presets first** on color screen — one press to save common moods
- Touch field optional — never required
- Encoder-only path: select preset or open field + knob fine-tune without drag
- High Visibility theme: thicker crosshair, higher contrast hex line

---

## Technical approach (implementation phase)

### Color math

**HSV math is sufficient** — no pre-rendered bitmap gradient required for v1 implementation.

- Field maps touch polar coords → hue (angle) + saturation (radius)
- Value/brightness via encoder or inner/outer radius band
- Convert HSV → RGB → `#RRGGBB` with existing `circe_entry_validate_color_hex()`

Optional later: 128×128 RGB565 lookup texture for exotic gradients — defer unless HSV feels too artificial.

### LVGL objects

| Object | Purpose |
|--------|---------|
| `lv_obj` canvas or `lv_canvas` | Field hit area + optional 1×1 draw |
| `lv_line` / thin borders | Crosshair |
| `lv_obj` circle | Magnifier clip + zoom blit |
| `lv_label` | Hex + commands |

Touch: `LV_EVENT_PRESSED`, `LV_EVENT_PRESSING`, `LV_EVENT_RELEASED` on field only.

### Circular display geometry

- Safe drawable disc ~240 px diameter (match terminal column)
- Polar mapping center = field center
- Clamp magnifier position so it stays inside viewport (y ≥ feed offset)
- Reject touches outside field radius (don’t steal edge bezel)

### Performance / LVGL task safety

- **Do not** scan SD or rebuild index during pick
- HSV→hex per touch move is cheap (<100 µs)
- Magnifier: sample 9×9 or 11×11 patch from computed color grid — redraw magnifier label only, not full screen
- Throttle `LV_EVENT_PRESSING` updates to 30–50 ms if needed
- Heavy work (save) stays on `circe_worker`

### Memory

- Avoid full 412×412 RGB buffer (~500 KB)
- HSV grid: optional 64×64 byte hue/sat table (~4 KB) or compute inline
- Magnifier zoom: 11×11 × 3 bytes temporary stack OK; prefer static scratch in picker struct

### Worker boundary

Touch picker = **pure LVGL** until user presses SAVE → existing `circe_worker_post_save_entry`.

---

## Risks

| Risk | Mitigation |
|------|------------|
| LVGL task overload on drag | HSV inline math; throttle redraw |
| Magnifier object cost | Single small canvas, reuse each frame |
| Touch + encoder conflict | Disable encoder group during drag; re-enable on release |
| Round screen edge touches | Circular hit mask |
| Theme contrast | Test Fall Out of Time + High Visibility |
| Preset drift | Keep preset table separate from touch result |

---

## Out of scope (v2 implementation phase)

- Voice
- Camera / ML color sampling
- Cloud sync
- Strand re-enable
- Favorite NVS store (design hook only)

---

## Related

- [color-system.md](../design/color-system.md)
- [ENCODER_FIRST_NAVIGATION.md](../ui/ENCODER_FIRST_NAVIGATION.md)
