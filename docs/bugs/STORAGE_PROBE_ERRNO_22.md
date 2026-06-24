# Bug: Storage Probe fopen EINVAL (errno=22)

**Date:** 2026-06-24  
**Phase:** Storage-Probe-EINVAL Fix  
**Hardware:** SenseCAP Watcher, FAT32 SD card  
**Symptom:** Boot logs show SD mounted and CIRCE dirs found, then `probe fail: probe write open fail errno=22` and `init fail: probe failed`.

---

## Hardware log (before fix)

```
SD card mounted at /sdcard
resolved base from existing: /sdcard/CIRCE
using existing dir /sdcard/CIRCE/ENTRIES
using existing dir /sdcard/CIRCE/INDEX
using existing dir /sdcard/CIRCE/CACHE
using existing dir /sdcard/CIRCE/LOGS
probe fail: probe write open fail errno=22
init fail: probe failed
```

Card was FAT32, fsck-clean, reseated, directories present, mount OK. Failure was specifically on **opening the probe temp file**, not on mount or mkdir.

---

## Failing path and mode (before fix)

| Field | Value |
|-------|--------|
| Probe path | `{base}/storage_probe.tmp` → `/sdcard/CIRCE/storage_probe.tmp` |
| fopen mode | `"w"` |
| errno | `22` (`EINVAL`) |
| stat on dirs | Passed (mount + CIRCE tree OK) |

The probe file lived at the **base directory** with a lowercase multi-segment name (`storage_probe.tmp`). On ESP-IDF FAT/VFS this combination is a plausible EINVAL source: path/mode validation, hidden-ish dot segment, or VFS fopen quirks on certain FAT layouts.

---

## Root cause

**Invalid probe file location and naming for FAT32 + ESP-IDF VFS**, not missing SD or exFAT.

Contributing factors:

1. Probe file at `{base}/storage_probe.tmp` instead of a known-writable subdir (`LOGS/`).
2. Lowercase long probe name with embedded dot at base level.
3. Insufficient logging — failure did not print exact path, strlen, or mode before fix.

Directories existed and `stat()` succeeded; only **file create/open** failed with EINVAL.

---

## Fix

1. **Probe path:** `/sdcard/CIRCE/LOGS/PROBE.TMP` (8.3-style, uppercase, no timestamp/UUID/colons).
2. **Staged probe (A–J):** stat `/sdcard`, base, LOGS; then fopen `"w"` with full logging (path, len, mode, errno); fallback to `open(O_WRONLY|O_CREAT|O_TRUNC, 0666)`; fputs/fflush/fclose; fopen `"r"` or `open(O_RDONLY)`; read; unlink.
3. **Path audit:** `circe_storage_path_join()` checks `snprintf` return; logs `PATH_TOO_LONG` on truncation.
4. **User error for errno=22:** `Storage path invalid. Probe: <path>`.
5. **Entry saves:** `/sdcard/CIRCE/ENTRIES/YYYYMMDD/{8-hex-id}.JSON` with `{id}.TMP` temp files (FAT-safe).
6. **Index compatibility:** Prefer existing `entry_index.jsonl` / `.dirty`; new writes use `ENTRY.IDX` / `DIRTY.TAG` when absent.
7. **Entry IDs:** 8 uppercase hex chars (no UUID hyphens) for FAT-safe filenames.

---

## Expected log after fix

```
probe start base='/sdcard/CIRCE' logs='/sdcard/CIRCE/LOGS' path='/sdcard/CIRCE/LOGS/PROBE.TMP' len=28 mode_w='w'
probe A stat /sdcard OK
probe B stat base OK
probe C stat logs OK
probe D fopen(w) OK
probe E fputs OK
probe F/G fclose OK
probe H fopen(r) OK
probe I fread OK bytes=3
probe J remove OK
probe PASS write=OK read=OK del=OK path='/sdcard/CIRCE/LOGS/PROBE.TMP'
storage ready at /sdcard/CIRCE probe=PASS
```

If `fopen(w)` returns EINVAL but `open()` succeeds, serial logs: `fopen(w) EINVAL but open() succeeded` and probe still passes via POSIX path.

---

## Verification checklist

| Check | Status |
|-------|--------|
| Boot probe (serial) | **PASS** — `fopen(w)` OK on `/sdcard/CIRCE/LOGS/PROBE.TMP` |
| Storage ready (boot) | **YES** — `storage ready base=/sdcard/CIRCE` |
| More → Storage UI | **Pending user** |
| Test Save | **Pending user** |
| Body + Green save | **Pending user** |
| Review / Delete | **Pending user** |

### Hardware serial (2026-06-24, post-flash)

```
probe start base='/sdcard/CIRCE' logs='/sdcard/CIRCE/LOGS' path='/sdcard/CIRCE/LOGS/PROBE.TMP' len=28 mode_w='w'
probe A stat /sdcard OK
probe B stat base OK
probe C stat logs OK
probe D fopen(w) OK
probe PASS write=OK read=OK del=OK path='/sdcard/CIRCE/LOGS/PROBE.TMP'
storage ready base=/sdcard/CIRCE entries=/sdcard/CIRCE/ENTRIES index=/sdcard/CIRCE/INDEX
```

---

## Files changed

- `firmware/circe/main/circe_storage.c` — staged probe, entry path/save/scan fixes
- `firmware/circe/main/circe_storage_paths.c` — `PROBE.TMP`, path join audit, index/dirty resolution
- `firmware/circe/main/circe_entry.c` — 8-hex FAT-safe entry IDs
- `firmware/circe/main/circe_index.c` — `ENTRY.TMP` rewrite temp

---

## Commit recommendation

**Do not commit** until hardware confirms Probe PASS, Test Save (no LVGL stack overflow reboot), body+color save, Review, and Delete on the Watcher.

---

## Follow-on: LVGL stack overflow on Test Save

After probe PASS, More → Storage → Test Save rebooted with `stack overflow in task LVGL task` because save self-test ran inside LVGL button callback. Fixed in **LVGL-Worker-Task** phase — see [LVGL_STACK_OVERFLOW_STORAGE_IO.md](LVGL_STACK_OVERFLOW_STORAGE_IO.md).

Later, worker stack and FAT rename issues were fixed in subsequent phases — see [WORKER_STACK_OVERFLOW_STORAGE_SELFTEST.md](WORKER_STACK_OVERFLOW_STORAGE_SELFTEST.md) and [FAT_RENAME_FAILURE.md](FAT_RENAME_FAILURE.md).

Final entry extension fix: `.JSON` → `.JSN` — see [FAT_EXTENSION_EINVAL.md](FAT_EXTENSION_EINVAL.md).
