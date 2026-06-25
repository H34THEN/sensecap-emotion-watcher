# Phase RTC-Time Report

## UI-stability validation (pre-requisite)

| Check | Result |
|-------|--------|
| Build | **PASS** (`0xad490`, ~709 KB) |
| App-flash | **PASS** — `/dev/ttyACM1` @ `0x110000` (2026-06-24 session) |
| Boot loop (serial) | **No loop observed** — single `ESP-ROM` marker after flash; no Guru/stack overflow in captured boot |
| Home / settings / diagnostics / save-review-delete | **Manual on-device** — automation cannot drive knob/UI; re-verify checklist below |

Pre-flash note: close `idf.py monitor` if port busy (`fuser /dev/ttyACM1`).

On-device checklist:

1. Boot reaches home (CIRCE / online / ready)
2. Header and feed do not overlap
3. Settings → More opens without crash
4. Diagnostics has no Strand / TODAY STRAND button
5. Save → Review → Delete still works
6. Settings → More → TIME shows SET/UNSET
7. Save with time unset → `ENTRIES/UNSET/`
8. Set date/time → SAVE TIME → save → `ENTRIES/YYYYMMDD/`
9. Reboot → time restored from NVS → review still finds entry

Flash command:

```bash
cd firmware/circe && idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

## Clock source

1. **`bsp_rtc_init()`** (SenseCAP BSP) — hardware RTC if present
2. **ESP-IDF system time** — `time()` / `settimeofday()`
3. **NVS manual backup** — namespace `circe_time`, restored on boot when system year &lt; 2020

## settimeofday()

**Yes.** `circe_time_apply()` and NVS restore both call `settimeofday()`.

## Manual time UI

**Implemented** under Settings → **TIME**:

- Display: date, time, status (SET/UNSET)
- SET DATE (year/month/day sliders)
- SET TIME (hour/minute sliders)
- SAVE TIME (persist + apply)
- Back navigation

Terminal-style rows; no keyboard.

## Time persistence after reboot

- **Manual SAVE TIME:** persisted in NVS; reapplied on boot if RTC/system still invalid → **should persist**
- **RTC-provided valid time:** used directly each boot without NVS
- If neither RTC nor NVS: status **UNSET**, subline prompts user to set time

Document on device after reboot test.

## Save path format

| Time state | Path |
|------------|------|
| Set | `/sdcard/CIRCE/ENTRIES/YYYYMMDD/` |
| Unset | `/sdcard/CIRCE/ENTRIES/UNSET/` |

## Legacy 19700101 compatibility

- Existing entries under `19700101/` remain on SD
- Review scan walks all folders — **still finds legacy entries**
- New unset saves use `UNSET`, not `19700101`

## Save / review / delete tests

**Pending hardware** after flash. Code paths unchanged except folder resolution via `circe_time_entry_storage_folder()`.

## Build result

**PASS** — `circe.bin` size `0xad490` (~709 KB)

## Flash result

**PASS** — app-only flash to `0x110000` on `/dev/ttyACM1` (709776 bytes written, hash verified).

## Boot result

Serial capture after flash: normal ESP-IDF boot, no repeated reset loop in observed window. Full `circe_time` log line not captured (monitor contention); confirm on device via Settings → TIME status after reboot.

## Remaining bugs

- Hardware validation not run in this session
- Date edit uses sliders (not per-field knob stepping) — acceptable MVP
- `circe_storage_health_check()` still runs on LVGL task in diagnostics
- Strand remains disabled

## Commit recommendation

**Yes, after hardware checklist passes:**

```
feat(time): manual date/time, UNSET/YYYYMMDD entry folders

NVS-backed settimeofday, TIME settings UI, time_status in JSON.
Legacy 19700101 entries still load via folder scan.
```

Files: `circe_time.c/h`, `circe_entry.c`, `circe_storage.c`, `circe_save.c`, `circe_ui.c`, `main.c`, copy/engine headers, docs.

## Recommended next phase

1. **Hardware validate** UI-stability + RTC save paths on Watcher
2. **Optional:** migrate or relabel `19700101` → `UNSET` tool (diagnostics only)
3. **Optional:** worker-backed diagnostics health check (LVGL stack safety)
4. Defer voice, camera, ML, cloud, Strand until logging UX is stable
