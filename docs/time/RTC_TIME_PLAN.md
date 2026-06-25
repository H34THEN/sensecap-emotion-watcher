# RTC / Time Plan — CIRCE Standalone

## Problem

Without a valid clock, entries land in `/sdcard/CIRCE/ENTRIES/19700101/` (epoch date).

## Goals

- Standalone-first: no Wi-Fi required
- Manual date/time from device UI
- Correct `YYYYMMDD` folders when time is set
- `UNSET` folder when time is not set
- Legacy `19700101` entries remain readable

## Architecture

### Module: `circe_time.c`

| Function | Role |
|----------|------|
| `circe_time_init()` | After `bsp_rtc_init()`, accept system time if year ≥ 2020; else restore NVS manual time |
| `circe_time_apply()` | `settimeofday()` + NVS persist |
| `circe_time_is_set()` | Whether saves use dated folders |
| `circe_time_entry_storage_folder()` | `YYYYMMDD` or `UNSET` |
| `circe_time_fill_entry_timestamps()` | ISO timestamps or `"unset"` |

### Persistence

- Boot must call `nvs_flash_init()` before `circe_time_init()` (see `main.c`).
- NVS namespace `circe_time`: `manual_set`, `year`, `month`, `day`, `hour`, `minute`
- Software clock via `settimeofday()` for current session
- On reboot: NVS manual values reapplied if system/RTC still invalid

**Note:** SenseCAP Watcher `bsp_rtc_init()` may provide hardware RTC time. If year ≥ 2020 after boot, that is used without NVS. Manual NVS is fallback when RTC unset.

### Entry JSON fields

| Field | Set | Unset |
|-------|-----|-------|
| `time_status` | `"set"` | `"unset"` |
| `local_date` | `YYYY-MM-DD` | `null` |
| `timestamp` / `created_at` | ISO-8601 UTC | `"unset"` |

### Storage folders

| Condition | Folder |
|-----------|--------|
| Time set | `/sdcard/CIRCE/ENTRIES/YYYYMMDD/` |
| Time unset | `/sdcard/CIRCE/ENTRIES/UNSET/` |
| Legacy | `/sdcard/CIRCE/ENTRIES/19700101/` (still scanned) |

### Review / load

`scan_entries_for_latest()` walks **all** date folders — no change required for legacy compatibility.

## UI (Settings → TIME)

Unified rotary number picker (no sliders):

```
> YEAR   2026 _
  MONTH  06
  ...
```

- Rotate changes selected field; press advances; double-press back; long-press save
- Help subline: `PRESS NEXT  HOLD SAVE`
- See `docs/time/TIME_PICKER_REDESIGN.md`

## Out of scope (this phase)

- NTP / Wi-Fi sync (future, disabled by default)
- Strand date filtering
- Migrating old `19700101` files automatically

## Test plan

1. Boot with unset time → save → path `ENTRIES/UNSET/`
2. Set time → save → path `ENTRIES/20260624/` (example)
3. Review + delete both
4. Reboot → NVS restores manual time → review still works
