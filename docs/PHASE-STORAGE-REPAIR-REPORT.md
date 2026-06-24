# Phase Storage-Repair Report

**Date:** 2026-06-24  
**Hardware:** SenseCAP Watcher `/dev/ttyACM1`  
**Scope:** Fix STORAGE_NOT_READY false negative, path normalization, probe, centered UI  
**Feature freeze:** maintained — no voice/ML/cloud/HUD expansion

---

## 1. Root cause of STORAGE_NOT_READY

Save layer used `access("/sdcard", W_OK)` to decide if storage was ready. Boot init never used this check — only `mkdir` + log. **W_OK fails on ESP-IDF FAT mount point while writes work**, causing every save to return `STORAGE_NOT_READY` despite boot log “storage ready”.

## 2. Was storage_ready actually false?

**At save time, the save-layer check was false even when `s_ready` was true.** The engine flag set at boot could be true while `circe_storage_is_ready()` returned false due to the broken access probe.

## 3. Path mismatch?

User SD card shows `CIRCE/INDEX`, `CIRCE/ENTRIES`. Firmware used `/sdcard/circe/...`. On FAT these are usually the same inode (case-insensitive), but the mismatch hid diagnosis. **Fixed with `circe_storage_paths_resolve()`** detecting existing tree.

## 4. Chosen canonical path

| Path | Purpose |
|------|---------|
| `/sdcard/CIRCE` | Base (prefers existing `/sdcard/CIRCE` or `/sdcard/circe`) |
| `/sdcard/CIRCE/ENTRIES` | JSON entries |
| `/sdcard/CIRCE/INDEX` | Index + `.dirty` |
| `/sdcard/CIRCE/CACHE` | Strand cache |
| `/sdcard/CIRCE/LOGS` | Reserved |

Resolver accepts lowercase subdirs if already present; no data migration needed on FAT.

## 5. Storage probe

At init: write/read/delete `/sdcard/CIRCE/LOGS/PROBE.TMP` (FAT-safe). Was `{base}/storage_probe.tmp` — caused **errno=22 EINVAL** on hardware; fixed in Storage-Probe-EINVAL phase. `s_probe_passed` gates `circe_storage_is_ready()`.

## 6–11. Hardware validation

| Test | Status |
|------|--------|
| Storage probe (serial boot) | **PASS** (see [STORAGE_PROBE_ERRNO_22.md](bugs/STORAGE_PROBE_ERRNO_22.md)) |
| Storage probe (UI) | **Pending user** |
| Test Save | **Pending user** |
| Quick save | **Pending user** |
| Body save (no color / Green / Purple) | **Pending user** |
| Review / Delete / Reboot | **Pending user** |
| Centered UI | **Pending user photo** |

Agent cannot tap Watcher or hold serial when port busy.

## 12. Centered screens

All functional subscreens — see [CENTERED_UI_VALIDATION.md](ui/CENTERED_UI_VALIDATION.md).

## 13. Build

**PASS** — 691,600 B (`0xa8d90`) after Storage-Probe-EINVAL fix

## 14. Flash

**PASS** — app-flash to `/dev/ttyACM1` @ `0x110000` (Storage-Probe-EINVAL build)

## 15. Remaining bugs

1. UI save flow validation pending (Test Save must not reboot — fixed via worker; user confirm)  
2. Touch flow validation pending  
3. Index append may still warn (non-fatal; JSON is truth)  
4. RTC may show `19700101` entry folders until time set  

## 16. Commit recommendation

**Do not commit** until user confirms Test Save (no reboot), body+color save, Review, and centered UI on hardware. Probe PASS and worker task start confirmed via serial boot.

## 17. Next recommended prompt

> Flashed LVGL-Worker build. More → Storage → Test Save shows JSON OK without reboot. Body → Green save works. Review + Delete OK. If all pass, commit storage + worker fixes.

---

## Follow-on phases (after Storage-Repair)

| Phase | Issue | Doc |
|-------|--------|-----|
| Storage-Probe-EINVAL | Probe `errno=22` on `storage_probe.tmp` | [STORAGE_PROBE_ERRNO_22.md](bugs/STORAGE_PROBE_ERRNO_22.md) |
| LVGL-Worker-Task | Test Save stack overflow in LVGL task | [LVGL_STACK_OVERFLOW_STORAGE_IO.md](bugs/LVGL_STACK_OVERFLOW_STORAGE_IO.md), [PHASE-LVGL-WORKER-TASK-REPORT.md](PHASE-LVGL-WORKER-TASK-REPORT.md) |

**Phase stopped.**
