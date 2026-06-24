# Phase 2 Implementation Roadmap

**Prerequisite:** Phase 1 research and documentation complete.

---

## Phase 2 goal

Minimal viable **local journaling firmware** on SenseCAP Watcher: body-first entry flow, save to microSD, privacy defaults enforced.

---

## Milestones

### M1 — Development environment (week 1)

- [ ] Install ESP-IDF v5.2.1
- [ ] Clone SenseCAP-Watcher-Firmware + submodules
- [ ] Build and flash `factory_firmware` to hardware
- [ ] Verify serial console, display, touch, encoder, SD mount
- [ ] Backup `nvsfactory` partition

### M2 — Project scaffold (week 1–2)

- [ ] Create `firmware/circe` based on `factory_firmware` copy
- [ ] Strip or gate SenseCraft MQTT (spike: compile without cloud)
- [ ] Add Circe menu entry on Watcher home (new SquareLine page)
- [ ] Prove `lvgl_port_lock` pattern with hello screen

### M3 — Storage spike (week 2)

- [ ] microSD FAT mount via BSP
- [ ] Write/read test JSON file
- [ ] Decide SQLite vs JSONL ( spike both )
- [ ] Implement `local_storage` minimal API

### M4 — Body-first flow UI (week 3–4)

- [ ] circe_assistant scripted screens
- [ ] body_sensation_tags (list UI first; map second)
- [ ] emotion_tracker with skip
- [ ] color_picker + hex_color_input + intensity_slider
- [ ] sleep_energy_stress_ratings, context_tags, feeling_summary
- [ ] entry_review + privacy toggles + save

### M5 — Camera (week 4–5)

- [ ] Investigate still capture API
- [ ] camera_capture + retake_photo
- [ ] Photo file linkage in entry schema

### M6 — Polish (week 5–6)

- [ ] delete_entry
- [ ] Error handling (SD full, SD removed)
- [ ] Encoder navigation QA on all screens
- [ ] RGB LED optional feedback

---

## Phase 3 preview

- mood_strand_visualizer (today view)
- pattern_reflection_engine (basic counts)
- calibration_mode
- sync_queue + LAN API sketch

---

## Phase 4 preview

- export_dataset
- Magic Mirror module prototype
- Hades ingestion script

---

## Risk register

| Risk | Mitigation |
|------|------------|
| SenseCraft coupling in factory FW | Early fork/spike without MQTT |
| Camera API unclear | Fallback: skip photo in MVP |
| SD reliability | Atomic writes; user warnings |
| 412×412 UI density | Body map as list-first MVP |
| Battery life | USB-powered dev; document runtime later |

---

## Definition of done (Phase 2)

1. User completes body-first entry end-to-end on device.
2. Entry persists across reboot on microSD.
3. Defaults: `training_ok=false`, `private_locked=true`.
4. No network calls during entry flow.
5. Documentation updated with API decisions made.

---

## Recommended next agent focus

Single vertical slice: **body_sensation_tags → color → save → reload on review screen**.

Do not start ML, Mirror, or Hades integration until slice is stable on hardware.
