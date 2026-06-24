# Bug: FAT Extension EINVAL — `.JSON` Rejected

**Date:** 2026-06-24  
**Phase:** FAT-Extension-Repair  
**Hardware:** SenseCAP Watcher, FAT32 SD  

---

## Symptom

After worker stack and rename-repair fixes:

- Probe `PROBE.TMP` — **OK**
- Temp entry write `{id}.TMP` — **OK**
- Rename `{id}.TMP` → `{id}.JSON` — **FAIL errno=22**
- Remove+rename — **FAIL errno=22**
- Direct write `{id}.JSON` — **FAIL errno=22**

No reboot. SD mounted. Directories exist.

---

## Root cause

**ESP-IDF FAT/VFS rejects `.JSON` as a final filename/extension** on this hardware configuration. `.TMP` (3-char 8.3) works; `.JSON` (4-char extension) returns **EINVAL** for both `rename()` and `fopen(w)`.

Not SD format, not missing dirs, not stack overflow.

---

## Fix

Use **8.3-compatible `.JSN`** for new entry files:

| Role | Path |
|------|------|
| Temp | `/sdcard/CIRCE/ENTRIES/YYYYMMDD/{id}.TMP` |
| Final | `/sdcard/CIRCE/ENTRIES/YYYYMMDD/{id}.JSN` |

Legacy load/review scans (in order): `.JSN`, `.JSON`, `.json`, `.DAT`, bare id.

Boot storage probe runs **fatname compatibility probe** in `LOGS/` testing:

- `PROBE.TMP`, `PROBE.JSN`, `PROBE.JSON`, `PROBE.DAT`, `PROBE` (no ext)

Logs direct `fopen`, `rename`, `unlink` results and errno for each.

---

## Expected hardware log

```
fatname 'PROBE.TMP' direct_fopen=OK ...
fatname 'PROBE.JSN' direct_fopen=OK rename=OK ...
fatname 'PROBE.JSON' direct_fopen=FAIL errno=22 (Invalid argument)
save self-test: JSON OK  INDEX OK/WARN  LOAD OK  DEL OK
```

---

## Files changed

- `circe_storage_paths.h` — `CIRCE_ENTRY_FILE_EXT ".JSN"`
- `circe_storage.c` — entry paths, scan/load compat, fatname probe

---

## Commit recommendation

**Do not commit** until Test Save and body save pass on hardware.
