# Magic Mirror Visualization Plan

Design for LAN visualization: mood blanket, color strands, pattern cards, body maps, weekly/monthly summaries.

**No implementation.** Assumes `sync_queue` pushes non-private aggregates only.

Related: [MOOD_STRAND_SPECIFICATION.md](MOOD_STRAND_SPECIFICATION.md), [integration/magic-mirror.md](../integration/magic-mirror.md)

---

## Module: MMM-Circe (future)

Node.js MagicMirror module consuming Watcher or NAS-hosted JSON API.

---

## Data sources

| Source | Refresh | Privacy |
|--------|---------|---------|
| Watcher LAN API | 5–15 min poll | public aggregates only |
| NAS mirror cache | on file change | same |
| Hades pattern API | hourly | aggregated observations |

Never fetch raw private entries by default.

---

## 1. Mood blanket

### Concept

One **row per day**, one **stitch per entry** — crochet blanket on screen.

### Layout

```
┌──────────────────────────────────────┐
│  June 2026                    row 30 │
│  [█][█][░][█][·][█][█] ...           │  ← each cell = day
│  ▔▔▔▔▔▔▔                             │
│  Selected day expanded below         │
└──────────────────────────────────────┘
```

| Symbol | Meaning |
|--------|---------|
| `█` | Day has public strand blocks (dominant color shown) |
| `░` | Multiple public colors — striped mini cell |
| `·` | Missing day (void style) |
| `🔒` | Private-only day indicator (optional icon, no details) |

### Config

```js
{
  module: "MMM-Circe",
  position: "bottom_bar",
  config: {
    circeEndpoint: "http://watcher.local:8080",
    view: "blanket",
    monthOffset: 0,
    showPrivateDays: false,
    missingDayStyle: "void",
    rowHeight: 12
  }
}
```

---

## 2. Color strands

### Daily strand bar

Horizontal bar for today — blocks per [MOOD_STRAND_SPECIFICATION.md](MOOD_STRAND_SPECIFICATION.md).

### Weekly band

7 stacked day strands; height proportional to entry count (cap visual).

### API

```
GET /api/mirror/strand?from=&to=&privacy=public_only
```

Response: `{ "rows": [ DailyRow ] }`

---

## 3. Pattern cards

Circe reflection cards — **observational text only**.

### Card model

```json
{
  "card_id": "card-uuid",
  "generated_at": "...",
  "template_key": "pattern.color_cluster",
  "params": { "colors": ["#4A5568"], "count": 5 },
  "disclaimer_key": "correlation_not_causation",
  "source": "watcher_rules | hades_gpu"
}
```

### Mirror UI

- Max 2 sentences
- Tap to expand week view
- Dismiss persists on Watcher sync back

No diagnostic styling.

---

## 4. Body sensation maps

### Aggregate heatmap (not single entry)

Frequency of body areas over rolling 7d **public entries only**:

```json
{
  "window": "rolling_7d",
  "areas": {
    "chest": 4,
    "shoulders": 3,
    "head": 1
  },
  "top_sensations": [
    { "tag": "tight", "count": 5 }
  ]
}
```

### Render

- Silhouette SVG with opacity per area count
- No single-entry detail on Mirror default
- Label: "Aggregated — not a medical image"

---

## 5. Weekly summaries

```json
{
  "week": "2026-W25",
  "entry_count_public": 12,
  "entry_count_private_hidden": 3,
  "dominant_colors": ["#4A5568", "#7B68EE"],
  "missing_days": 2,
  "pattern_cards": [ /* card ids */ ],
  "strand_preview": { "rows": [ /* 7 DailyRow */ ] }
}
```

Display: headline + mini blanket strip + optional card carousel.

Circe copy via template keys — rendered on Mirror or pre-rendered on Watcher.

---

## 6. Monthly summaries

```json
{
  "month": "2026-06",
  "active_days": 18,
  "total_public_entries": 45,
  "color_histogram": [{ "hex": "#4A5568", "count": 12 }],
  "blanket_row_ids": ["2026-06-01", "..."]
}
```

Full month blanket grid — primary "wall art" view.

---

## Sync payload tiers

| Tier | Contents | Default |
|------|----------|---------|
| 0 | Strand public blocks only | **On** |
| 1 | Weekly/monthly rollups | On |
| 2 | Pattern cards | Opt-in |
| 3 | Body heatmap aggregates | Opt-in |
| 4 | Photo thumbnails | Opt-in + consent |
| 5 | Full entry JSON | Off |

---

## Rendering rules (autism-friendly)

- No autoplay animation
- Muted palette for UI chrome; strand colors user-data only
- No sound on update
- High contrast mode config
- Private days never show "you missed logging"

---

## Offline / NAS fallback

Mirror reads from NAS copy if Watcher sleep:

```
/mnt/nas/circe/mirror-cache/
  strand/
  weekly/
  monthly/
  cards/
```

Watcher sync job writes cache; Mirror polls NAS.

---

## Delete propagation

When entry deleted on Watcher:

- Push `delete` for entry_id to Mirror
- Mirror rebuilds affected day row from cache invalidation

See [ENTRY_LIFECYCLE.md](ENTRY_LIFECYCLE.md).

---

## Future voice on Mirror

Optional caption from transcript aggregates — off by default; never play audio without explicit user action on Mirror.
