# Phase Report — Camera Memories MVP

**Date:** 2026-06-25  
**Branch:** main  
**Commit:** (pending)  
**Roadmap:** `docs/CIRCE_future_functionality_roadmap_reference.md` — Section 8, guardrails, privacy/consent rules

---

## Summary

Implemented **Camera Memories MVP as scaffold**: full UI flow, consent, worker-backed photo attach path, entry JSON fields, storage layout, review metadata, and delete cleanup. **JPEG capture is not integrated** — `circe_camera_capture_jpeg()` returns unavailable until SSCMA/Himax camera pipeline can be coordinated safely with SD SPI.

---

## Camera files added/modified

| File | Change |
|------|--------|
| `firmware/circe/main/circe_photo.c` | **New** — paths, write, attach, consent NVS, capture stub |
| `firmware/circe/main/circe_photo.h` | **New** — API + status enum |
| `firmware/circe/main/circe_entry.c/h` | Photo JSON fields |
| `firmware/circe/main/circe_storage_paths.c/h` | `PHOTOS` directory helpers |
| `firmware/circe/main/circe_worker.c/h` | `CIRCE_WORKER_PHOTO_CAPTURE`, delete photo cleanup |
| `firmware/circe/main/circe_ui.c` | Consent/capture/result flows; reflection PHOTO button; review line |
| `firmware/circe/main/circe_copy.c/h` | Photo copy keys |
| `firmware/circe/main/circe_conversation_engine.h` | Photo flow IDs |
| `firmware/circe/main/CMakeLists.txt` | Added `circe_photo.c` |
| `schemas/emotion-entry.schema.json` | Optional photo fields |
| `docs/camera/CAMERA_MEMORIES_MVP.md` | **New** |
| `docs/memory/ENTRY_LIFECYCLE.md` | Photo step in lifecycle |
| `docs/ui/COMPANION_INTERFACE_SPEC.md` | Reflection PHOTO action |
| `docs/conversation/CONVERSATION_ENGINE_COPY_POLISH.md` | Photo copy keys |
| `docs/CIRCE_future_functionality_roadmap_reference.md` | Item 8 marked scaffold |

---

## Camera hardware findings

- SenseCAP Watcher camera: **SSCMA client → Himax WE2** over SPI.
- BSP documentation notes **SD card shares SPI bus with SSCMA**.
- Factory firmware uses `tf_module_ai_camera.c` / `sscma_client_invoke()` for vision.
- CIRCE standalone intentionally **does not init SSCMA** at boot (SD journaling stability).
- Standard ESP32-CAM APIs do not apply directly.
- **Blocker:** Safe concurrent SD journaling + SSCMA capture requires explicit bus coordination not yet implemented.

---

## Capture status

**Scaffolded only.** User can complete full consent/capture UI; worker runs attach path; capture stub logs reason and returns `CIRCE_CAMERA_STATUS_UNAVAILABLE`. File write + JSON update code is ready for real JPEG buffer when capture is integrated.

---

## Consent flow

1. Reflection → **PHOTO** (body/quick entries only).
2. Consent screen: optional / saved locally / use only if you choose.
3. **CONTINUE** or **SKIP** (default-safe).
4. NVS `circe_photo/consent_ok` on continue.
5. `photo_training_ok` always **false** — not conflated with training.

---

## Storage path

```
/sdcard/CIRCE/PHOTOS/YYYYMMDD/ENTRYID.JPG
```

Atomic `.TMP` → `.JPG` rename. Directory created on demand by worker.

---

## Entry JSON fields (optional)

- `photo_attached`, `photo_id`, `photo_path`, `photo_created_at`, `photo_consent`, `photo_training_ok`
- Omitted or false when skipped; old entries remain valid.

---

## Review display

When `photo_attached`: review summary shows **Photo memory saved** (metadata only; no image render).

---

## Delete / photo cleanup

On entry delete, worker loads entry and attempts `circe_photo_delete_file()` before removing `.JSN`. Failure logged; entry delete proceeds.

---

## Failure states

| Condition | User-facing | Entry |
|-----------|-------------|-------|
| Camera unavailable | Camera unavailable + entry still saved | Saved |
| Photo write failed | Could not save photo + entry still saved | Saved |
| User skip | Returns to reflection | Saved unchanged |

Technical details logged to serial only.

---

## Build result

**PASS** — `circe.bin` size `0xBA360` (762736 bytes), 94% app partition free.

---

## Flash result

**PASS** — app-flash to `/dev/ttyACM1` at 2 Mbaud. Hash verified. Hard reset OK.

---

## Hardware validation

**Boot validated (serial):**

- CIRCE standalone MVP starting
- NVS initialized
- SD mounted at `/sdcard/CIRCE`
- Worker task started (stack 16384)
- Home wheel created (5 labels)
- No panic, no stack overflow in boot log

**Interactive UI (manual gap):** Full body-save / photo-skip / photo-unavailable paths not exercised in this automated session. Code paths are scaffolded with safe fallbacks; prior save/reflection flows unchanged except additive PHOTO action.

---

## Regression (expected)

| Area | Status |
|------|--------|
| Normal save / photo skip | Expected pass (pending flash) |
| Photo capture path | Unavailable scaffold (by design) |
| Pattern / timeline | No rewrite; TODAY display bug unchanged |
| Regulation | No PHOTO on regulation saves |
| Diagnostics TEST SAVE | No storage worker changes to save path |

---

## Git

Commit and push after this report.

Suggested message: `feat(camera): scaffold optional local photo memories`

---

## Remaining bugs

- **REVIEW → TODAY display** — timeline may log items but browse UI may not show them (unchanged, not fixed this phase).

---

## Recommended next roadmap phase

1. **SSCMA camera integration** — init capture with SD bus coordination; enable real JPEG in existing scaffold.
2. Or **Voice Personality** (item 9) if camera hardware work is deferred.

---

## Validation checklist (post-flash)

- [x] Boot / home wheel / worker (serial)
- [ ] Body save, SKIP photo (manual)
- [ ] CAPTURE → unavailable graceful (manual; expected scaffold behavior)
- [ ] Review photo metadata when attached (manual; requires future capture)
- [ ] Regulation, pattern, diagnostics regressions (manual)
- [ ] Commit + push to `origin/main`
