# Phase Storage-Probe-EINVAL Report

**Date:** 2026-06-24  
**Hardware:** SenseCAP Watcher `/dev/ttyACM1`, FAT32 SD  
**Scope:** Fix probe `errno=22` (EINVAL), FAT-safe paths, staged probe logging  
**Feature freeze:** maintained

---

## 1. Exact failing path (before fix)

| Field | Value |
|-------|--------|
| Probe path | `/sdcard/CIRCE/storage_probe.tmp` |
| fopen mode | `"w"` |
| errno | `22` (`EINVAL`) |
| stat on dirs | OK (mount + CIRCE tree) |

## 2. Reason errno=22 occurred

Probe attempted to create/open a lowercase dot-segment file at the **base directory** (`storage_probe.tmp`). ESP-IDF FAT/VFS rejected the open with `EINVAL` while directory `stat()` calls succeeded. Not missing SD, not exFAT.

## 3. Fixed probe path

`/sdcard/CIRCE/LOGS/PROBE.TMP` — 8.3-style, uppercase, in LOGS subdir.

## 4. Probe result (hardware serial)

**PASS** — all stages A–J OK; `fopen(w)` succeeded (no POSIX fallback needed).

```
probe D fopen(w) OK
probe PASS write=OK read=OK del=OK path='/sdcard/CIRCE/LOGS/PROBE.TMP'
storage ready base=/sdcard/CIRCE
```

## 5. Storage ready result

**YES** at boot (`s_ready` + `s_probe_passed`).

## 6–9. Save / UI validation

| Test | Status |
|------|--------|
| Test Save (UI) | **Pending user** |
| Body + color save | **Pending user** |
| Review / Delete | **Pending user** |
| Storage screen UI | **Pending user** |

Agent confirmed probe + init via serial only; cannot tap Watcher UI.

## 10. Build result

**PASS** — 691,600 B (`0xa8d90`), compile Jun 24 2026 11:17:31

## 11. Flash result

**PASS** — `idf.py --port /dev/ttyACM1 -b 2000000 app-flash` @ `0x110000`

(First attempt failed: port busy; released monitor PID and retried successfully.)

## 12. Other code changes

- Entry JSON: `/sdcard/CIRCE/ENTRIES/YYYYMMDD/{8-hex-id}.JSON` with `{id}.TMP` temp
- Entry IDs: 8 uppercase hex (FAT-safe, no UUID hyphens)
- Index: prefer existing `entry_index.jsonl` / `.dirty`; new default `ENTRY.IDX` / `DIRTY.TAG`
- Path join audit: `PATH_TOO_LONG` on truncated `snprintf`

## 13. Commit recommendation

**Do not commit yet** — probe PASS confirmed on hardware, but user should confirm Test Save, body+Green save, Review, and Delete on device. If all pass:

> `fix(storage): FAT-safe probe path LOGS/PROBE.TMP, staged probe, entry path audit`

## 14. Docs

- [STORAGE_PROBE_ERRNO_22.md](bugs/STORAGE_PROBE_ERRNO_22.md) — root cause + fix detail
- [STORAGE_READY_MISMATCH.md](bugs/STORAGE_READY_MISMATCH.md) — updated follow-on note
- [PHASE-STORAGE-REPAIR-REPORT.md](PHASE-STORAGE-REPAIR-REPORT.md) — prior phase (superseded for probe path)
