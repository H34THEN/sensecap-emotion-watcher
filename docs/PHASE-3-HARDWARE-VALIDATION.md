# Phase 3 Hardware Validation Checklist

Environment: SenseCAP Watcher connected on `/dev/ttyACM1`  
Date: 2026-06-24 (updated after partition fix)

---

## Partition mismatch (root cause)

**Symptom:** After flash, monitor still showed `Project name: factory_firmware`.

**Cause:** CIRCE used a generic single-app partition table (`factory` at 0x10000). The Watcher uses **OTA slots** — factory firmware runs from `ota_0` at **0x110000**.

**Fix:** Replaced `firmware/circe/partitions.csv` with Watcher-compatible OTA layout matching device boot table. Set `CONFIG_ESPTOOLPY_FLASHSIZE_32MB` and `CONFIG_PARTITION_TABLE_CUSTOM` in `sdkconfig.defaults`.

**Important:** Partition table is for **build/link only**. Do not `partition-table-flash` to device.

See [hardware/WATCHER_FLASHING_NOTES.md](hardware/WATCHER_FLASHING_NOTES.md).

---

## Environment

| Item | Result |
|------|--------|
| ESP-IDF version | **v5.2** |
| Target | esp32s3 |
| Build | **PASS** |
| Flash port | **`/dev/ttyACM1`** |
| Monitor port | **`/dev/ttyACM1`** |
| Flash command | `idf.py --port /dev/ttyACM1 -b 2000000 app-flash` |
| Flash address | `0x110000` (ota_0) only |
| nvsfactory | Protected — app-flash only |

---

## Boot validation (monitor)

| Check | Result | Evidence |
|-------|--------|----------|
| Project name | **PASS** | `Project name: circe` |
| Loaded partition | **PASS** | `Loaded app from partition at offset 0x110000` |
| Partition table match | **PASS** | Boot table matches Watcher layout |
| SD mount | **PASS** | `SD card mounted at /sdcard` |
| Storage init | **PASS** | `storage ready at /sdcard/circe` |
| Touch init | **PASS** | `Touch panel create success` |
| Encoder init | **PASS** | `Knob Config Succeed` |
| Boot stability | **PASS** | 30s monitor — no panic/reboot after init-order fix |
| factory_firmware | **GONE** | No longer appears in boot logs |

---

## Interactive checklist (on-device — user)

| # | Test | Result | Notes |
|---|------|--------|-------|
| 1 | Firmware boots | **PASS** | Monitor proof above |
| 2 | Circe home screen appears | **PENDING** | Logs stable; visual confirm on device |
| 3 | Touch works | **PENDING** | Touch panel initialized |
| 4 | Encoder/dial works | **PENDING** | Knob initialized |
| 5 | microSD mounts | **PASS** | Log proof |
| 6 | Body-first flow saves entry | **PENDING** | |
| 7 | Quick entry 2–4 taps | **PENDING** | |
| 8 | Entry review loads saved entry | **PENDING** | |
| 9 | Hard delete removes entry | **PENDING** | |
| 10 | Rebuild index works | **PENDING** | Diagnostics → Rebuild |
| 11 | Entry persists after reboot | **PENDING** | |
| 12 | Storage self-test passes | **PENDING** | Run from Diagnostics (not on boot) |
| 13 | Sensation list scroll | **PENDING** | |
| 14 | Color skip/save clear | **PENDING** | |
| 15 | Privacy copy visible | **PENDING** | Save done + More → Diagnostics |

---

## Boot crash fixed (2026-06-24)

Initial CIRCE boot on hardware hit:

1. `assert failed: vTaskGenericNotifyGiveFromISR` — boot self-test + LVGL/touch race
2. `Interrupt wdt timeout` — SD I/O under `lvgl_port_lock` during Today strand load

**Fixes:**

- Storage init before LVGL; removed boot self-test (Diagnostics only)
- No SD reads under LVGL lock on home screen
- `lvgl_port_lock(-1)` for UI init

---

## Manual test script

1. Flash: `idf.py --port /dev/ttyACM1 -b 2000000 app-flash`
2. Monitor: confirm `Project name: circe`
3. Body / Quick / Review / Delete / Reboot flows
4. More → Run self test / Rebuild index

---

## Rollback

- nvsfactory backup: `backups/watcher/nvsfactory.bin`
- Factory app may remain in ota_1 — see WATCHER_FLASHING_NOTES.md
