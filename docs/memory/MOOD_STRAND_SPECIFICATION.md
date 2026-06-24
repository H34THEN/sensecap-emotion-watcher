# Mood Strand Specification

Formal spec for mood strands — daily rows of color blocks supporting multiple entries per day, missing days, and Magic Mirror rendering.

Metaphor: **crochet mood blanket** / **weather blanket** — one row per day, one stitch (block) per entry.

Related: [color-system.md](../design/color-system.md), [MAGIC_MIRROR_VISUALIZATION_PLAN.md](MAGIC_MIRROR_VISUALIZATION_PLAN.md)

---

## Core concepts

| Term | Definition |
|------|------------|
| **Strand** | Ordered sequence of color blocks for one calendar day |
| **Block** | One entry's visual contribution |
| **Row** | One day's strand in a multi-day blanket |
| **Blanket** | Collection of rows (week/month/year) |

---

## Block data model

Each block derives from one entry:

```json
{
  "entry_id": "550e8400-e29b-41d4-a716-446655440000",
  "at": "2026-06-24T14:32:00Z",
  "local_time": "10:32",
  "color_hex": "#7B68EE",
  "intensity": 7,
  "width_units": 1,
  "opacity": 0.85,
  "stroke_weight": 3,
  "private_locked": true,
  "tooltip": {
    "emotion": null,
    "body_summary": "chest, tight",
    "entry_mode": "body_only"
  }
}
```

### Derived visual properties

| Property | Formula | Range |
|----------|---------|-------|
| `opacity` | `0.4 + (intensity / 10) * 0.6` | 0.4–1.0 |
| `stroke_weight` | `1 + round(intensity / 2)` | 1–6 px at base scale |
| `width_units` | Default `1`; optional `entry_mode=full` → 1.2 | Relative |

**Hue comes only from `color_hex`.** No semantic color mapping.

---

## Daily row

```json
{
  "strand_version": "1.0.0",
  "date": "2026-06-24",
  "timezone": "America/New_York",
  "blocks": [ /* ordered by `at` ascending */ ],
  "block_count": 3,
  "missing": false,
  "placeholder": null
}
```

### Ordering rules

1. Sort blocks by `at` (UTC) ascending
2. Tie-break by `entry_id` lexicographic
3. Maximum blocks per day: **none** (practical UI cap 48 for render performance)

### Multiple entries per day

Each entry → **one block**. Three entries → three stitches in the row:

```
[ block₁ ][ block₂ ][ block₃ ]
  8:00      14:00     21:00
```

Spacing between blocks: **gap_units: 0.15** (15% of block width) — visual separation without implying missing time.

---

## Spacing rules

### Intra-day (between blocks)

| Rule | Value |
|------|-------|
| Minimum gap | 2 px at Watcher scale |
| Gap scales with row width | proportional `gap_units` |
| Same-color adjacent blocks | still separate blocks (no merge) unless user setting `merge_adjacent_identical: true` |

### Inter-day (between rows in blanket)

| Rule | Value |
|------|-------|
| Row height | fixed `row_height_units: 1` |
| Row gap | `0.25` row heights |
| Week separator | optional subtle line Sundays |

---

## Missing day rules

A day is **missing** when `block_count == 0` for local calendar date.

### Missing day representation

```json
{
  "date": "2026-06-25",
  "blocks": [],
  "block_count": 0,
  "missing": true,
  "placeholder": {
    "style": "void | gray | dotted",
    "color_hex": null
  }
}
```

| `placeholder.style` | Visual | When |
|----------------------|--------|------|
| `void` | Empty row / transparent | Default — no shame |
| `gray` | `#E0E0E0` single stitch | User setting "show gray for quiet days" |
| `dotted` | Dotted outline | Craft metaphor mode |

**Circe never says:** "You missed a day."

**Circe may say:** "Quiet day on the blanket — that's allowed."

### Partial day

Not missing if ≥1 entry. No special case.

---

## Default color when skipped

Entry with `color_source: default` → `#808080` block still renders (user chose skip). Distinct from missing day.

---

## Privacy and sync

| Context | private_locked blocks |
|---------|----------------------|
| Watcher personal strand | Shown |
| Magic Mirror default | **Hidden** — row shows gap or anonymized count only |
| Mirror opt-in "show aggregates" | Color only, no tooltip |
| Export/GPU | Excluded unless eligible |

Mirror row for day with only private entries:

```json
{
  "date": "2026-06-24",
  "blocks_public": [],
  "private_block_count": 3,
  "display": "private_day_indicator"
}
```

---

## Watcher rendering (412×412)

### Today view

- Circular ring or horizontal bar
- Blocks as arc segments proportional to time-of-day (optional) or equal width
- Tap block → open entry review (if not deleted)

### Week view

- 7 rows stacked
- Scroll vertically

---

## Magic Mirror rendering expectations

### Mood blanket module

| Element | Spec |
|---------|------|
| Canvas | Configurable width; row = one day |
| Block width | `(canvas_width - gaps) / max_blocks_that_day_in_week` capped |
| Tooltip | time + optional emotion/body (non-private only) |
| Animation | None default (autistic-friendly) |
| Refresh | Poll LAN API every N minutes |

### API payload (draft)

```
GET /api/strand?from=2026-06-01&to=2026-06-30&privacy=public_only
→ { "rows": [ DailyRow, ... ] }
```

See [MAGIC_MIRROR_VISUALIZATION_PLAN.md](MAGIC_MIRROR_VISUALIZATION_PLAN.md).

---

## Strand index (SQLite)

Table `strand_blocks`:

| Column | Type |
|--------|------|
| entry_id | TEXT PK |
| date_bucket | TEXT |
| at | TEXT |
| color_hex | TEXT |
| intensity | INT |
| sort_order | INT |

Rebuild from entries on index corruption.

---

## Versioning

`strand_version` independent of `schema_version`. Visual formula changes bump strand minor version; renderers declare supported version.

---

## Examples

### Three-entry day

| Time | Color | Intensity |
|------|-------|-----------|
| 08:00 | #68D391 | 3 |
| 14:00 | #4A5568 | 8 |
| 22:00 | #9B59B6 | 5 |

Row renders three blocks left-to-right (or chronological on ring).

### Missing day between active days

```
Jun 23: [██][██]
Jun 24: (void)
Jun 25: [██]
```

No interpolation between colors across missing days.
