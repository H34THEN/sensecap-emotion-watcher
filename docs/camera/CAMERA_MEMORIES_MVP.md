# Camera Memories MVP

**Phase:** Camera Memories MVP  
**Firmware:** `firmware/circe`  
**Status:** Scaffold — UI, storage, entry metadata; capture blocked pending SSCMA integration

---

## Purpose

Optional, user-triggered photo memory linked to a body check-in entry. Not emotion recognition, not automatic capture, not training.

---

## User flow

```
Entry Ready → SAVE → worker save → Reflection
  → PHOTO (body/quick only)
  → Consent (PHOTO MEMORY / optional / saved locally)
  → CAPTURE or SKIP
  → Worker photo job
  → Result (saved / unavailable / failed)
  → REVIEW or HOME
```

Regulation saves do not offer photo memory.

---

## Consent

Short consent screen before capture. `CONTINUE` stores NVS flag `circe_photo/consent_ok`. User can `SKIP` at any step. Entry remains saved regardless.

`photo_training_ok` is always **false** — separate from training consent.

---

## Storage

| Item | Path |
|------|------|
| Base | `/sdcard/CIRCE/PHOTOS/` |
| File | `/sdcard/CIRCE/PHOTOS/YYYYMMDD/ENTRYID.JPG` |

FAT-safe 8.3-style names. Atomic write via `.TMP` rename.

---

## Entry JSON (optional)

```json
{
  "photo_attached": true,
  "photo_id": "B488E87C",
  "photo_path": "/sdcard/CIRCE/PHOTOS/20260624/B488E87C.JPG",
  "photo_created_at": "2026-06-24T20:12:00Z",
  "photo_consent": true,
  "photo_training_ok": false
}
```

---

## Camera hardware investigation

SenseCAP Watcher camera uses **SSCMA client → Himax WE2** over SPI. BSP notes: SD card shares SPI with SSCMA — coordinating capture with active SD journaling is non-trivial.

CIRCE standalone does **not** initialize SSCMA at boot (preserves SD stability).

`circe_camera_capture_jpeg()` currently returns `CIRCE_CAMERA_STATUS_UNAVAILABLE` with logged reason. Photo file write path is implemented and ready when capture is integrated.

Reference: factory `tf_module_ai_camera.c`, `sscma_client_monitor` example.

---

## Worker safety

| Job | Task |
|-----|------|
| `CIRCE_WORKER_PHOTO_CAPTURE` | Attempt capture, write JPEG, update entry JSON |

No photo work on LVGL task. Delete entry attempts photo file removal first.

---

## Review

Entries with `photo_attached` show **Photo memory saved** in review summary (metadata only — no image render in MVP).

---

## Failure states

| State | User copy |
|-------|-----------|
| Camera unavailable | Camera unavailable. Entry is still saved. |
| Photo save failed | Could not save photo. Entry is still saved. |
| Skip | Returns to reflection; entry unchanged |

---

## Guardrails

- No automatic capture
- No ML / cloud / face analysis
- No training pipeline
- Do not block journaling for camera

---

## Known limitations

- **Capture not integrated** — scaffold only
- **REVIEW → TODAY display bug** unchanged
- No photo preview/thumbnail in review UI
