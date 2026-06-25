# Bug: LVGL Stack Overflow During Storage I/O

**Date:** 2026-06-24  
**Phase:** LVGL-Worker-Task Fix  
**Hardware:** SenseCAP Watcher  
**Symptom:** More → Storage → Test Save causes black screen / reboot.

---

## Monitor log (before fix)

```
I circe_storage: save self-test target=/sdcard/CIRCE/ENTRIES/19700101/531CC038.JSON

***ERROR*** A stack overflow in task LVGL task has been detected.
```

Boot storage probe (`LOGS/PROBE.TMP`) passed. Failure occurred only when running the save self-test from the Storage diagnostics UI.

---

## Root cause

**Blocking SD/file I/O ran inside the LVGL task** (button event callback chain).

`btn_event_cb()` called `circe_storage_run_save_self_test()` synchronously. That function allocates large stack frames (4 KB JSON buffers, entry structs, index rewrite paths, multiple `fopen`/`stat` calls). The LVGL task stack is sized for UI rendering, not deep storage work — overflow and reboot.

Same pattern affected:

- Test Save / Run self test
- Save Entry (body, color, quick flows)
- Delete Entry
- Rebuild Index
- Reinitialize Storage
- Review (index rebuild + load latest entry)

---

## Fix

Introduced **`circe_worker` task** (FreeRTOS queue + dedicated stack):

1. LVGL callbacks only enqueue a command and show immediate status (`Running test...`, `Saving...`, etc.).
2. Worker runs all blocking storage/JSON/index operations off the LVGL thread.
3. Completion delivered via **`lv_async_call()`** → UI handler updates HUD and navigates safely on LVGL thread.

Worker stack: **12288 bytes**. LVGL stack unchanged (not used as primary fix).

---

## Verification

After flash, More → Storage → Test Save must **not** reboot. Serial must **not** show `stack overflow in task LVGL task`. Worker log should show:

```
I circe_worker: run type=0
I circe_storage: save self-test: JSON OK ...
```

---

## Files changed

- `firmware/circe/main/circe_worker.c` — worker task + queue
- `firmware/circe/main/circe_worker.h` — command/completion types
- `firmware/circe/main/circe_ui.c` — enqueue only from callbacks; `lv_async_call` completion handler
- `firmware/circe/main/CMakeLists.txt` — build worker

---

## Remaining risk

~~`CIRCE_FLOW_DIAGNOSTICS` screen still called `circe_storage_health_check()` inside `circe_ui_show_step()`~~ — **fixed 2026-06-24** (Diagnostics Worker Safety phase). Health check now runs on `circe_worker`. See [DIAGNOSTICS_LVGL_STORAGE_RISK.md](DIAGNOSTICS_LVGL_STORAGE_RISK.md).

`CIRCE_FLOW_STRAND` screen still loads strand colors via `circe_storage_today_strand()` inside `circe_ui_show_step()` (Strand remains disabled in UI). Move to worker if strand navigation is re-enabled.

---

## Follow-on: worker stack overflow (2026-06-24)

Moving I/O to `circe_worker` fixed the LVGL crash, but Test Save then overflowed **`circe_worker` (12288 B)** because save/load/index still used **4096 B JSON buffers on the stack**. Fixed in Worker-Stack-Repair phase — heap buffers via `circe_buf.c`, minimal staged self-test. See [WORKER_STACK_OVERFLOW_STORAGE_SELFTEST.md](WORKER_STACK_OVERFLOW_STORAGE_SELFTEST.md).

---

## Commit recommendation

**Do not commit** until user confirms Test Save does not reboot and body+color save / Review / Delete work on hardware.
