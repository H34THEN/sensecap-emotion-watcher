# Emotional Color Intelligence MVP

**Phase:** Emotional Color Intelligence MVP  
**Firmware:** `firmware/circe`  
**Status:** Implemented

---

## Purpose

Derive local, private, non-diagnostic color metadata from `color_hex` at save time. Support review, reflection, and pattern recognition without assigning universal emotional meaning to colors.

---

## Module

| File | Role |
|------|------|
| `circe_color_intel.c/h` | Hex parse, RGB→HSV, family/temperature/brightness/saturation labels |

### Public helpers

- `circe_color_parse_hex`
- `circe_color_rgb_to_hsv`
- `circe_color_family_from_hue(h, s)`
- `circe_color_temperature_from_hue(h, s)`
- `circe_color_brightness_label(v)`
- `circe_color_saturation_label(s)`
- `circe_color_intel_apply_to_entry` — called from save prep
- `circe_color_intel_from_timeline_item` — pattern/review fallback from hex

---

## Derived JSON fields (optional)

Written when `color_hex` is valid and not skipped:

| Field | Example |
|-------|---------|
| `color_hue` | `258` |
| `color_saturation` | `0.70` |
| `color_value` | `1.00` |
| `color_family` | `purple` |
| `color_temperature` | `cool` |
| `color_brightness_label` | `bright` |
| `color_saturation_label` | `vivid` |

Old entries without these fields remain valid. Load path computes traits from hex when missing.

---

## Thresholds

### Gray / low saturation

- `color_saturation < 0.15` → family `gray`, temperature `neutral`

### Hue families (when not gray)

| Hue range | Family |
|-----------|--------|
| 0–15, 345–360 | red |
| 15–45 | orange |
| 45–70 | yellow |
| 70–160 | green |
| 160–200 | cyan |
| 200–260 | blue |
| 260–290 | purple |
| 290–345 | pink |

### Temperature

- **warm:** red, orange, yellow, pink
- **cool:** green, cyan, blue, purple
- **neutral:** gray / low saturation

### Brightness (`color_value`)

| Value | Label |
|-------|-------|
| < 0.25 | dark |
| < 0.50 | dim |
| < 0.75 | balanced |
| else | bright |

### Saturation label

| Saturation | Label |
|------------|-------|
| < 0.25 | muted |
| < 0.65 | soft |
| else | vivid |

---

## Save integration

`circe_entry_prepare_for_save()` in `circe_save.c`:

- Skipped color → clear intel fields
- Valid hex → normalize then `circe_color_intel_apply_to_entry()`

No SD reads. Simple math only.

---

## Review display

Review/detail color line (when traits available):

```text
color CUSTOM #8A4DFF / purple / cool / bright
```

Timeline browse line4 (compact):

```text
COLOR purple cool bright
```

Fallback without derived fields: existing hex-only display.

---

## Reflection integration

Priority remains below intensity, body patterns, and tone. Color observations use trait language only:

- `This color is cool and bright.`
- `I saved this as a vivid purple.`
- `Your recent colors have stayed mostly muted.`

Subline: `Only you decide what it means.`

---

## Pattern integration

Pattern scan uses derived traits (or computes from hex via lightweight timeline loader):

- temperature cool/warm (≥3 entries)
- saturation muted (≥3)
- brightness dark/bright (≥3)

Does not parse full entry structs on worker stack.

---

## Language guardrails

**Use:** This color is… / I saved this as… / Your recent colors… / Only you decide what it means.

**Avoid:** Blue means sadness / Red means anger / This proves… / diagnosis language.

---

## Known limitations

- No color-distance-from-prior-entry in this MVP
- No color picker gradient UI changes
- REVIEW → TODAY browse display bug unchanged
