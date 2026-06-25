# Terminal Feed NULL Cursor Boot Loop

## Symptom

Immediate reboot after UI init. Guru Meditation in LVGL:

```
lv_obj_has_flag_any()
lv_obj_is_layout_positioned()
lv_obj_add_flag()
circe_terminal_feed_show_cursor()
circe_terminal_feed_clear()
clear_content()
circe_ui_show_step()
```

`EXCVADDR = 0x0000001c` — NULL pointer dereference on cursor object.

## Root Cause

`circe_ui_show_step()` always calls `clear_content()` first, which called `circe_terminal_feed_clear()`.

On first boot (and after any destroy), `s_feed` had never been through `circe_terminal_feed_init()`. All widget pointers were NULL.

`circe_terminal_feed_show_cursor()` checked `feed` but not `feed->cursor`, then called `lv_obj_add_flag(feed->cursor, ...)` on NULL.

## Fix

1. **`circe_terminal_feed_destroy()`** — stop cursor timer, delete panel, zero struct.
2. **`clear_content()`** — call `destroy` instead of `clear` (safe when never created).
3. **NULL guards** on every LVGL call in feed init/clear/set/show_cursor.
4. **`feed_init()`** — always destroys any prior feed before creating widgets.
5. **Diagnostic logging** — pointer addresses logged at create/destroy/cursor show/hide.

## Build Order (enforced)

1. create parent (viewport)
2. `feed_init` → panel → lines → cursor
3. `feed_set` / `feed_show_cursor`

Never show cursor on an uninitialized or destroyed feed.

## Files

- `firmware/circe/main/circe_terminal.c`
- `firmware/circe/main/circe_ui.c` (`clear_content`)
