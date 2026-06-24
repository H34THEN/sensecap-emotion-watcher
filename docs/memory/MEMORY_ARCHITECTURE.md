# Memory Architecture

How Circe remembers across time scales. Designed for **years** of data on SenseCAP Watcher microSD (FAT32, up to 32 GB) with optional LAN GPU and Magic Mirror consumers.

**Principle:** Entries are immutable facts; rollups are derived and rebuildable; embeddings live off-device by default.

---

## System overview

```
┌─────────────────────────────────────────────────────────────────┐
│ SHORT-TERM (RAM)          EntryDraft, session, UI state         │
└───────────────────────────────┬─────────────────────────────────┘
                                │ save
┌───────────────────────────────▼─────────────────────────────────┐
│ CANONICAL (microSD)       Per-entry JSON + media sidecars       │
│                           SQLite index (derived, rebuildable)   │
└───────────────────────────────┬─────────────────────────────────┘
                                │ nightly / on-demand rollup
┌───────────────────────────────▼─────────────────────────────────┐
│ DAILY / WEEKLY / MONTHLY  Rollup JSON (derived caches)           │
└───────────────────────────────┬─────────────────────────────────┘
                                │ export (consent-gated)
┌───────────────────────────────▼─────────────────────────────────┐
│ LONG-TERM (LAN)           Hades embeddings, GPU pattern models    │
│                           Magic Mirror visualization cache        │
└─────────────────────────────────────────────────────────────────┘
```

See [STORAGE_DECISION_STUDY.md](STORAGE_DECISION_STUDY.md) for format choice.

---

## Short-term memory

**Duration:** Current session only (minutes).  
**Location:** ESP32 RAM. Lost on crash unless autosave draft enabled.

### Stores

| Artifact | Contents |
|----------|----------|
| `EntryDraft` | In-progress fields before commit |
| `ConversationState` | Pattern key, step index, interaction_mode |
| `SessionTranscript` | Circe turns + user button choices (optional) |
| `UI focus state` | Encoder group, scroll positions |

### Rules

- Never synced, never exported, never trained on.
- Optional **draft autosave** to `/circe/drafts/<uuid>.draft.json` on idle (user setting, default off).
- On successful save, draft deleted.

### Circe use

- Reflect current selections in review: "Chest, tight — color next?"
- No cross-session recall except via long-term layers.

---

## Daily memory

**Duration:** One calendar day (user timezone).  
**Location:** microSD derived + index.

### Stores

| Artifact | Path (proposed) | Contents |
|----------|-----------------|----------|
| Entry records | `entries/YYYY-MM-DD/<uuid>.json` | Canonical EmotionEntry |
| Day rollup | `rollups/daily/YYYY-MM-DD.json` | Aggregates, strand row |
| Day strand | embedded in daily rollup | Ordered color blocks |
| Day index row | SQLite `days` table | entry_count, first_at, last_at |

### Daily rollup schema (derived)

```json
{
  "rollup_version": "1.0.0",
  "date": "2026-06-24",
  "timezone": "America/New_York",
  "entry_ids": ["uuid-1", "uuid-2"],
  "entry_count": 2,
  "strand": { "see": "MOOD_STRAND_SPECIFICATION.md" },
  "aggregates": {
    "colors": ["#7B68EE", "#4A5568"],
    "top_sensations": [{"tag": "tight", "count": 2}],
    "avg_sleep": 4.0,
    "private_count": 2,
    "training_eligible_count": 0
  },
  "generated_at": "2026-06-24T23:59:00Z"
}
```

### Rebuild

Delete rollup → regenerate from entry files + SQLite index. Safe operation.

### Circe use

- "Three entries today."
- Today strand widget on Watcher home.
- Inputs pattern discovery for "this week" (rolls up from dailies).

---

## Weekly memory

**Duration:** ISO week or Sunday–Saturday (user setting).  
**Location:** microSD derived.

### Stores

| Artifact | Path | Contents |
|----------|------|----------|
| Week rollup | `rollups/weekly/2026-W25.json` | Cross-day stats |
| Pattern candidates | embedded `pattern_hints[]` | Precomputed co-occurrences |
| Mirror payload cache | optional `mirror/weekly/2026-W25.json` | Non-private strand summary |

### Weekly rollup highlights

- Dominant colors (hex clusters, not meanings)
- Sensation frequency table
- Sleep vs stress correlation **counts** (not causation)
- Entry count by day (missing days explicit)
- `reflection_dismissed_ids[]` — user hid a pattern offer

### Circe use

- Weekly reflection offer (on demand): "Several entries share similar colors this week."
- Magic Mirror weekly band.

### Regeneration

Rebuild from daily rollups or raw entries (slower).

---

## Monthly memory

**Duration:** Calendar month.  
**Location:** microSD derived + optional LAN archive.

### Stores

| Artifact | Path | Contents |
|----------|------|----------|
| Month rollup | `rollups/monthly/2026-06.json` | Blanket row metadata |
| Mood blanket row | `blanket_rows/2026-06.json` | One row = one day stitch pattern |
| Long-term stats | embedded | Tag lifetime counters |

### Monthly rollup highlights

- Days with entries vs missing days
- Color palette histogram (user's hexes)
- Body area heatmap counts (aggregate, not diagnostic)
- Voice/photo counts (future)
- Export bundle manifest pointers

### Circe use

- "Your June blanket has 18 active days."
- Monthly summary card on Mirror.

---

## Long-term memory

**Duration:** Indefinite (years).  
**Location:** microSD canonical entries + LAN Hades/Mirror.

### On Watcher (microSD)

| Store | Retention | Notes |
|-------|-----------|-------|
| All entry JSON files | Until user deletes | Canonical source of truth |
| Photos / voice sidecars | Policy per PHOTO/VOICE models | Linked by entry id |
| SQLite index | Rebuildable | Fast query |
| Rollups | Rebuildable | Can prune old rollups, not entries |
| Pattern cache | Optional prune > 2 years | Recomputable |
| Config / calibration | Indefinite | NVS + SD mirror |

### On Hades (opt-in export only)

| Store | Contents |
|-------|----------|
| Export bundles | JSONL + media |
| Embeddings index | Vectors per entry/modality |
| GPU model artifacts | Personal fine-tunes |
| Pattern graphs | Clusters, not diagnoses |

### On Magic Mirror (opt-in sync only)

| Store | Contents |
|-------|----------|
| Strand cache | Non-private color timelines |
| Summary cards | Weekly/monthly JSON |
| No raw photos by default | Thumbnail opt-in |

### Circe use

- Pattern reflections with hedged language
- "Similar to entries you saved in March" (similarity from Hades, user-confirmed)
- Never: "You have depression based on 2024 data"

---

## Memory layer comparison

| Layer | Mutable | Export default | Training default | Rebuildable |
|-------|---------|----------------|------------------|-------------|
| Short-term | Yes | No | No | N/A |
| Daily rollup | Derived | Aggregates only | No | Yes |
| Weekly rollup | Derived | Aggregates only | No | Yes |
| Monthly rollup | Derived | Aggregates only | No | Yes |
| Entry canonical | Edit creates revision | Per consent | Per entry flag | No (source) |
| Hades embeddings | Separate | User export | User export | From export |

---

## Timezone and "day" boundaries

- User timezone stored in `config/device.json` (`timezone: IANA string`).
- `created_at` always UTC in entries.
- Daily rollups bucket by **local date** of `created_at`.
- Strand "missing day" = no entries in local calendar day.

---

## Privacy by layer

| Layer | private_locked entries |
|-------|-------------------------|
| Short-term | Always local |
| Daily/weekly/monthly rollups (personal view) | Included in on-device stats (default) |
| Rollups for Mirror sync | **Excluded** |
| Export / GPU | **Excluded** unless both flags allow |
| Pattern hints for household | **Excluded** |

See [privacy-model.md](../design/privacy-model.md).

---

## Capacity planning (years)

Assumptions: 3 entries/day avg, 2 KB JSON/entry, 200 KB photo 10% of entries.

| Years | Entries | JSON ~ | Photos ~ | Rollups ~ | Total ~ |
|-------|---------|--------|----------|-----------|---------|
| 1 | 1,100 | 2 MB | 22 MB | 1 MB | 25 MB |
| 5 | 5,500 | 11 MB | 110 MB | 5 MB | 130 MB |
| 10 | 11,000 | 22 MB | 220 MB | 10 MB | 260 MB |

Well within 32 GB. Photo retention policy may dominate — see [PHOTO_MEMORY_MODEL.md](PHOTO_MEMORY_MODEL.md).

---

## Related

- [ENTRY_LIFECYCLE.md](ENTRY_LIFECYCLE.md)
- [STORAGE_DECISION_STUDY.md](STORAGE_DECISION_STUDY.md)
- [SCHEMA_ADDITIONS.md](SCHEMA_ADDITIONS.md)
