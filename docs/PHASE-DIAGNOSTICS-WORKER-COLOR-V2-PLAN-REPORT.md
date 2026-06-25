# Phase Diagnostics Worker + Color Picker v2 Plan Report

## Part 1 — Diagnostics worker safety

### Moved to worker

- `circe_storage_health_check()` — no longer called from `circe_ui_show_step(CIRCE_FLOW_DIAGNOSTICS)`

### Worker commands added

| Command | Handler |
|---------|---------|
| `CIRCE_WORKER_HEALTH_CHECK` | `run_health_check()` |
| `CIRCE_WORKER_STORAGE_STATUS` | same (alias) |
| `CIRCE_WORKER_DIAGNOSTICS_REFRESH` | same (alias) |

Completion includes `circe_storage_health_t health` in `circe_worker_completion_t`.

### LVGL cleanup

- **Removed:** synchronous `circe_storage_health_check(&h)` in diagnostics `show_step`
- **Added:** `checking storage...` placeholder + `post_health_check()` enqueue
- **Added:** `RUN PROBE` → `CIRCE_WORKER_STORAGE_PROBE` (was worker-backed but had no UI button)
- **Completion handler:** health types update feed via `diagnostics_apply_health()` on LVGL async thread

### Validation (on-device)

After app-flash:

1. Settings → Diagnostics — shows checking, then `ready:… entries:… probe:…`
2. RUN PROBE / TEST SAVE / SELF TEST — no reboot
3. Save / Review / Delete — no regression expected

## Part 2 — Color Picker v2 (design only)

Created `docs/color/COLOR_PICKER_V2_TOUCH_FIELD.md`:

- Touch-drag circular field + magnifier + live hex
- Encoder fine-tune; preset fallback (CALM … CUSTOM)
- HSV math preferred over large bitmap gradient
- LVGL performance and worker boundary documented

## Build

**PASS** — `circe.bin` `0xafb30`

## Flash

Run: `idf.py --port /dev/ttyACM1 -b 2000000 app-flash` (close serial monitor if port busy)

## Commit

```
stabilize diagnostics worker flow and plan touch color picker v2
```

## Recommended next phase

Implement Color Picker v2 touch field (single screen, preset fallback preserved).
