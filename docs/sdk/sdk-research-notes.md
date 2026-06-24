# SenseCAP Watcher SDK — Research Notes

**Primary repository:** [Seeed-Studio/SenseCAP-Watcher-Firmware](https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware)

**License:** Apache 2.0

---

## ESP-IDF version

| Requirement | Source |
|-------------|--------|
| **ESP-IDF v5.2.1** (exact) | Firmware README, [Build Development Environment](https://wiki.seeedstudio.com/build_watcher_development_environment/) wiki |

Setup follows Espressif's standard ESP-IDF installation guide. Wiki recommends a `get_idf` shell alias for environment initialization on Linux/macOS.

**Target chip:** `esp32s3` (`idf.py set-target esp32s3`)

---

## Repository structure (observed from clone)

```
SenseCAP-Watcher-Firmware/
├── components/           Shared BSP and middleware
├── examples/             Standalone demos + factory firmware
└── README.md
```

### Top-level components

| Component | Purpose |
|-----------|---------|
| `sensecap-watcher` | Board support package (BSP) for Watcher hardware |
| `sscma_client` | Himax/SSCMA communication client |
| `lvgl` | LVGL graphics library |
| `esp_lvgl_port` | LVGL porting layer for ESP-IDF |
| `esp_lcd_touch_chsc6x` | Touch controller driver |
| `knob` | Dial wheel / encoder input |
| `rlottie` | Lottie animations |
| `esp_jpeg_simd` | JPEG encode/decode |
| `byte_track` | Object tracking (example use) |
| `esp_io_expander_pca95xx_16bit` | IO expander |

### Examples (selected)

| Example | Relevance to CIRCE |
|---------|-------------------|
| `factory_firmware` | **Primary reference** — full UI, task flow, app layer |
| `lvgl_demo` / `lvgl_encoder_demo` | UI + encoder patterns |
| `get_started` | Minimal bring-up |
| `speech_recognize` | Voice input exploration (future) |
| `openai-realtime` | Cloud voice (explicitly out of CIRCE privacy scope) |
| `knob_rgb` | Encoder + RGB LED |
| `qrcode_reader` | QR (SenseCraft binding pattern) |

**CIRCE will fork/extend from `examples/factory_firmware`**, not greenfield ESP-IDF.

---

## Factory firmware architecture

Documented in [Watcher Software Framework](https://wiki.seeedstudio.com/watcher_software_framework/).

Three layers:

1. **APP applications** — Wi-Fi, BLE, SenseCraft/MQTT, OTA, platform comms.
2. **UI and interaction** — LVGL views, SquareLine-generated UI, page manager.
3. **Task flow** — Node-RED-inspired engine + functional modules (FMs).

### Factory firmware directory layout

```
examples/factory_firmware/main/
├── app/                  Application services (connectivity, OTA, etc.)
├── sensor/               Sensor drivers
├── task_flow_engine/     TFE core (parse JSON, wire modules)
├── task_flow_module/     Built-in functional modules
├── view/                 UI layer
│   ├── ui/               SquareLine export
│   └── ui_manager/       Events, groups, callbacks
└── util/                 Shared utilities
```

---

## Task Flow Engine (TFE)

### Lifecycle

1. Initialize engine; register FMs in linked list.
2. Receive task-flow JSON (local file, MQTT, BLE, or voice — type field).
3. Parse JSON → instantiate modules → configure → wire event pipelines → start.

### Task flow JSON fields

| Field | Meaning |
|-------|---------|
| `tlid` | Template ID |
| `ctd` | Created timestamp |
| `tn` | Task name |
| `type` | 0=local, 1=MQTT, 2=BLE, 3=voice |
| `task_flow[]` | Array of module blocks |

Each block: `id`, `type`, `index`, `version`, `params`, `wires`.

### Module interface (`tf_module.h`)

```c
struct tf_module_ops {
    int (*start)(void *p_module);
    int (*stop)(void *p_module);
    int (*cfg)(void *p_module, cJSON *p_json);
    int (*msgs_sub_set)(void *p_module, int evt_id);
    int (*msgs_pub_set)(void *p_module, int output_index, int *p_evt_id, int num);
};
```

Init order: `cfg` → `msgs_sub_set` → `msgs_pub_set` → `start` → (on teardown) `stop`.

### Event pipeline

- Uses ESP-IDF `esp_event` (wrapped in `tf.h`).
- Module `id` doubles as event ID for subscriptions.
- Large payloads use **pointer passing** (`tf_data_buf`) to avoid copy overhead.
- Data types in `tf_module_data_type.h` (prefix `TF_DATA_TYPE_`).

### Built-in functional modules

| Module | Role |
|--------|------|
| `timer` | Periodic excitation source |
| `ai camera` | Himax capture + inference |
| `alarm trigger` | Attach audio/text to detections |
| `image analyzer` | LLM image analysis (cloud) |
| `local alarm` | On-device sound/RGB/image/text alarm |
| `sensecraft alarm` | Platform notification |
| `uart alarm` | UART output alarm |

**CIRCE note:** Custom FMs for scheduled reflection reminders are possible in Phase 3+; core journaling is UI-driven, not task-flow-driven.

---

## UI integration

Documented in [Watcher UI Integration Guide](https://wiki.seeedstudio.com/watcher_ui_integration_guide/).

| Concept | Location |
|---------|----------|
| SquareLine UI export | `view/ui/` |
| Callback implementations | `view/ui_manager/ui_events.c` |
| Page transitions / groups | `pm.c`, `pm.h`, `view_pages.c` |
| UI init | `view_init()` in `view.c` calls `ui_init()` + `lv_pm_init()` |
| Thread safety | `lvgl_port_lock()` / `unlock()` around LVGL ops |
| Alarm overlay | `view_alarm.c` |

Inputs: **touchscreen + encoder**. Objects added to LVGL groups for encoder focus.

**Warning from Seeed:** Modifying factory page callback bindings can break SenseCraft integration; prefer style-only changes on existing pages, or add new pages.

---

## Build process

```bash
# One-time
git clone https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware.git
cd SenseCAP-Watcher-Firmware
git submodule update --init

# Per example
cd examples/factory_firmware
idf.py set-target esp32s3
idf.py build
```

Wiki path uses `example/factory_firmware` (singular) — repository uses `examples/factory_firmware` (plural). **Use actual repo path.**

---

## Flash process

```bash
# Prefer app-only flash to protect nvsfactory
idf.py --port /dev/ttyACM0 -b 2000000 app-flash

# Monitor (try each UART if no output)
idf.py --port /dev/ttyACM0 monitor
```

**Before risky operations:** backup `nvsfactory` partition with `esptool.py read_flash` (see wiki for offset `0x9000`, size 204800).

Exit monitor: `Ctrl+]`

---

## Serial console (factory firmware)

Documented commands on hardware wiki include: `help`, `wifi_sta`, `ota`, `taskflow`, `factory_info`, `battery`, `bsp`, `reboot`, `factory_reset`, `record`, `vi_ctrl`, `iperf`, `rgb`.

Useful for Phase 2 bring-up and hardware validation without custom UI.

---

## Submodule dependency

README references cloning `SenseCAP-Watcher` with submodules. The GitHub repo name is `SenseCAP-Watcher-Firmware`; verify submodule init after clone.

---

## CIRCE SDK strategy (Phase 2 recommendation)

1. **Base project:** `examples/factory_firmware` copy or git submodule.
2. **UI approach:** New SquareLine project for Circe screens; integrate via `view/` pattern.
3. **Data layer:** New `circe_storage` component (microSD + JSON/SQLite — TBD in schema doc).
4. **Avoid:** Cloud `image analyzer`, SenseCraft dependency for core journaling.
5. **Optional retain:** Wi-Fi/BLE stack disabled or gated for LAN-only sync module.

---

## Unknowns for SDK integration

| Unknown | Why it matters |
|---------|----------------|
| Minimal build without SenseCraft MQTT | Standalone privacy mode |
| LVGL version pinned in repo | SquareLine export compatibility |
| Partition table free space for Circe app | OTA headroom |
| Direct camera API outside task flow | Photo capture step in user flow |
| File system API for microSD in app layer | Entry persistence |
| Speech pipeline without cloud | Local voice on Watcher |

See [docs/roadmap/phase-2-implementation.md](../roadmap/phase-2-implementation.md).
