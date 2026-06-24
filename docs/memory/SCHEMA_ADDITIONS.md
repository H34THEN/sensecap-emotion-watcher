# Schema Additions (Phase 1.75)

Proposed extensions to `EmotionEntry` and related stores to support memory architecture **without future rewrites**.

Update [schemas/emotion-entry.schema.json](../../schemas/emotion-entry.schema.json) in Phase 2 when implementing.

---

## EmotionEntry — new fields

| Field | Type | Required | Phase |
|-------|------|----------|-------|
| `entry_mode` | enum | yes | 2 |
| `interaction_mode` | object | no | 2 |
| `lifecycle_state` | enum | yes | 2 |
| `revision` | integer | yes | 2 |
| `timezone_at_capture` | string | no | 2 |
| `local_date` | string (YYYY-MM-DD) | yes | 2 |
| `photo_captured_at` | string \| null | no | 2b |
| `color_user_label` | string \| null | no | 3 |
| `integrity_warning` | boolean | no | 3 |
| `exported_at` | string \| null | no | 4 |
| `sync_status` | enum | no | 3 |
| `embedding_ref` | object \| null | no | 5 |
| `_extensions` | object | no | any |

### `entry_mode`

```json
"enum": ["quick", "body_only", "color", "full", "photo", "voice", "shutdown"]
```

### `interaction_mode`

```json
{
  "short_answer": false,
  "icon_first": false,
  "low_energy": false
}
```

### `lifecycle_state`

```json
"enum": ["draft", "active", "deleted"]
```

Persisted drafts optional; active entries default.

### `revision`

Integer ≥ 1. Increment on edit.

### `local_date`

Local calendar date bucket for strand — computed at save from `created_at` + user timezone.

### `sync_status`

```json
"enum": ["local_only", "queued", "synced", "failed", "delete_pending"]
```

---

## assistant_transcript structure (normative)

```json
{
  "turns": [
    {
      "role": "circe | user",
      "at": "ISO8601",
      "pattern_key": "body.invite",
      "action": "selected_sensation",
      "value": "tight"
    }
  ],
  "voice": {
    "audio_path": "voice/uuid.wav",
    "transcript_path": "voice/transcripts/uuid.json",
    "verified": false
  }
}
```

---

## New top-level store schemas

### Device config `config/device.json`

```json
{
  "schema_version": "1.0.0",
  "device_id": "watcher-001",
  "timezone": "America/New_York",
  "sync_enabled": false,
  "default_entry_mode": "body_only",
  "photo_prompts": "soft",
  "patterns_include_private": true,
  "backup_nas_url": null
}
```

### SQLite `entry_index` table

| Column | Type |
|--------|------|
| id | TEXT PK |
| local_date | TEXT |
| created_at | TEXT |
| updated_at | TEXT |
| revision | INT |
| lifecycle_state | TEXT |
| entry_mode | TEXT |
| private_locked | INT |
| training_ok | INT |
| color_hex | TEXT |
| intensity | INT |
| sync_status | TEXT |
| json_path | TEXT |
| schema_version | TEXT |

### Sidecar schemas

- `photos/<id>.meta.json` — see PHOTO_MEMORY_MODEL
- `voice/<id>.meta.json` — see VOICE_MEMORY_MODEL
- `patterns/observations/<id>.json` — see PATTERN_DISCOVERY_DATA_MODEL

---

## `_extensions` pattern

Future fields ship in `_extensions` until promoted to core:

```json
{
  "schema_version": "1.0.0",
  "id": "...",
  "_extensions": {
    "experimental_rgb_led": "#7B68EE"
  }
}
```

Readers merge known extensions on migration. Prevents major version churn.

---

## JSON Schema relaxation

Change root `additionalProperties` from `false` to:

```json
"additionalProperties": false,
"properties": {
  ...
  "_extensions": { "type": "object" }
}
```

Or use `additionalProperties: true` for `_extensions` only via `patternProperties` in draft 2020-12 — document choice in Phase 2.

---

## Version roadmap

| Version | Changes |
|---------|---------|
| 1.0.0 | Phase 1 baseline |
| 1.1.0 | entry_mode, lifecycle, revision, local_date, interaction_mode |
| 1.2.0 | photo_captured_at, photo sidecar consent |
| 1.3.0 | sync_status, voice block in transcript |
| 2.0.0 | Reserved for breaking renames only |

---

## Migration example 1.0.0 → 1.1.0

```json
{
  "defaults": {
    "entry_mode": "full",
    "revision": 1,
    "lifecycle_state": "active",
    "local_date": "derive from created_at + config.timezone"
  }
}
```

Run once via `circe-migrate` or firmware first-boot job.
