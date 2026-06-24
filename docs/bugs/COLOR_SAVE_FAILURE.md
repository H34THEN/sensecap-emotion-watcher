# Bug: Color Save Failure (Body + Color Flow)

**Status:** Fixed (Function-Stabilization phase)  
**Severity:** Release-blocking  
**Reported flow:** Home → Ready → body area → sensation → intensity → color pick → save  
**Symptom:** HUD shows *"Couldn't save — try again?"* with no diagnostic detail

---

## Reproduction

### Expected steps

1. Home → **Ready**
2. Pick a body area (e.g. Chest)
3. Pick a sensation (e.g. Tight)
4. Set intensity → **Continue**
5. **Continue to save** (skip add-another)
6. Tap a color (Gray / Slate / Green / Purple)
7. Expect save confirmation; observed generic save error

### Logs before fix

Serial capture was not available during the original report. Code review traced failure to `persist_entry()` in `circe_ui.c`, which returned false from either:

- `circe_entry_save_json_atomic()` — JSON file write
- `circe_entry_index_insert()` — index append

Both paths surfaced the same copy key (`CIRCE_PATTERN_ERROR_SAVE_FAILED`) with **no error code or ESP log tag**.

### Logs after fix (boot baseline)

```
I (1935) circe_main: SD card mounted at /sdcard
I (2105) circe_storage: storage ready at /sdcard/circe
```

On save failure, firmware now emits:

```
E circe_ui: save error: INDEX_APPEND_FAILED detail=Index append failed
E circe_save: save failed: INDEX_APPEND_FAILED json written for id=<uuid>
```

(or `JSON_WRITE_FAILED`, `STORAGE_NOT_READY`, `INVALID_ENTRY_STATE`, etc.)

HUD subline shows: `Save failed (ERROR_CODE)` for field diagnosis without exposing private entry data.

---

## Root cause

**Primary:** The save pipeline lacked validation, structured error codes, and logging. Any failure in JSON write or index append produced the same user-visible message, making hardware diagnosis impossible.

**Contributing issues fixed in this phase:**

| Issue | Effect |
|-------|--------|
| No `storage_ready` check at save time | Saves attempted even when SD/index unavailable |
| Index insert always ran full index rewrite | Unnecessary SD rewrite before every new entry; rewrite failures were previously ignored, append could fail silently on busy SD |
| No JSON size guard | Oversized entries could truncate silently (buffer was 2048 B) |
| Color buttons labeled with raw `#RRGGBB` | LVGL treats `#` as recolor opcode — labels could render blank; confusing UX on color step |
| No color validation / fallback | Malformed `color_hex` from bad button id would fail JSON validation paths without recovery |

**Not the root cause:** Color hex values themselves (`#808080`, `#4A5568`, `#68D391`, `#9B59B6`) are valid. Skip-color and color-pick share the same save function; the bug was in the **save pipeline**, not color-specific JSON fields.

---

## Fix (updated Save-Repair phase)

### Function-Stabilization (first pass)

Central `circe_save` module, error codes, color label fix, index rewrite only for existing IDs.

### Save-Repair (second pass — addresses user-visible failure)

**Root cause:** Index append failure was fatal even when JSON was written. Review used index only.

| Change | Effect |
|--------|--------|
| `CIRCE_SAVE_OK_INDEX_WARN` | JSON OK + index fail = user sees **Saved** |
| `circe_index_append_best_effort()` | Marks index dirty; does not block save |
| Review JSON directory scan | Finds entries without index |
| Test Save diagnostic | Isolates JSON vs index vs load vs delete |
| On-screen `Save failed:\nCODE` | Clear fatal errors only |

See [SAVE_REPAIR_REPORT.md](SAVE_REPAIR_REPORT.md) and [STORAGE_READY_MISMATCH.md](STORAGE_READY_MISMATCH.md).

**Storage-Repair update:** False `STORAGE_NOT_READY` was caused by `access("/sdcard", W_OK)` in save layer — fixed with write probe + path resolver.

---

## Color fallback behavior

If color validation fails at button handler or pre-save:

1. Log warning with invalid value
2. Set `color_hex = "#808080"`
3. Continue save (unless user navigates away)

User is **not** blocked by a bad color picker id.

---

## Verification checklist

| Flow | Expected | Hardware retest |
|------|----------|-----------------|
| Quick save | Preset → saved | **User confirm** |
| Body-only (Skip color) | Saved with `#808080` | **User confirm** |
| Body + color | Saved with chosen hex | **User confirm** |
| Review | Opens latest entry | **User confirm** |
| Delete | Removes entry + index row | **User confirm** |
| Reboot persistence | Entries survive reset | **User confirm** |

Automated serial validation captured boot + storage-ready only. Touch flows require on-device retest after flash.

---

## Related files

- `firmware/circe/main/circe_save.c`
- `firmware/circe/main/circe_ui.c` — `persist_entry()`, color handler
- `firmware/circe/main/circe_index.c` — `circe_index_insert()`
- `firmware/circe/main/circe_storage.c` — atomic JSON write
