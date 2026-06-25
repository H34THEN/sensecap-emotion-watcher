# Phase Time Picker Redesign Report

## Slider code removed

- `circe_ui.c`: five `lv_slider_*` widgets and handlers (`time_set_date`, `time_set_time`, `time_apply_date`, `time_apply_time`, `time_save` menu buttons)
- Separate `CIRCE_FLOW_TIME_EDIT_DATE` / `CIRCE_FLOW_TIME_EDIT_TIME` screens replaced by unified `CIRCE_FLOW_TIME` picker

## Rotary number picker

New module `circe_time_picker.c`:

- Five 26 px terminal-style label rows in content column
- Selected row prefixed with `>` and trailing `_` cursor
- Encoder read directly via indev `read_cb` while LVGL indev disabled (avoids slider/focus conflict)
- Press / double-press / long-press timing matches terminal nav (450 ms / 800 ms)

## Ranges and wrap

YEAR 2020–2037, MONTH 1–12, DAY 1–max(month), HOUR 0–23, MIN 0–59 — all wrap at limits. Day clamped when year/month changes.

## Save / cancel

- **Long press** → `circe_time_apply()` + NVS; success → Settings with “Time saved.”
- **Double press** → Settings (no save)
- Save failure → stay on picker, subline “Time save failed.”

## Build

**PASS** — `circe.bin` `0xad560`

## Flash

Run: `idf.py --port /dev/ttyACM1 -b 2000000 app-flash`

## Hardware validation

**Pending on-device** — confirm no bars, rotate/press/long-press, YYYYMMDD path, reboot persistence.

## Entry path after set time

Unchanged: `/sdcard/CIRCE/ENTRIES/YYYYMMDD/` when SET, `UNSET/` when not saved.

## Reboot persistence

Unchanged NVS restore in `circe_time_init()` — confirm on device.

## Remaining bugs

- Interactive validation not automated
- Intensity flow still uses slider (unchanged scope)
- No on-screen SAVE/BACK buttons (encoder-only per spec)

## Commit recommendation

**Yes after hardware checklist:**

```
feat(ui): rotary number picker for date/time setup

Replace TIME slider bars with encoder-driven field list.
```
