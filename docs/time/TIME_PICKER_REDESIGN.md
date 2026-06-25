# Time Picker Redesign

## Problem

Settings → More → TIME used LVGL slider bars. On the circular Watcher display these are unreadable and unusable.

## Solution

Replace sliders with a **rotary number picker** — plain text rows, one highlighted field at a time.

## Layout

```
CIRCE
time setup

> YEAR   2026 _
  MONTH  06
  DAY    24
  HOUR   19
  MIN    42

PRESS NEXT  HOLD SAVE
```

## Controls

| Input | Action |
|-------|--------|
| Rotate CW | Increase selected field |
| Rotate CCW | Decrease selected field |
| Press | Next field (wraps MIN → YEAR) |
| Double press | Back to Settings |
| Long press (800 ms) | Save via `circe_time_apply()` |

## Ranges

| Field | Range | Wrap |
|-------|-------|------|
| YEAR | 2020–2037 | yes |
| MONTH | 01–12 | yes |
| DAY | 01–31 (clamped to month) | yes |
| HOUR | 00–23 | yes |
| MIN | 00–59 | yes |

## Module

`circe_time_picker.c` — UI labels + encoder poll. Backend unchanged (`circe_time.c`, NVS, storage paths).

## Init values

- If time SET → current system time
- If unset → default `2026-01-01 12:00`

## Removed

- `CIRCE_FLOW_TIME_EDIT_DATE` / `CIRCE_FLOW_TIME_EDIT_TIME` slider screens (redirect to unified picker)
- All `lv_slider_create` calls for date/time in `circe_ui.c`
