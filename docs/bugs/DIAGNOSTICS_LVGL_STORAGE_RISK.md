# Diagnostics LVGL Storage Risk

## Risk

`circe_storage_health_check()` performs filesystem work:

- `esp_vfs_fat_info("/sdcard", …)`
- `dir_exists()` on entries/index paths
- `circe_index_count()` (index file read)

Previously this ran inside `circe_ui_show_step(CIRCE_FLOW_DIAGNOSTICS)` on the **LVGL task**, same class of risk as save self-test before the worker task existed.

## Symptom pattern

Not as heavy as full save self-test, but still:

- SD/VFS calls on UI thread
- Potential stack pressure during screen rebuild
- Inconsistent with worker-first storage policy

## Fix (2026-06-24)

Diagnostics screen now:

1. Shows `checking storage...` immediately on LVGL thread.
2. Enqueues `CIRCE_WORKER_HEALTH_CHECK` (or refresh variants).
3. Worker runs `circe_storage_health_check()` off LVGL.
4. Result delivered via `lv_async_call` → updates feed line safely.

Added **RUN PROBE** button → existing `CIRCE_WORKER_STORAGE_PROBE` (already worker-backed).

## LVGL callbacks (allowed)

- Enqueue worker command
- Set subline: `Checking...`, `Probing...`, `Running test...`
- Show placeholder feed text

## Still on worker (unchanged)

- Test Save / Self Test
- Rebuild Index
- Reinit Storage
- Storage probe
- Save / Delete / Review load

## Related

- [LVGL_STACK_OVERFLOW_STORAGE_IO.md](LVGL_STACK_OVERFLOW_STORAGE_IO.md)
- [WORKER_STACK_OVERFLOW_STORAGE_SELFTEST.md](WORKER_STACK_OVERFLOW_STORAGE_SELFTEST.md)
