# Phase Settings Persistence Cleanup Report

## Root cause

Missing `nvs_flash_init()` in `main.c`. Theme save called `nvs_open("circe_ui")` before NVS subsystem was initialized → `ESP_ERR_NVS_NOT_INITIALIZED`.

## NVS error codes

Before fix: `nvs_open` failed with **`ESP_ERR_NVS_NOT_INITIALIZED`** (logged generically as "failed to save theme_id").

After fix: boot logs `NVS initialized`; theme save logs success or specific step failure via `esp_err_to_name()`.

## Theme persistence (expected after fix)

1. Apply Fall Out of Time → monitor shows `theme saved to NVS`
2. Reboot → `theme active: Fall Out of Time`

**Re-verify on device** after app-flash.

## Time persistence (unchanged logic, NVS now init'd)

1. Settings → TIME → long-press save → status SET
2. Reboot → status SET (NVS `circe_time` restore or valid system clock)
3. Test Save → `/sdcard/CIRCE/ENTRIES/YYYYMMDD/`

User confirmed pre-fix: Test Save to `20260624/54474824.JSN`, JSON/INDEX/LOAD/DEL OK.

## Core entry flow

No storage/UI/time changes in this phase — regression expected unchanged.

## Build

Run after flash: `idf.py build` → expect PASS.

## Flash

```bash
cd firmware/circe && idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

## Commit

```
stabilize circe storage, time, terminal UI, and settings persistence
```

Includes: storage worker, `.JSN` paths, terminal UI, RTC/time, rotary picker, NVS init, theme logging fix.

## Remaining bugs

- Strand disabled (stack overflow)
- Diagnostics health check on LVGL task
- `bsp_rtc_get_time` not wired into `circe_time` (NVS + settimeofday only)

## Recommended next phase

Body/check-in UX polish or worker-backed diagnostics — defer voice/camera/ML/cloud.
