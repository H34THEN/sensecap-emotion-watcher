# Bug: Worker Stack Overflow During Pattern Reflection Load

**Date:** 2026-06-25  
**Symptom:** Reboot after save when post-save pattern reflection runs.

```
I circe_save: save ok id=B488E87C color=#648096
***ERROR*** A stack overflow in task circe_worker has been detected.
--- parse_object at cJSON.c
```

---

## Root cause

After a successful save, `run_save_entry()` called `circe_reflection_load_recent_context()` **before returning**, while the save path still held large stack frames (`circe_entry_t`, save JSON work).

That loader reused full timeline browse logic:

- `circe_timeline_load_category()` with 16× `circe_entry_load()` (full `circe_entry_from_json` / cJSON parse)
- Nested on top of the save stack → exceeded 12 KB worker stack

---

## Fix

1. **Defer pattern load** until after `run_save_entry()` returns (`load_reflection_context_after_save()` in worker switch).
2. **Lightweight loader** — `circe_timeline_load_pattern_context()` reads only pattern fields via minimal cJSON (no full `circe_entry_t`), max **10** entries, index rows on heap.
3. **Worker stack** increased 12288 → **16384** bytes as margin.

---

## Files changed

- `circe_worker.c`
- `circe_timeline.c/h`
- `circe_reflection.c`

---

## Expected after fix

Save completes, pattern context loads, reflection screen appears — no worker stack overflow.
