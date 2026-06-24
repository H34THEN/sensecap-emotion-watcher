# Photo Memory Model

Photo storage, metadata, consent, retention, embeddings, and deletion — linked to entries but managed as **media sidecars**.

---

## Storage layout

```
/sdcard/circe/
  photos/
    <entry_id>.jpg              # primary
    <entry_id>_thumb.jpg        # optional 128px preview
  photos/_trash/                # post-edit/delete quarantine
  photos/_orphan/               # compaction candidates
```

**Never** store JPEG bytes inside SQLite.

---

## Photo metadata

### In EmotionEntry (canonical)

| Field | Type | Description |
|-------|------|-------------|
| `photo_path` | string | Relative path e.g. `photos/<uuid>.jpg` |
| `photo_hash` | string | SHA256 of file bytes |
| `photo_captured_at` | ISO8601 | Shutter time (may differ from entry `created_at`) |

### Sidecar `photos/<entry_id>.meta.json` (optional extension)

```json
{
  "entry_id": "uuid",
  "schema_version": "1.0.0",
  "format": "jpeg",
  "width": 640,
  "height": 480,
  "size_bytes": 184320,
  "sha256": "...",
  "exif_stripped": true,
  "training_ok": false,
  "private_locked": true,
  "photo_consent": {
    "capture_ok": true,
    "export_ok": false,
    "embedding_ok": false,
    "sync_ok": false
  },
  "embedding_ref": null
}
```

Sidecar **mirrors** entry privacy flags at capture time; updated on edit.

---

## Photo consent layers

| Layer | Default | Controls |
|-------|---------|----------|
| Capture | User taps shutter | Photo exists locally |
| Entry `private_locked` | true | Blocks sync/export |
| Entry `training_ok` | false | Blocks training export |
| `photo_consent.export_ok` | false | Explicit photo in export bundle |
| `photo_consent.embedding_ok` | false | Hades may compute vision embedding |
| `photo_consent.sync_ok` | false | Mirror thumbnail |

**Rule:** Export including photos requires `training_ok && !private_locked && export_ok`.

Training on **labels only** (no pixels) may proceed without `export_ok` if user enables text-only export — separate export profile.

---

## Training consent

Photos are **high sensitivity**.

| Export profile | Includes photos |
|----------------|-----------------|
| `labels_only` | No |
| `full_consented` | Yes, if all gates pass |
| `photos_only_research` | User rare opt-in + manifest signature |

Hades pipeline strips EXIF; optional face blur job documented in ml design.

Circe never auto-enables photo training consent.

---

## Retention

| Policy | Default | Behavior |
|--------|---------|----------|
| Keep with entry | Yes | Photo life tied to entry |
| Delete entry | Delete photo | Move to `_trash` → purge |
| Orphan cleanup | Monthly job | `_orphan/` if no index row |
| Max storage cap | User setting off | Optional: warn at 80% SD |
| Age-based purge | Off | Future: "delete photos older than N years" |

### Retention without deleting entry

User may **remove photo** from entry:

- `photo_path` → null
- File → `_trash`
- Entry otherwise intact

---

## Future embedding support

### On Hades (default)

1. Export includes consented JPEGs
2. GPU computes vision embedding
3. Store in Hades vector DB: `{ entry_id_hash, embedding_id, model_version }`

### On Watcher (not default)

Optional `embedding_ref`:

```json
"embedding_ref": {
  "store": "hades",
  "id": "emb-uuid",
  "model": "clip-personal-v1"
}
```

Watcher does **not** store 512+ dim vectors on SD unless user explicitly enables "local similarity cache" (Phase 4+).

### Similarity search

Hades returns neighbor entry ids → Watcher fetches local JSON if still present.

---

## Deletion behavior

| Action | Photo file | Sidecar | Index |
|--------|------------|---------|-------|
| Hard delete entry | purge after tombstone | delete | remove |
| Remove photo only | trash | delete | update entry |
| Edit retake | old → trash, new write | replace meta | update hash |
| Export | copy only | N/A | N/A |

Sync delete event to Mirror if thumbnail was synced.

---

## Integrity

On boot optional scan:

- Verify `photo_hash` matches file
- Mismatch → flag entry `integrity_warning: true` in index
- Circe: "A photo file couldn't be read — entry text is still here."

---

## Related

- [CIRCE_PHOTO_CAPTURE_EXPERIENCE.md](../conversation/CIRCE_PHOTO_CAPTURE_EXPERIENCE.md)
- [ENTRY_LIFECYCLE.md](ENTRY_LIFECYCLE.md)
- [GPU_PIPELINE_PLAN.md](GPU_PIPELINE_PLAN.md)
