# Bug: FAT Rename Failure on JSON Commit

**Date:** 2026-06-24  
**Phase:** FAT-Rename-Repair  
**Hardware:** SenseCAP Watcher, FAT32 SD  
**Symptom:** Test Save no longer reboots, but JSON write fails at `rename`.

---

## Hardware log (before fix)

```
I circe_storage: save self-test B write JSON
E circe_storage: rename failed
E circe_storage: save self-test: JSON FAIL  INDEX -  LOAD -  DEL -
```

Probe PASS. Directories exist. Temp write succeeded; **atomic rename temp → final failed** with no errno logged.

---

## Paths involved

| Role | Path |
|------|------|
| Temp (before fix) | `/sdcard/CIRCE/ENTRIES/19700101/{id}.TMP` |
| Final | `/sdcard/CIRCE/ENTRIES/19700101/{id}.JSON` |

Both are FAT 8.3 uppercase. Temp naming was already correct (`{id}.TMP`, not double extension).

---

## Root cause (likely)

ESP-IDF FAT/VFS **`rename()` from `.TMP` to `.JSON` fails** on this stack even when:

- Parent dir exists (or is created)
- Temp file is fully written, flushed, and closed
- Final file does not exist

Typical failure modes on embedded FAT:

- Rename across different extensions not supported reliably
- Destination must be removed first on some implementations
- No fallback when rename unsupported

Previous code: single `rename()` attempt, **deleted temp on failure**, no errno detail, no fallback.

Date folder `19700101` (RTC unset) was created via `mkdir_p`; **not the primary failure** — temp write succeeded, proving dir existed.

---

## Fix

1. **Detailed rename logging** — temp/final paths, lengths, exists flags, `rename` rc, errno, `strerror`
2. **`ensure_entry_dir()`** — stat + mkdir with logging
3. **Staged commit:**
   - A: write `{id}.TMP`
   - B: `rename` temp → final
   - C: if fail and final exists → `unlink(final)` then retry rename
   - D: if still fail → **direct write to final** (MVP fallback), unlink temp
4. **Leave temp on rename failure** until fallback succeeds (recovery)
5. **Boot/save cleanup** — remove stale `*.TMP` under `ENTRIES/*/` (never entry data files)
6. **`CIRCE_SAVE_OK_NON_ATOMIC`** when direct-write fallback used

---

## Expected serial after fix

```
save_json dir exists /sdcard/CIRCE/ENTRIES/19700101
commit rename FAIL rc=-1 errno=... (...)
commit rename failed; fallback direct write final='...'
commit direct write OK final='...'
save_json commit=direct_write id=...
save self-test: JSON OK_NON_ATOMIC  INDEX OK/WARN  LOAD OK  DEL OK
```

Or if rename works:

```
commit rename OK temp='...' -> final='...'
save_json commit=atomic_rename
save self-test: JSON OK  INDEX OK/WARN  LOAD OK  DEL OK
```

---

## Files changed

- `firmware/circe/main/circe_storage.c` — commit pipeline, cleanup, logging
- `firmware/circe/main/circe_save.c` — `JSON_OK_NON_ATOMIC` result

---

## Commit recommendation

**Do not commit** until Test Save shows JSON OK (or OK_NON_ATOMIC), LOAD OK, DEL OK on hardware.

---

## Follow-on: `.JSON` extension EINVAL (2026-06-24)

Rename-repair still failed because **final path used `.JSON`** — ESP-IDF FAT/VFS returns EINVAL for both rename and direct fopen to `.JSON`, while `.TMP` works. Fixed in **FAT-Extension-Repair** by switching new entry files to **`.JSN`**. See [FAT_EXTENSION_EINVAL.md](FAT_EXTENSION_EINVAL.md).
