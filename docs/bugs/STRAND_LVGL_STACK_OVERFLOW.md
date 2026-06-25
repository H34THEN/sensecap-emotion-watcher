# Strand LVGL Stack Overflow

## Symptom

Selecting **Today Strand** from Storage diagnostics caused immediate reboot:

```
terminal_feed_destroy
terminal_feed_create
terminal_feed_cursor_create
***ERROR*** A stack overflow in task LVGL task has been detected.
```

## Root Cause

`CIRCE_FLOW_STRAND` ran inside `circe_ui_show_step()` on the **LVGL task**:

1. `setup_terminal_shell()` — full terminal feed destroy/create cycle.
2. `circe_storage_today_strand()` — SD/index scan with large on-stack buffers in the LVGL task context.

Combined UI rebuild + storage I/O exceeded the LVGL task stack (documented risk in prior phases).

## Fix (Phase UI-Stability)

**Strand disabled for now** — core save/review/delete is higher priority.

1. Removed **TODAY STRAND** button from diagnostics menu.
2. `CIRCE_FLOW_STRAND` shows static `strand unavailable` — **no SD reads**.
3. `strand` button handler sets subline only (no navigation).
4. Home arc strand blocks no longer created (`refresh_strand_arc_from_blocks` clears layer only).

Strand cache append on save is unchanged (storage-side, lightweight); only the visual arc is suppressed.

## Future

If Strand returns, load data via `circe_worker` and apply UI with `lv_async_call` — never scan SD inside `show_step()` or button callbacks.
