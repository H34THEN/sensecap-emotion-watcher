# Save Repair Report

**Date:** 2026-06-24  
**Phase:** Save-Repair + Centered UI Pass  
**Hardware:** SenseCAP Watcher `/dev/ttyACM1`

---

## Problem

Users still saw *"Couldn't save"* after body + color flow, even after Function-Stabilization added error codes. Saves felt broken; Review often showed no entries.

---

## Root cause (continued failure)

**Primary:** Index append failure was treated as **fatal save failure**, even when the canonical JSON file was written successfully.

Flow before fix:

1. `circe_entry_save_json_atomic()` writes `/sdcard/circe/entries/YYYY-MM-DD/{id}.json` ✓  
2. `circe_entry_index_insert()` appends to `entry_index.jsonl` ✗  
3. UI shows *"Couldn't save"* — user believes nothing was saved  
4. Review uses index only → entry invisible  

**Secondary:** Review and load paths had **no JSON directory fallback** when index was missing, dirty, or corrupt.

**Not the root cause:** Color hex values (`#68D391`, `#9B59B6`, etc.) — these were always valid once button labels were fixed.

---

## Fix summary

| Change | File(s) |
|--------|---------|
| JSON write success = user-visible save success | `circe_save.c`, `circe_ui.c` |
| Index append is best-effort; failure → `CIRCE_SAVE_OK_INDEX_WARN` | `circe_save.c` |
| `circe_index_mark_dirty()` + `.dirty` marker on SD | `circe_index.c` |
| `circe_index_append_best_effort()` | `circe_index.c` |
| Rebuild index on boot if dirty | `circe_storage.c` |
| Review scans `/sdcard/circe/entries/` when index fails | `circe_storage.c` |
| Load/delete find JSON by ID without index | `circe_storage.c` |
| **Test Save** diagnostic (JSON / INDEX / LOAD / DEL) | `circe_storage.c`, `circe_ui.c` |
| On-screen error: `Save failed:\nJSON_WRITE_FAILED` | `circe_ui.c` |
| Index warn message: *Saved. Index will rebuild.* | `circe_ui.c` |
| Serial proof logging on every save attempt | `circe_save.c` |

---

## New save semantics

```
JSON OK + Index OK     → CIRCE_SAVE_OK
JSON OK + Index FAIL   → CIRCE_SAVE_OK_INDEX_WARN (user sees saved)
JSON FAIL              → fatal (STORAGE_NOT_READY / JSON_WRITE_FAILED / INVALID_ENTRY_STATE)
```

Canonical source of truth: **JSON files on SD**.

Index is a cache for fast Review — rebuildable from JSON at any time.

---

## Test Save diagnostic

**More → Storage → Test Save**

Runs:

1. Create temp quick entry  
2. Write JSON  
3. Index best-effort  
4. Load back (index or path fallback)  
5. Delete  

Displays: `JSON OK  INDEX OK/WARN  LOAD OK  DEL OK`

---

## Serial log example (successful save, index warn)

```
I circe_save: save attempt storage_ready=yes base=/sdcard/circe json_path=/sdcard/circe/entries/2026-06-24/{id}.json ...
I circe_save: save result json=OK index=WARN user=OK_INDEX_WARN
W circe_index: index marked dirty
W circe_save: save json ok, index warn id={id} color=#68D391
```

---

## Hardware validation

| Test | Agent | User confirm |
|------|-------|--------------|
| Boot + SD mount | ✓ serial | — |
| Test Save button | — | **Required** |
| Body + color save | — | **Required** |
| Review after save | — | **Required** |
| Reboot persistence | — | **Required** |

Agent cannot tap Watcher touch screen. **Do not treat save as fixed until user confirms on device.**

---

## Storage-Repair follow-up (2026-06-24)

If user still saw `STORAGE_NOT_READY` after index-first save fix, root cause was **`access("/sdcard", W_OK)` false negative** in save readiness — see [STORAGE_READY_MISMATCH.md](STORAGE_READY_MISMATCH.md).
