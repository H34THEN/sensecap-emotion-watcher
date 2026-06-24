# Training Dataset Design

**Status:** Design only — not implemented.

---

## Purpose

Support future machine learning for:

- Photo analysis
- Emotion recognition
- Pattern detection
- Body sensation correlation
- Color association learning
- Voice correlation
- Context correlation
- Sleep correlation

The Watcher is primarily a **data collection device**; training runs on local GPU (Hades) later.

---

## Inclusion criteria

| Rule | Enforcement |
|------|-------------|
| User opted in per entry | `training_ok == true` |
| Not private | `private_locked == false` |
| User initiated export | `export_dataset` module |
| No cloud | LAN/SD only |

Default population: **empty**.

---

## Record fields for ML

### Core labels (user-provided)

- `emotion`, `body_areas`, `body_sensations`
- `color_hex`, `intensity`
- `sleep_rating`, `energy_rating`, `stress_rating`
- `context_tags`, `summary_text`

### Features (derived)

- `hour_of_day`, `day_of_week`
- `flow_path`
- Photo: JPEG bytes + `photo_hash`

### Future modalities

- Audio clips (voice correlation)
- Circe transcript turns
- Ambient RGB LED color (if logged)

---

## Export format

```
circe-export-2026-06-24/
  manifest.json
  entries.jsonl
  photos/
    <uuid>.jpg
  README.txt          # consent + schema documentation
```

### manifest.json

```json
{
  "schema_version": "1.0.0",
  "exported_at": "2026-06-24T20:00:00Z",
  "device_id_hash": "sha256:...",
  "record_count": 42,
  "includes_photos": true
}
```

### Anonymization

- Replace `device_id` with salted hash in export
- Strip EXIF from photos
- Optional: blur faces in photos (Hades batch job — future)

---

## Task definitions (future GPU work)

| Task | Input | Target |
|------|-------|--------|
| Color association | body_sensations + context | color_hex |
| Sensation prediction | emotion + context | body_sensations[] |
| Photo mood | image | color_hex or emotion (weak labels) |
| Sleep-stress | ratings time series | next-day stress |
| Voice tone | audio | sensation tags (consented) |

All models **assistive only** — outputs are suggestions on device.

---

## Negative training data

Do not train on:

- `private_locked` entries
- Factory Watcher surveillance task-flow images
- SenseCraft cloud logs

---

## Versioning

- Dataset bundles versioned with `schema_version`
- Breaking changes require new export migration tool on Hades

---

## Related

- [export_dataset.md](../modules/export_dataset.md)
- [privacy-model.md](../design/privacy-model.md)
- [entry-schema.md](../schema/entry-schema.md)
