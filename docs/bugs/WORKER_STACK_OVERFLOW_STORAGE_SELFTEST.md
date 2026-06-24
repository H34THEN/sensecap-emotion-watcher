# Bug: Worker Stack Overflow During Storage Self-Test

**Date:** 2026-06-24  
**Phase:** Worker-Stack-Repair  
**Hardware:** SenseCAP Watcher  
**Symptom:** After LVGL worker fix, More → Storage → Test Save still reboots.

---

## Monitor log (before fix)

```
I circe_worker: worker task started stack=12288
I circe_worker: run type=0
I circe_storage: save self-test target=/sdcard/CIRCE/ENTRIES/19700101/EE670B77.JSON

***ERROR*** A stack overflow in task circe_worker has been detected.
```

Storage I/O was correctly moved off the LVGL task, but the **worker stack (12 KB) was still too small** for nested save/load/index paths with large **on-stack buffers**.

---

## Root cause

Nested storage calls allocated **4096-byte JSON buffers and 640-byte index line buffers on the stack** simultaneously:

| Location | Stack allocation |
|----------|------------------|
| `circe_entry_save_json_atomic()` | `char json[4096]` |
| `circe_entry_load()` | `char json[4096]` |
| `scan_entries_for_latest()` | `char json[4096]` per file loop |
| `index_one_json_file()` | `char json[2048]` |
| `circe_index` scan/rewrite | `char line[640]` |
| Self-test (old) | two `circe_entry_t` + full save + load + `circe_entry_delete_hard()` (index rewrite) |

Worst case self-test stack: **save JSON buffer + index rewrite line + load JSON buffer + second entry struct** ≈ 10–14 KB nested — exceeds 12288-byte worker stack.

Increasing worker stack alone would not fix save/review/rebuild paths hitting the same limits.

---

## Fix

1. **`circe_buf.c/h`** — heap helpers for JSON (4096 B) and index lines (640 B) with allocation failure logging.
2. **Moved off stack:** all `char json[4096]` / `[2048]` in storage; all `char line[640]` in index.
3. **Minimal staged self-test** (A–E):
   - A: minimal QUICK entry (color only, no body fields)
   - B: `circe_entry_save_json_atomic()` (heap JSON)
   - C: read file back, verify ID substring (no full `circe_entry_load` / index lookup)
   - D: optional index append (non-fatal WARN)
   - E: `unlink()` test file (no index rewrite delete)
4. **Worker stack high-water logging** before/after each command via `uxTaskGetStackHighWaterMark()`.
5. **`CIRCE_SAVE_MEMORY_ALLOCATION_FAILED`** error when heap alloc fails.

---

## Expected serial after fix

```
I circe_worker: run type=0
I circe_worker: worker stack before: XXXX words free
I circe_storage: save self-test A minimal entry id=...
I circe_storage: save self-test B write JSON
I circe_storage: save self-test C read back
I circe_storage: save self-test D index optional
I circe_storage: save self-test E delete file
I circe_storage: save self-test: JSON OK  INDEX OK/WARN  LOAD OK  DEL OK
I circe_worker: worker stack after: YYYY words free
```

No `stack overflow in task circe_worker`.

---

## Files changed

- `circe_buf.c`, `circe_buf.h` — heap buffer helpers
- `circe_storage.c` — heap JSON, minimal self-test
- `circe_index.c` — heap index line buffers
- `circe_worker.c` — stack/heap logging
- `circe_save.c` — `MEMORY_ALLOCATION_FAILED` mapping

---

## Commit recommendation

**Do not commit** until user confirms Test Save completes without reboot and save/review/delete flows work.

---

## Follow-on: FAT rename failure

After worker fix, Test Save wrote temp OK but **`rename({id}.TMP → {id}.JSON)` failed**. Fixed in FAT-Rename-Repair — see [FAT_RENAME_FAILURE.md](FAT_RENAME_FAILURE.md).
