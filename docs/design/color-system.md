# Color System

Inspired by mood blankets, weather blankets, and crochet color-tracking projects — each emotional entry contributes a **color record** to a personal timeline.

---

## Goals

1. Associate feelings with colors without prescribing meanings.
2. Support multiple entries per day (multi-segment strand).
3. Enable visualization on Watcher and future Magic Mirror displays.
4. Allow hex precision for crafters tracking yarn colors.

---

## Color picker

### Emotional tone (words)

Terminal list on `CIRCE_FLOW_EMOTION_TONE` — CALM, OVERWHELMED, etc. Writes `emotion`, `emotion_label`, `emotional_tone`. No hex on this step.

See [EMOTION_COLOR_FLOW_SPLIT.md](EMOTION_COLOR_FLOW_SPLIT.md).

### Color field (v2)

Touch-drag field on `CIRCE_FLOW_COLOR_PICKER` with preset fallback on `CIRCE_FLOW_COLOR_PRESETS`.

See [COLOR_PICKER_V2_IMPLEMENTATION.md](../color/COLOR_PICKER_V2_IMPLEMENTATION.md).

### Derived color intelligence (MVP)

At save time, valid `color_hex` values produce optional HSV-derived fields (`color_family`, `color_temperature`, `color_brightness_label`, `color_saturation_label`, etc.). See [EMOTIONAL_COLOR_INTELLIGENCE_MVP.md](../color/EMOTIONAL_COLOR_INTELLIGENCE_MVP.md). CIRCE uses these for review, reflection, and patterns — never universal emotion→color mapping.

### Legacy notes

- **Wheel or palette grid** sized for 412×412 display.
- **Favorites row** — user-defined swatches (from calibration mode).
- **Recent colors** — last N entry colors for quick reuse.
- Does not enforce emotion→color mapping; optional suggestions only.

### Emotion-color relationships (optional hints)

Stored as **user preferences**, not defaults:

```json
{
  "emotion_color_hints": {
    "overwhelmed": "#4A5568",
    "calm": "#68D391"
  }
}
```

Circe may ask: "Last time you felt similar, you chose a blue-gray — use again?" User can decline.

---

## Hex color input

- Text field optimized for encoder input (character grid or numpad-style hex).
- Validates `#RRGGBB` format.
- Live preview swatch + strand preview dot.
- Syncs bidirectionally with color picker selection.

---

## Favorite colors

- Managed in **calibration_mode**.
- Up to 12 favorites (display as horizontal scroll).
- Each favorite: `{ "label": "ocean", "hex": "#0077BE" }` — label optional.

---

## Intensity slider

- Range 1–10 (or 0–100 internally).
- Affects **strand visual weight**: opacity, thickness, or glow — not the base hue.
- Default: 5 if skipped.

---

## Mood strand visualization

### Concept

Each entry produces one **strand segment** for its calendar day:

```
Day timeline (horizontal or circular for round display):

  ●───●────●●───●
  ^   ^    ^^   ^
  AM  noon PM  evening entries
```

### Segment properties

| Property | Source |
|----------|--------|
| Color | `color_hex` |
| Weight | `intensity` |
| Tooltip | time + optional emotion/body summary |

### Multi-entry days

Crochet/blanket metaphor: multiple stitches/colors per row — each entry is one stitch.

Weather blanket metaphor: hourly color changes mapped to check-in times.

### Views

| View | Device | Phase |
|------|--------|-------|
| Today strand | Watcher | 3 |
| Week band | Watcher / Mirror | 3–4 |
| Month grid | Magic Mirror | 4 |

---

## Data record

```json
{
  "color_hex": "#7B68EE",
  "color_source": "picker",
  "intensity": 7,
  "recorded_at": "2026-06-24T14:32:00Z"
}
```

One color record per entry; strand renderer queries all entries in range.

---

## RGB LED feedback (optional)

During color picker step, set Watcher RGB LED to selected color (solid mode) via BSP — reinforces embodiment. User setting: `ambient_led_during_entry` default off.

---

## Default UI theme — Neon Terminal

Starter palette for circular HUD readability (2026-06-24):

| Role | Hex |
|------|-----|
| Background | `#000000` |
| Surface | `#050505` |
| Panel | `#0B0B0F` |
| Text | `#E8FFE8` |
| Muted | `#8FAF9A` |
| Accent (green) | `#39FF14` |
| Accent (magenta) | `#FF2BD6` |
| Border | `#243024` |

Firmware: `CIRCE_THEME_NEON_TERMINAL` — default when no theme saved in NVS. Selectable in Settings → Appearance.

---

## Related modules

- [color_picker.md](../modules/color_picker.md)
- [hex_color_input.md](../modules/hex_color_input.md)
- [intensity_slider.md](../modules/intensity_slider.md)
- [mood_strand_visualizer.md](../modules/mood_strand_visualizer.md)
