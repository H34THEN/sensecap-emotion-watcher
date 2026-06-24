# Emotion Entry Schema (Planning)

**Status:** Draft for Phase 2 implementation. See also [schemas/emotion-entry.schema.json](../../schemas/emotion-entry.schema.json).

---

## Record: `EmotionEntry`

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema_version` | string | yes | e.g. `"1.0.0"` |
| `id` | string (UUID) | yes | Unique entry ID |
| `created_at` | string (ISO8601) | yes | UTC timestamp |
| `updated_at` | string (ISO8601) | no | Set on edit |
| `device_id` | string | yes | Watcher instance ID (hashed in exports) |
| `completion_status` | enum | yes | `complete`, `partial` |
| `flow_path` | enum | yes | `standard`, `body_first` |
| `emotion` | string \| null | no | Named emotion or null |
| `emotion_skipped` | boolean | yes | User declined emotion picker |
| `color_hex` | string | yes | `#RRGGBB` |
| `color_source` | enum | yes | `picker`, `hex`, `favorite`, `default` |
| `intensity` | integer | yes | 1–10 |
| `body_areas` | string[] | yes | From body area taxonomy |
| `body_sensations` | string[] | yes | From sensation taxonomy |
| `sleep_rating` | integer \| null | no | 1–10 |
| `energy_rating` | integer \| null | no | 1–10 |
| `stress_rating` | integer \| null | no | 1–10 |
| `context_tags` | string[] | yes | User/system tags |
| `summary_text` | string | yes | Short free text (may be empty) |
| `photo_path` | string \| null | no | Relative path on SD |
| `photo_hash` | string \| null | no | SHA256 for integrity |
| `training_ok` | boolean | yes | Default `false` |
| `private_locked` | boolean | yes | Default `true` |
| `assistant_transcript` | object | no | Optional Circe conversation log |
| `calibration_snapshot_id` | string | no | Settings version reference |

---

## Indexes

Recommended queries:

- By date: `created_at` day bucket
- By privacy: `private_locked`, `training_ok`
- For strand: `(date, created_at, color_hex, intensity)`
- For patterns: `body_sensations[]`, ratings

---

## Storage layout (proposed)

```
/sdcard/circe/
  entries/2026-06-24/<uuid>.json
  photos/<uuid>.jpg
  index/circe.db          # SQLite — TBD
  config/favorites.json
  config/context_tags.json
  export/                 # user-initiated bundles only
```

---

## Versioning

- Breaking schema changes increment major version.
- `local_storage` migrates or reads multiple versions.
- Export bundles include `schema_version` per record.

---

## Example

```json
{
  "schema_version": "1.0.0",
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "created_at": "2026-06-24T14:32:00Z",
  "device_id": "watcher-local-001",
  "completion_status": "complete",
  "flow_path": "body_first",
  "emotion": null,
  "emotion_skipped": true,
  "color_hex": "#7B68EE",
  "color_source": "picker",
  "intensity": 7,
  "body_areas": ["chest", "throat"],
  "body_sensations": ["tight", "pressure", "shallow_breathing"],
  "sleep_rating": 4,
  "energy_rating": 3,
  "stress_rating": 6,
  "context_tags": ["work", "noise"],
  "summary_text": "Afternoon meeting overload",
  "photo_path": "photos/550e8400-e29b-41d4-a716-446655440000.jpg",
  "photo_hash": null,
  "training_ok": false,
  "private_locked": true
}
```
