# Bug: Worker Stack Overflow During Timeline Category Load

**Date:** 2026-06-24  
**Symptom:** Reboot when opening REVIEW → TODAY / YESTERDAY / THIS WEEK / ALL ENTRIES.

```
I circe_worker: run type=7
I circe_worker: worker stack before: 6376 words free
***ERROR*** A stack overflow in task circe_worker has been detected.
```

Worker command type 7 = `CIRCE_WORKER_LOAD_TIMELINE`.

---

## Root cause

`circe_timeline_load_category()` still used the original heavy browse path:

- Index rows allocated on the **worker stack** (`rows[16]`)
- Up to 16× **`circe_entry_load()`** — full `circe_entry_t` + complete cJSON parse per entry
- Nested on top of `circe_index_list_collect()` frames → exceeded 16 KB worker stack

The lightweight `item_from_json_path()` loader existed for pattern reflection / pattern scan but was not wired into timeline category browse.

---

## Fix

1. **Heap index rows** in `circe_timeline_load_category()` via `circe_buf_alloc`.
2. **Lightweight item load** — replace `circe_entry_load()` with `item_from_json_path()` (minimal field extraction only).
3. **Parse body sensations** in `item_from_json_path()` for browse display lines.

Same strategy as `docs/bugs/WORKER_STACK_OVERFLOW_PATTERN_REFLECTION.md`.

---

## Files changed

- `circe_timeline.c`

---

## Expected after fix

REVIEW → time period loads browse screen without worker stack overflow. VIEW / DELETE still use full `circe_entry_load()` for a single entry (acceptable).
