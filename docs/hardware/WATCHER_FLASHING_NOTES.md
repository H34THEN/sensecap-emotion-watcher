# SenseCAP Watcher — CIRCE Flashing Notes

Last updated: 2026-06-24

---

## Ports

| Port | Device | Notes |
|------|--------|-------|
| `/dev/ttyACM1` | ESP32-S3 (bottom USB) | **Use this for CIRCE flash/monitor** |
| `/dev/ttyACM0` | Did not connect to ESP32-S3 in validation session | May be Himax or other interface |

Always confirm with `idf.py -p PORT monitor` boot logs showing `Project name: circe`.

---

## Protected partitions — do not flash

| Partition | Offset | Notes |
|-----------|--------|-------|
| nvsfactory | 0x9000 | Factory calibration — backup at `backups/watcher/nvsfactory.bin` |
| model | 0x1910000 | AI model SPIFFS |
| storage | 0x1A10000 | Internal SPIFFS |
| Himax firmware | — | Never flash from CIRCE workflow |

**Never run full flash or `erase_flash`** unless explicitly approved with backups.

---

## Watcher partition layout (device truth)

```
nvsfactory  0x9000   size 0x32000
nvs         0x3b000  size 0xd2000
otadata     0x10d000 size 0x2000
phy_init    0x10f000 size 0x1000
ota_0       0x110000 size 0xc00000   ← CIRCE app-flash target
ota_1       0xd10000 size 0xc00000
model       0x1910000 size 0x100000
storage     0x1a10000 size 0x5f0000
```

CIRCE `partitions.csv` matches this layout (build-time only — **do not** `partition-table-flash` to device).

---

## App-only flash (safe)

```bash
export CIRCE_WATCHER_SDK=/path/to/SenseCAP-Watcher-Firmware
export CMAKE_POLICY_VERSION_MINIMUM=3.5
source $IDF_PATH/export.sh
cd firmware/circe
idf.py build
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
idf.py --port /dev/ttyACM1 monitor
```

`app-flash` writes **only** `circe.bin` → `0x110000` (ota_0). Does not touch bootloader, partition table, nvsfactory, otadata, model, or storage.

Manual equivalent:

```bash
esptool.py --chip esp32s3 -p /dev/ttyACM1 -b 2000000 write_flash 0x110000 build/circe.bin
```

---

## Monitor proof (2026-06-24)

After partition fix + app-flash to `/dev/ttyACM1`:

```
I (808) cpu_start: Project name:     circe
I (265) boot: Loaded app from partition at offset 0x110000
I (1931) circe_main: SD card mounted at /sdcard
I (2101) circe_storage: storage ready at /sdcard/circe
I (2281) SPD2010: Touch panel create success
```

Previously, wrong partition table (`factory` at 0x10000) caused device to keep booting `factory_firmware` from ota_0.

---

## Backups

- nvsfactory: `backups/watcher/nvsfactory.bin`
- To restore nvsfactory (emergency only): `esptool.py write_flash 0x9000 backups/watcher/nvsfactory.bin`

---

## Rollback to factory firmware

Factory app remains in **ota_1** if not overwritten. Recovery options (user decision):

1. Flash factory `ota_1` app binary to ota_0 via app-flash from factory_firmware build, or
2. Switch otadata boot slot (advanced — modifies otadata only).

Do not erase flash.
