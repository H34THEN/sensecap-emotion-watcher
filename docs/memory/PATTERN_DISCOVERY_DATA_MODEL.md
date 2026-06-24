# Pattern Discovery Data Model

Structures for discovering recurring patterns **without implementing** analytics code.

Outputs feed `pattern_reflection_engine` and Hades GPU jobs. Language rules: [CIRCE_PATTERN_REFLECTION_ENGINE.md](../conversation/CIRCE_PATTERN_REFLECTION_ENGINE.md).

---

## Design principles

1. **Observational counts only** — no causal claims in stored labels
2. **Rebuildable from entries** — pattern cache is derived
3. **Privacy-aware** — separate `personal` vs `syncable` aggregates
4. **Extensible** — new pattern types add rows, not schema rewrites

---

## Core entities

### PatternObservation

Single detected recurrence:

```json
{
  "observation_id": "obs-uuid",
  "pattern_type": "color_recurrence | sensation_recurrence | context_trigger | sleep_correlation | emotion_cluster | multimodal_cluster",
  "window": { "kind": "weekly", "start": "2026-06-18", "end": "2026-06-24" },
  "confidence": "low | medium | high",
  "confidence_note": "count-based only",
  "support_count": 4,
  "support_total": 7,
  "includes_private": true,
  "sync_allowed": false,
  "payload": { /* type-specific */ },
  "generated_at": "2026-06-24T23:00:00Z",
  "dismissed_until": null,
  "schema_version": "1.0.0"
}
```

Stored: `patterns/observations/<observation_id>.json` + SQLite `pattern_observations` index.

---

## 1. Recurring colors

### Payload

```json
{
  "pattern_type": "color_recurrence",
  "colors": [
    { "hex": "#4A5568", "count": 5, "entry_ids": ["..."] },
    { "hex": "#5D6D7E", "count": 4, "entry_ids": ["..."] }
  ],
  "cluster_method": "exact_hex | delta_e<15",
  "dominant_cluster": ["#4A5568", "#5D6D7E"]
}
```

### Trigger threshold (default)

- Same hex ≥ 3 times in 7 days, OR
- Cluster ≥ 5 times in 14 days

### Circe reflection template

"Your entries this week share similar colors."

---

## 2. Recurring sensations

### Payload

```json
{
  "pattern_type": "sensation_recurrence",
  "sensations": [
    { "tag": "tight", "count": 6, "areas": ["chest", "shoulders"] }
  ],
  "area_cooccurrence": [
    { "areas": ["chest", "throat"], "count": 3 }
  ]
}
```

### Optional area+sensation pairs (future schema)

```json
"pairs": [{ "area": "chest", "sensation": "tight", "count": 4 }]
```

---

## 3. Recurring triggers (context)

### Payload

```json
{
  "pattern_type": "context_trigger",
  "context_tag": "noise",
  "followed_by": {
    "sensations": [{ "tag": "overstimulated", "count": 4 }],
    "colors": [{ "hex": "#4A5568", "count": 3 }]
  },
  "entry_pairs_within_hours": 2
}
```

**Note:** "trigger" is internal enum name — Circe says **"often appears with"**, never "caused by."

---

## 4. Recurring contexts

Frequency table without pairing:

```json
{
  "pattern_type": "context_frequency",
  "tags": [
    { "tag": "work", "count": 8, "pct_of_entries": 0.4 }
  ]
}
```

---

## 5. Sleep relationships

### Payload

```json
{
  "pattern_type": "sleep_correlation",
  "condition": { "sleep_rating_lte": 4 },
  "correlated": {
    "stress_rating_gte": 7,
    "count": 5,
    "days": 5
  },
  "also_present": {
    "sensations": [{ "tag": "heavy", "count": 4 }]
  },
  "disclaimer_key": "correlation_not_causation"
}
```

### Threshold

≥ 3 matching days in 14-day window.

---

## 6. Emotional clusters

User-provided emotion words only — no ML labels.

```json
{
  "pattern_type": "emotion_cluster",
  "emotion": "weird",
  "associated": {
    "colors": [{ "hex": "#9B59B6", "count": 3 }],
    "sensations": [{ "tag": "floaty", "count": 2 }],
    "contexts": [{ "tag": "alone", "count": 2 }]
  }
}
```

If `emotion_skipped` majority, cluster type `unnamed_body_cluster` instead.

---

## Multimodal cluster (GPU-enriched, Hades)

Watcher stores **pointer only**:

```json
{
  "pattern_type": "multimodal_cluster",
  "cluster_id": "hades-cluster-42",
  "entry_ids": ["..."],
  "human_summary_key": "pattern.multimodal.v1",
  "generated_on": "hades"
}
```

Vectors not stored on Watcher SD by default.

---

## Aggregate tables (SQLite derived)

### `sensation_counts`

| sensation | date_bucket | count |

### `color_counts`

| color_hex | date_bucket | count |

### `context_counts`

| tag | date_bucket | count |

### `rating_pairs`

| sleep_bucket | stress_bucket | count |

Rebuild nightly from entries.

---

## Pattern lifecycle

| Event | Action |
|-------|--------|
| Nightly job | Regenerate observations for rolling windows |
| User dismisses | Set `dismissed_until` + 14 days |
| User deletes entry | Recompute affected observations |
| Export | Observations **not** exported by default (derived analytics) |

---

## Privacy matrix

| Data | Personal view | Mirror sync | GPU export |
|------|---------------|-------------|------------|
| Observations from private entries | Yes (default) | No | No |
| Observations public-only rebuild | Optional | Yes | Aggregates only |
| Entry ids in payload | Yes local | Strip ids | Anonymize |

Setting: `patterns_include_private` default **true** on Watcher, **false** for Mirror payload.

---

## Window types

| Window | Use |
|--------|-----|
| `rolling_7d` | Weekly Circe offers |
| `rolling_14d` | Sleep correlations |
| `rolling_30d` | Monthly summary |
| `calendar_month` | Blanket row stats |
| `all_time` | Calibration hints only |

---

## Future-proofing

New `pattern_type` values append without entry schema change. Renderers ignore unknown types.

GPU may add:

- `photo_color_alignment`
- `voice_tone_sensation`
- `embedding_neighbor`

Each as new observation type referencing `entry_id` list.
