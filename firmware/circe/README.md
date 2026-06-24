# CIRCE — Standalone firmware MVP

ESP-IDF v5.2.1 project for SenseCAP Watcher.

## Prerequisites

1. ESP-IDF v5.2.1 installed (`export.sh`)
2. SenseCAP Watcher SDK cloned:

```bash
export CIRCE_WATCHER_SDK=/path/to/SenseCAP-Watcher-Firmware
git clone --recursive https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware.git "$CIRCE_WATCHER_SDK"
```

## Build

```bash
cd firmware/circe
source $IDF_PATH/export.sh
idf.py set-target esp32s3
idf.py build
```

## Flash (bottom USB port, protect nvsfactory)

```bash
idf.py --port /dev/ttyACM0 -b 2000000 app-flash
idf.py --port /dev/ttyACM0 monitor
```

Use `app-flash` only — do not full-flash unless `nvsfactory` is backed up.

## Storage layout (microSD)

```
/sdcard/circe/
  entries/YYYY-MM-DD/<uuid>.json   # canonical source of truth
  index/entry_index.jsonl          # rebuildable index (SQLite circe.db in Phase 3)
```

## Scope (Phase 2)

Body-only and Quick entry modes, local private storage, review, delete, index rebuild.
