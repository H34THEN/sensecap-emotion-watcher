# CIRCE — Standalone firmware MVP

ESP-IDF v5.2+ project for SenseCAP Watcher (ESP32-S3, 32 MB flash, OTA layout).

## Prerequisites

1. ESP-IDF v5.2+ (`export.sh`)
2. SenseCAP Watcher SDK:

```bash
export CIRCE_WATCHER_SDK=/path/to/SenseCAP-Watcher-Firmware
git clone --recursive https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware.git "$CIRCE_WATCHER_SDK"
```

## Partition table

CIRCE uses a **Watcher-compatible OTA layout** (matches device, build reference only):

| Partition | Offset | Size |
|-----------|--------|------|
| nvsfactory | 0x9000 | 200K |
| nvs | 0x3b000 | 840K |
| otadata | 0x10d000 | 8K |
| phy_init | 0x10f000 | 4K |
| **ota_0** | **0x110000** | **12M** |
| ota_1 | 0xd10000 | 12M |
| model | 0x1910000 | 1M |
| storage | 0x1a10000 | 6080K |

Verify after config change:

```bash
idf.py partition-table
```

**Do not flash the partition table to the device.** Use app-only flash.

See [docs/hardware/WATCHER_FLASHING_NOTES.md](../../docs/hardware/WATCHER_FLASHING_NOTES.md).

## Build

```bash
cd firmware/circe
export CMAKE_POLICY_VERSION_MINIMUM=3.5
source $IDF_PATH/export.sh
idf.py set-target esp32s3
idf.py build
```

Requires `CONFIG_ESPTOOLPY_FLASHSIZE_32MB` and `CONFIG_PARTITION_TABLE_CUSTOM` (in `sdkconfig.defaults`).

## Flash (app-only — protect nvsfactory)

**Port:** `/dev/ttyACM1` (ESP32-S3 bottom USB on validated hardware)

```bash
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
idf.py --port /dev/ttyACM1 monitor
```

Confirm monitor shows `Project name: circe` (not `factory_firmware`).

Never run full flash unless nvsfactory is backed up (`backups/watcher/nvsfactory.bin`).

## Storage layout (microSD)

```
/sdcard/circe/
  entries/YYYY-MM-DD/<uuid>.json
  index/entry_index.jsonl
```

## Scope

Body-only and Quick entry, local private storage, review, edit, delete, Today strand, diagnostics.
