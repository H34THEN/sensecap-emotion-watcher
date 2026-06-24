# Module: export_dataset

**Layer:** Data  
**Phase:** 4

## Purpose

User-initiated export bundle for offline GPU training on Hades.

## Inclusion rules

Entry exported **only if**:

- `training_ok == true`
- `private_locked == false`
- User confirms export dialog

## Bundle contents

- JSONL entries (device_id hashed)
- Optional photo folder
- Manifest with schema_version, export_date, record_count

## Destinations

- microSD `/circe/export/`
- LAN copy to Hades (HTTPS)

## Never

- Automatic export
- Cloud upload

See [training-dataset-design.md](../ml/training-dataset-design.md).
