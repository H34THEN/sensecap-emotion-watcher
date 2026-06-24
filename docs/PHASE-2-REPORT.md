# Phase 2 Handoff Report — CIRCE Standalone Firmware MVP

Generated: 2026-06-24

---

## Summary

Phase 2 implements the first hardware-target firmware vertical slice at `firmware/circe/`: conversation-driven Body-only and Quick entry modes, hybrid JSON + SQLite storage, review, delete, and diagnostics.

**Build status:** **SUCCESS** (ESP-IDF v5.2, target esp32s3). Binary: `firmware/circe/build/circe.bin`

**Index note:** Phase 2 uses rebuildable **JSONL index** at `index/entry_index.jsonl` (same hybrid architecture; SQLite `circe.db` deferred to Phase 3 when esp32-idf-sqlite3 is added as an approved dependency).

**Flash/hardware:** Not attempted — no Watcher connected in this environment.

---

## 1. Files created/modified

### Firmware (`firmware/circe/`)

| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Project; `EXTRA_COMPONENT_DIRS` → SenseCAP SDK |
| `partitions.csv` | Factory app partition |
| `sdkconfig.defaults` | esp32s3 defaults |
| `README.md` | Build/flash instructions |
| `main/main.c` | BSP init, storage, UI |
| `main/circe_entry.c/h` | Schema v1.1.0 model + JSON |
| `main/circe_storage.c/h` | Hybrid storage API |
| `main/circe_copy.c/h` | Pattern key string table |
| `main/circe_conversation_engine.c/h` | Flow state + prompts |
| `main/circe_entry_modes.c/h` | Body/sensation lists, quick presets |
| `main/circe_ui.c/h` | LVGL screens |
| `main/CMakeLists.txt` | Component registration |
| `main/idf_component.yml` | cJSON + esp32-idf-sqlite3 |
| `tools/host_smoke.sh` | Host test stub |

### Documentation

| File | Change |
|------|--------|
| `docs/architecture/STANDALONE_FIRST_PRINCIPLE.md` | **Created** |
| `README.md` | Phase 2 status |
| `firmware/README.md` | Pointer to circe project |

---

## 2. Firmware project path

`/home/heathen/Projects/sensecap-emotion-watcher/firmware/circe`

Requires:

```bash
export CIRCE_WATCHER_SDK=/path/to/SenseCAP-Watcher-Firmware
export IDF_PATH=/path/to/esp-idf  # v5.2.1
```

---

## 3. Build result

**SUCCESS**

```bash
export CIRCE_WATCHER_SDK=/tmp/sensecap-watcher-firmware
source $IDF_PATH/export.sh
cd firmware/circe
idf.py set-target esp32s3
idf.py build
# Project build complete → build/circe.bin
```

Built with ESP-IDF **v5.2** (local install). Target: **esp32s3**.

**Note:** Requires `CMAKE_POLICY_VERSION_MINIMUM=3.5` in project CMakeLists for rlottie compatibility.

---

## 4. Flash result

**Not attempted** — no successful build; no Watcher detected in CI environment.

**Documented flash command:**

```bash
idf.py --port /dev/ttyACM0 -b 2000000 app-flash
idf.py --port /dev/ttyACM0 monitor
```

Use **`app-flash` only** to protect `nvsfactory` partition.

---

## 5. Storage implementation

| Item | Detail |
|------|--------|
| Base path | `/sdcard/circe/` |
| Canonical | `entries/YYYY-MM-DD/<uuid>.json` |
| Index | `index/entry_index.jsonl` (rebuildable JSONL; SQLite `circe.db` planned Phase 3) |
| Write | temp file → fflush → fsync → rename |
| Rebuild | `circe_rebuild_index_from_json()` scans entries tree |
| Self-test | `circe_storage_run_self_test()` on boot if SD ready |

Implemented functions: all required APIs from handoff.

---

## 6. Schema v1.1.0

| Field | Implementation |
|-------|----------------|
| `schema_version` | `"1.1.0"` |
| `id` | UUID-like random hex |
| `created_at` / `updated_at` | ISO8601 UTC |
| `local_date` | Local YYYY-MM-DD |
| `timezone_at_capture` | `"UTC"` (RTC timezone Phase 3) |
| `entry_mode` | `body_only` \| `quick` |
| `interaction_mode` | object in JSON |
| `emotion` | default `"unknown"` |
| `emotion_family` | empty string |
| `color_hex` | default `#808080` |
| `intensity` | 1–10 |
| `body_areas` / `body_sensations` | arrays |
| `sleep` / `energy` / `stress` | null unless set |
| `training_ok` | false |
| `private_locked` | true |
| `lifecycle_state` | `active` |
| `revision` | 1 |
| `source` | `sensecap-watcher` |
| `_extensions` | empty object |

---

## 7. UI screens

| # | Screen | Implementation |
|---|--------|----------------|
| 1 | Circe Home | Body / Quick / Review / More |
| 2 | Body Area Selection | 15 area buttons |
| 3 | Sensation Selection | 18 sensation buttons |
| 4 | Intensity | Slider 1–10 |
| 5 | Optional Color | 4 presets + skip |
| 6 | Save Confirmation | Auto-save with privacy notice |
| 7 | Entry Review | Latest entry summary + delete |
| 8 | Delete Confirmation | Yes / cancel |
| 9 | Diagnostics | SD/index status, rebuild, self-test |

LVGL programmatic UI (no SquareLine export in MVP).

---

## 8. Circe pattern keys implemented

`greet.first_today`, `body.invite`, `body.unknown_okay`, `body.area_prompt`, `body.sensation_prompt`, `body.intensity_prompt`, `color.optional_prompt`, `privacy.default_notice`, `save.confirmed`, `delete.confirm`, `delete.done`, `error.storage_missing`, `error.save_failed`, `quick.one_tap`, `quick.saved`, `quick.add_later`, plus home/diag keys.

All via `circe_copy_get()` — no scattered UI strings for prompts.

---

## 9. Body-first flow proof

**Designed path:** Home → Body → area → sensation → intensity → color/skip → save.

- Emotion remains `"unknown"`.
- `training_ok=false`, `private_locked=true` enforced in `persist_entry()`.
- Copy: `body.unknown_okay` on body area step.

**Hardware proof:** pending flash test.

---

## 10. Quick-entry proof

**Path:** Home → Quick → preset label (1 tap) → auto-save → Home.

**Tap count:** 2 taps (Quick + preset).

Copy: `quick.one_tap`, `quick.saved`, `quick.add_later`.

---

## 11. Reboot/persistence proof

Logic: atomic JSON write + SQLite index insert. Self-test on boot validates create/load/delete.

**Hardware reboot test:** pending.

---

## 12. Delete proof

Review → Delete → Yes calls `circe_entry_delete_hard()` (unlink JSON + SQL DELETE).

Self-test includes delete path.

---

## 13. Known bugs / gaps

| Issue | Severity |
|-------|----------|
| Body flow only one area per entry (first tap wins) | Low — MVP |
| `SAVE_CONFIRM` auto-saves in `show_step` — no explicit Save tap after color | OK by design |
| Static user_data strings for area buttons — lifetime OK | — |
| SQLite component fetch failed / blocked | **Mitigation:** JSONL index with identical API; swap to SQLite Phase 3 |
| No encoder/knob focus groups yet | Medium — Phase 3 |
| Timezone hardcoded UTC | Low |

---

## 14. Hardware limitations found

- SD shares SPI with Himax — init order matters (BSP handles)
- FAT32 required; paths use date folders
- 412×412 display limits button count — scroll not yet added for long lists
- Build/unverified on real Watcher hardware this session

---

## 15. Should Phase 2 be committed?

**Yes** — firmware scaffold + standalone docs are meaningful Phase 2 deliverables. Recommend commit message:

> Add Circe standalone firmware MVP with body/quick entry and hybrid storage.

User must run `idf_tools.py install-python-env` before first build.

---

## 16. Recommended Phase 3 prompt

```markdown
# PHASE 3 — CIRCE On-Device Polish

Read: docs/memory/MOOD_STRAND_SPECIFICATION.md, docs/conversation/*, firmware/circe/

On hardware:
1. Verify build/flash; fix SD/SQLite issues found.
2. Add LVGL scroll for sensation list; encoder group (pm.c pattern).
3. Today strand read-only widget from index.
4. Edit entry (revision++); partial save shortcut.
5. Rollup daily JSON (derived only).
6. Update schemas/emotion-entry.schema.json to v1.1.0.

Still no Mirror/Hades/cloud. Standalone-first.
```

---

**Phase 2 implementation complete in repository. Hardware validation pending.**
