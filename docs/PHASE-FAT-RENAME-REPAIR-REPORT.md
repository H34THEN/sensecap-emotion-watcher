# Phase FAT-Rename-Repair Report

**Date:** 2026-06-24  
**Hardware:** SenseCAP Watcher `/dev/ttyACM1`  
**Scope:** Fix JSON atomic commit rename failure on FAT32  
**Feature freeze:** maintained

---

## 1. Root cause

Single `rename({id}.TMP → {id}.JSON)` failed on ESP-IDF FAT/VFS without logging errno. No remove-final retry, no direct-write fallback. Temp was deleted on failure, losing recoverable data.

## 2–3. Paths before fix

| | Path |
|---|------|
| Temp | `/sdcard/CIRCE/ENTRIES/19700101/{id}.TMP` |
| Final | `/sdcard/CIRCE/ENTRIES/19700101/{id}.JSON` |

## 4. errno from rename

**Not captured before fix.** After fix, serial logs `commit rename FAIL rc=... errno=... (strerror)`.

## 5. Date directory missing?

**Unlikely** — temp write succeeded, so `19700101/` existed or was created. Added explicit `ensure_entry_dir()` stat/mkdir logging.

## 6. Temp filename invalid?

**No** — `{id}.TMP` is valid 8.3. Issue is rename/commit semantics, not temp naming.

## 7. Final commit strategy

Staged pipeline (first success wins):

1. `atomic_rename` — rename temp → final
2. `remove_rename` — unlink final if exists, rename again
3. `direct_write` — write final directly, unlink temp (`JSON_OK_NON_ATOMIC`)

## 8–12. Hardware validation

| Test | Status |
|------|--------|
| Test Save | **Pending user** — flash applied |
| Body + Green Save | **Pending user** |
| Review / Delete / Reboot | **Pending user** |

## 13. Build

**PASS** — 700,272 B (`0xab370`)

## 14. Flash

**PASS** — `idf.py --port /dev/ttyACM1 -b 2000000 app-flash` @ `0x110000`

## 15. Remaining bugs

1. UI validation pending  
2. RTC unset → `19700101` folders  

## 16. Commit recommendation

Superseded by FAT-Extension-Repair (`.JSN` extension).

---

## Follow-on: `.JSON` extension EINVAL

Rename and direct-write to `.JSON` both failed errno=22. Fix: use `.JSN`. See [FAT_EXTENSION_EINVAL.md](bugs/FAT_EXTENSION_EINVAL.md).
