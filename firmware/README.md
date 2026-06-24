# CIRCE Firmware (Placeholder)

**Phase 1:** No firmware source in this directory.

## Phase 2 plan

1. Clone [SenseCAP-Watcher-Firmware](https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware) as submodule or vendor copy.
2. Create `circe` example based on `examples/factory_firmware`.
3. Implement modules per [docs/architecture/module-map.md](../docs/architecture/module-map.md).

## Build prerequisites

- ESP-IDF **v5.2.1**
- Target: `esp32s3`
- SenseCAP Watcher hardware with microSD (FAT32)

See [docs/sdk/sdk-research-notes.md](../docs/sdk/sdk-research-notes.md).
