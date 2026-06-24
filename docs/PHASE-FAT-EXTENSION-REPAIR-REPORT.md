# Phase FAT-Extension-Repair Report

**Date:** 2026-06-24  
**Hardware:** SenseCAP Watcher `/dev/ttyACM1`  
**Scope:** Replace `.JSON` with FAT-safe `.JSN`; filename probe  
**Feature freeze:** maintained  

---

## 1. Was `.JSON` the cause?

**Yes (high confidence).** `.TMP` works; `.JSON` fails with EINVAL on rename and direct fopen. `.JSN` is 8.3-compatible and chosen for all new writes.

## 2. Filename compatibility probe

Added `run_fat_filename_extension_probe()` during storage probe — tests `PROBE.TMP`, `PROBE.JSN`, `PROBE.JSON`, `PROBE.DAT`, `PROBE` in `LOGS/`. Serial logs `direct_fopen`, `rename`, `delete`, errno per case.

## 3. Final extension chosen

**`.JSN`** (`CIRCE_ENTRY_FILE_EXT`)

## 4. New entry path format

`/sdcard/CIRCE/ENTRIES/YYYYMMDD/{8-hex-id}.JSN`  
Temp: `{id}.TMP`

## 5–8. Hardware validation

| Test | Status |
|------|--------|
| Test Save | **Pending user** |
| Body + Green Save | **Pending user** |
| Review / Delete | **Pending user** |

## 9. Build

**PASS** — 702,720 B (`0xab900`)

## 10. Flash

**PASS** — `idf.py --port /dev/ttyACM1 -b 2000000 app-flash` @ `0x110000`

## 11. Remaining bugs

1. UI validation pending  
2. RTC unset → `19700101` folders  
3. Legacy `.JSON` files on card still readable via compat scan  

## 12. Commit recommendation

**Do not commit** until Test Save shows JSON OK, LOAD OK, DEL OK.

```
fix(storage): use .JSN entry extension; FAT filename probe; legacy .JSON compat
```
