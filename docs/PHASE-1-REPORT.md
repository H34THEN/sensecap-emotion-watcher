# Phase 1 Handoff Report

Generated: 2026-06-24

---

## 1. Repository structure created

```
sensecap-emotion-watcher/
├── README.md
├── .gitignore
├── docs/
│   ├── architecture/
│   │   ├── overview.md
│   │   ├── module-map.md
│   │   └── data-flow.md
│   ├── hardware/
│   │   └── sensecap-watcher-research.md
│   ├── sdk/
│   │   └── sdk-research-notes.md
│   ├── personality/
│   │   └── circe-personality.md
│   ├── design/
│   │   ├── user-flow.md
│   │   ├── body-sensation-system.md
│   │   ├── color-system.md
│   │   └── privacy-model.md
│   ├── schema/
│   │   └── entry-schema.md
│   ├── modules/          (21 module specs)
│   ├── integration/
│   │   ├── magic-mirror.md
│   │   └── hades-watch.md
│   ├── ml/
│   │   └── training-dataset-design.md
│   └── roadmap/
│       └── phase-2-implementation.md
├── firmware/
│   └── README.md         (placeholder)
└── schemas/
    └── emotion-entry.schema.json
```

---

## 2. Documentation created

| Category | Documents |
|----------|-----------|
| Hardware facts | sensecap-watcher-research.md |
| SDK / build | sdk-research-notes.md |
| Architecture | overview, module-map, data-flow |
| Design | user-flow, body-sensation, color, privacy |
| Personality | circe-personality.md |
| Schema | entry-schema.md + JSON schema |
| Modules | 21 module specification files |
| Integration | magic-mirror, hades-watch |
| ML planning | training-dataset-design.md |
| Roadmap | phase-2-implementation.md |

---

## 3–6. Git status

See final git commands output in commit step below.

---

## 7. Watcher hardware facts discovered

| Area | Fact |
|------|------|
| MCU | ESP32-S3 @ 240 MHz, 8 MB PSRAM, 32 MB flash |
| AI | Himax HX6538 (Cortex-M55 + Ethos-U55), 16 MB flash |
| Display | 1.45", 412×412 touchscreen (SPI/I2C) |
| Touch | Capacitive + CHSC6x driver in SDK |
| Input | Dial wheel encoder (scroll + press) |
| Camera | OV5647, 120° FOV, fixed ~3 m, MIPI |
| Mic / Speaker | Single mic, 1 W speaker, I2S |
| Storage | microSD up to 32 GB FAT32; SPI flash |
| Wi-Fi | 802.11 b/g/n 2.4 GHz |
| BLE | Bluetooth 5 |
| RGB LED | 1× RGB, CLI modes breath/blink/solid |
| USB | Bottom port = data+power; back = power only |
| Power | 5 V / 1 A; 400 mAh backup battery |
| Temp | 0–45 °C operating |

Sources: Seeed wiki hardware overview, OSHW repo, firmware SDK clone.

---

## 8. SDK facts discovered

| Fact | Detail |
|------|--------|
| ESP-IDF | **v5.2.1** required |
| Target | esp32s3 |
| Repo | SenseCAP-Watcher-Firmware (Apache 2.0) |
| Reference app | examples/factory_firmware |
| UI | LVGL + SquareLine; view/ + ui_manager/ |
| Architecture | APP + UI + Task Flow Engine (Node-RED style) |
| Module API | tf_module.h ops: start/stop/cfg/msgs_sub/pub |
| Events | esp_event via tf.h; pointer pass for large data |
| Flash | Prefer `app-flash`; protect nvsfactory partition |
| UART | Two ports (ESP32 + Himax); try both for monitor |

---

## 9. Architecture recommendations

1. **Fork factory_firmware**, do not greenfield ESP-IDF.
2. **Body-first flow** as default UX path in orchestrator.
3. **microSD JSON/SQLite** for entries; photos as separate JPEG files.
4. **Disable or gate SenseCraft cloud** for privacy-compliant journaling.
5. **SquareLine** for UI; new Circe pages rather than modifying bound factory callbacks.
6. **Vertical slice first:** body tags → color → save → reload.
7. **Defer** task-flow custom modules until core UI journaling works.
8. **LAN integration** via sync_queue only after local storage proven.

---

## 10. Major unknowns

1. Standalone firmware without SenseCraft binding — compile-time feasibility?
2. Still photo capture API from custom LVGL (outside ai camera task flow)?
3. SQLite vs JSONL on FAT32 — performance and corruption recovery?
4. SD encryption for at-rest protection?
5. Battery runtime during full Circe session?
6. SquareLine / LVGL version lock with SDK submodules?
7. Partition free space for Circe OTA?
8. Crisis/distress UX policy for meltdown_warning tags?
9. Hades Watch API and GPU stack — not yet specified by user?
10. Magic Mirror transport — HTTP poll vs push?

---

## 11. Questions before implementation

1. Should Circe replace Watcher home UI entirely or live as a menu app?
2. Is microSD always present, or must internal flash fallback exist?
3. Preferred storage: SQLite or JSONL?
4. Is SenseCraft connectivity required for any feature, or fully offline?
5. Photo step: required, optional, or off by default?
6. Custom emotion list seed data from user?
7. LAN hostnames/IPs for Magic Mirror and Hades when sync is enabled?
8. Voice features priority relative to journaling MVP?
9. GitHub remote URL for this repository?
10. Crisis resources: local-only disclaimer vs emergency contact feature?

---

## 12. Recommended Phase 2 handoff prompt

```
# PHASE 2 CURSOR HANDOFF — CIRCE Firmware MVP

You are implementing Phase 2 of CIRCE on SenseCAP Watcher.

Read first:
- docs/roadmap/phase-2-implementation.md
- docs/sdk/sdk-research-notes.md
- docs/hardware/sensecap-watcher-research.md
- docs/design/body-sensation-system.md
- docs/design/privacy-model.md
- schemas/emotion-entry.schema.json

Goal: Vertical slice on hardware —
  body_sensation_tags → color_picker → privacy toggles → local_storage save → entry_review reload.

Constraints:
- ESP-IDF v5.2.1, esp32s3, base on factory_firmware
- Defaults: training_ok=false, private_locked=true
- No cloud, no ML, no Magic Mirror/Hades yet
- Body-first path must allow skipping emotion

Deliverables:
1. firmware/circe project building and flashing
2. local_storage writing JSON to microSD
3. Minimal SquareLine/LVGL screens for slice
4. Updated docs with decisions made (storage choice, camera API findings)

Start with M1–M2 from roadmap; confirm hardware flash before UI work.
```

---

**Phase 1 complete. Do not start Phase 2 in this session.**
