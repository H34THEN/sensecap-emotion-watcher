# CIRCE Data Flow

## Entry draft lifecycle

```
User input (UI modules)
        │
        ▼
   EntryDraft (RAM)
   - id: UUID v4
   - created_at: ISO8601
   - fields: partial → complete
        │
        ▼
   entry_review (read-only)
        │
        ├── retake_photo → camera_capture
        │
        ▼
   training_consent_toggle
   privacy_lock
        │
        ▼
   local_storage.commit()
        │
        ├── entries/YYYY-MM-DD/<uuid>.json
        ├── photos/<uuid>.jpg (optional)
        └── index.sqlite or index.jsonl (TBD)
```

---

## Field ownership by module

| Field | Writer module |
|-------|---------------|
| `emotion` | emotion_tracker |
| `emotion_skipped` | circe_assistant / body-first path |
| `color_hex` | color_picker, hex_color_input |
| `color_source` | color_picker \| hex \| favorite |
| `intensity` | intensity_slider |
| `body_areas[]` | body_sensation_tags |
| `body_sensations[]` | body_sensation_tags |
| `sleep_rating` | sleep_energy_stress_ratings |
| `energy_rating` | sleep_energy_stress_ratings |
| `stress_rating` | sleep_energy_stress_ratings |
| `context_tags[]` | context_tags |
| `summary_text` | feeling_summary |
| `photo_path` | camera_capture |
| `training_ok` | training_consent_toggle |
| `private_locked` | privacy_lock |
| `assistant_transcript` | circe_assistant (optional) |

---

## Read paths

### Mood strand (visualization)

```
local_storage.query(date_range)
    → extract (timestamp, color_hex, intensity)
    → mood_strand_visualizer.render()
```

Multiple entries per day produce multiple strand segments (see [color-system.md](../design/color-system.md)).

### Pattern reflection

```
local_storage.query(all, exclude private_locked unless user opts in)
    → pattern_reflection_engine.aggregate()
    → correlations: body_sensation ↔ color, sleep ↔ stress, etc.
```

On-device aggregates only in Phase 3; no ML required initially.

### Export dataset (opt-in)

```
export_dataset.run(filters)
    → include iff training_ok == true AND private_locked == false
    → anonymize device_id (hash salt in NVS)
    → write export bundle to SD or LAN
```

---

## Sync queue (future)

```
local_storage.on_save(entry)
    if !private_locked && sync_enabled:
        sync_queue.enqueue(entry_id, payload_type)
        
sync_queue.worker (background)
    → POST to Magic Mirror / Hades endpoint on LAN
    → retry with exponential backoff
    → never cloud by default
```

---

## Delete flow

```
delete_entry(id)
    → remove JSON + photo
    → remove from index
    → purge from sync_queue if pending
    → append tombstone log (optional audit)
```

Hard delete only; no soft-delete to cloud.

---

## Privacy enforcement points

| Checkpoint | Rule |
|------------|------|
| Save | Default `training_ok=false`, `private_locked=true` |
| export_dataset | Reject private or non-consented entries |
| sync_queue | Skip private entries |
| pattern_reflection (default) | Exclude private from household dashboards |
| Hades embedding (future) | Require per-entry or global opt-in |

See [privacy-model.md](../design/privacy-model.md).
