# Module: camera_capture

**Layer:** UI / Hardware  
**Phase:** 2

## Purpose

Optional photo associating visual memory with entry.

## Behavior

- Trigger still capture via Watcher camera path
- Store JPEG to microSD
- Preview before proceeding

## Outputs

- `photo_path`
- `photo_hash` (optional SHA256)

## Unknowns (Phase 2)

- API outside task-flow `ai camera` module
- Reference: `view_image_preview.c` in factory firmware

## Privacy

- Photos inherit `private_locked` and `training_ok` from entry
- No automatic upload or analysis
