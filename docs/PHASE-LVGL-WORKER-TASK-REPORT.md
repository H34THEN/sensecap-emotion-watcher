# Phase LVGL-Worker-Task Report

**Date:** 2026-06-24  
**Hardware:** SenseCAP Watcher `/dev/ttyACM1`  
**Scope:** Move blocking storage I/O out of LVGL callbacks into worker task  
**Feature freeze:** maintained

---

## 1. Root cause of LVGL stack overflow

Save self-test (and other save/delete/index operations) ran **synchronously inside `btn_event_cb()`** on the LVGL task. Deep call stack + 4 KB JSON buffers exceeded the LVGL task stack → `stack overflow in task LVGL task` → reboot.

## 2. Callbacks that were doing file I/O

| UI action | Old call (in LVGL task) |
|-----------|-------------------------|
| Test Save / Self test | `circe_storage_run_save_self_test()` |
| Save / Skip color / Color pick / Quick save | `circe_save_entry()` via `persist_entry()` |
| Review | `circe_storage_rebuild_index_if_dirty()`, `circe_entry_load()` |
| Delete confirm | `circe_entry_delete_hard()` |
| Rebuild index | `circe_rebuild_index_from_json()` |
| Reinitialize storage | `circe_storage_reinit()` |

## 3. Worker task implementation

- **Files:** `circe_worker.c`, `circe_worker.h`
- **Task name:** `circe_worker`
- **Queue depth:** 6 commands
- **Single-flight:** `s_busy` rejects overlapping ops with “Please wait...”
- **Commands:** TEST_SAVE, SAVE_ENTRY, DELETE_ENTRY, REBUILD_INDEX, STORAGE_PROBE, REINIT_STORAGE, LOAD_REVIEW

## 4. Worker stack size

**12288 bytes** (`CIRCE_WORKER_STACK`)

## 5. UI async update mechanism

1. LVGL callback enqueues command + sets subline (`Running test...`, `Saving...`).
2. Worker completes → `malloc` completion struct → **`lv_async_call(async_deliver, copy)`**.
3. `circe_ui_worker_done()` runs on LVGL thread: updates HUD, `go_step()`, strand cache, review draft.

JSON-first semantics preserved: worker calls `circe_save_entry_report()` unchanged.

## 6–10. Hardware validation

| Test | Status |
|------|--------|
| Boot probe PASS | **PASS** (serial) |
| Worker task start | **PASS** (serial: `worker task started stack=12288`) |
| Test Save (no reboot) | **Pending user tap** |
| Body + color save | **Pending user** |
| Review / Delete | **Pending user** |
| Reboot persistence | **Pending user** |

Agent cannot tap Watcher UI; flash + boot log only.

## 11. Build result

**PASS** — 694,160 B (`0xa9790`)

## 12. Flash result

**PASS** — `idf.py --port /dev/ttyACM1 -b 2000000 app-flash` @ `0x110000`

## 13. Remaining bugs

1. UI flow validation pending (Test Save regression, save/review/delete)  
2. RTC unset → `local_date` may be `19700101` until time sync  
3. Strand screen still reads SD inside `circe_ui_show_step()` (lower risk)  

## 14. Commit recommendation

**Do not commit** until user confirms Test Save does not reboot and save/review/delete paths work.

Suggested message:

> `fix(ui): offload storage I/O to circe_worker task; lv_async_call for UI updates`
