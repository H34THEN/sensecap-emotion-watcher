# Phase 3 Hardware Validation Checklist

Environment: dev build host, **no SenseCAP Watcher connected**  
Date: 2026-06-24

---

## Environment

| Item | Result |
|------|--------|
| ESP-IDF version | **v5.2** (`IDF_PATH=/home/heathen/esp-idf`, tag `v5.2`) |
| Target | esp32s3 |
| Build | **PASS** — `firmware/circe/build/circe.bin` |
| Flash port | **Not attempted** — no `/dev/ttyACM*` or `/dev/ttyUSB*` |
| Monitor port | **Not attempted** |
| Flash command (documented) | `idf.py --port /dev/ttyACM0 -b 2000000 app-flash` |
| nvsfactory protection | Use **`app-flash` only**, not full flash |

---

## Checklist

| # | Test | Result | Notes |
|---|------|--------|-------|
| 1 | Firmware boots | **BLOCKED** | No hardware |
| 2 | Circe home screen appears | **BLOCKED** | |
| 3 | Touch works | **BLOCKED** | |
| 4 | Encoder/dial works | **BLOCKED** | Code attaches LVGL encoder group; needs device |
| 5 | microSD mounts | **BLOCKED** | Boot logs `SD card mounted at /sdcard` when card present |
| 6 | Body-first flow saves entry | **BLOCKED** | Flow implemented + build OK |
| 7 | Quick entry 2–4 taps | **CODE OK** | 2 taps: Quick → preset; optional Home = 3 |
| 8 | Entry review loads saved entry | **BLOCKED** | |
| 9 | Hard delete removes entry | **BLOCKED** | Self-test covers logic on boot if SD ready |
| 10 | Rebuild index works | **BLOCKED** | Diagnostics button wired |
| 11 | Entry persists after reboot | **BLOCKED** | |
| 12 | Storage self-test passes | **BLOCKED** | Runs on boot when SD + storage init OK |
| 13 | Sensation list usable on 1.45" | **BLOCKED** | Scroll panel added; needs touch test |
| 14 | Color skip/save clear | **BLOCKED** | Skip → save done screen with privacy copy |
| 15 | Privacy copy visible | **CODE OK** | Save done + diagnostics standalone notice |

---

## Expected boot log (when hardware available)

```
I (xxx) circe_main: CIRCE standalone MVP starting
I (xxx) circe_main: SD card mounted at /sdcard
I (xxx) circe_storage: storage ready at /sdcard/circe
I (xxx) circe_storage: storage self test passed
```

If SD missing:

```
W (xxx) circe_main: SD card init failed — storage will not work
```

---

## Manual test script (for Watcher owner)

1. Insert FAT32 microSD, flash `app-flash`, open monitor.
2. Confirm SD mount + self-test pass.
3. **Body:** Body → chest → tight → intensity → Continue → Skip color → Home from save done.
4. **More → Rebuild index** — count should match entries.
5. **Review** — latest entry visible.
6. **Delete** — confirm removed.
7. **Reboot** — entry still in Review.
8. **Quick** — Quick → Chest tight (2 taps).
9. **Today strand** — color blocks on home + More → Today strand.
10. **Encoder** — rotate to focus buttons, press to select.

---

## Blockers

- No serial device detected on build host.
- All on-device results remain **BLOCKED** until Watcher + microSD are connected.
