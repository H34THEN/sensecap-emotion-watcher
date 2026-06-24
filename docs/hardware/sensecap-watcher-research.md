# SenseCAP Watcher — Hardware Research Notes

**Sources (official):**

- [Hardware Overview](https://wiki.seeedstudio.com/watcher_hardware_overview/)
- [Build Development Environment](https://wiki.seeedstudio.com/build_watcher_development_environment/)
- [OSHW-SenseCAP-Watcher](https://github.com/Seeed-Studio/OSHW-SenseCAP-Watcher) (schematics, datasheets)

All facts below are drawn from these sources unless marked **CIRCE inference** (design assumption, not hardware fact).

---

## Core compute

| Component | Specification | Source |
|-----------|---------------|--------|
| Main MCU | ESP32-S3 @ 240 MHz, 8 MB PSRAM | Wiki hardware table |
| AI coprocessor | Himax HX6538 (WiseEye2): Cortex-M55 + Ethos-U55 | Wiki, OSHW README |
| ESP32 flash | 32 MB | Wiki hardware table |
| Himax flash | 16 MB | Wiki hardware table |

The ESP32-S3 handles UI, connectivity, audio I/O, storage orchestration, and application logic. The Himax handles on-device vision inference and communicates with the ESP32 (SSCMA client component in firmware SDK).

---

## Display

| Property | Value |
|----------|-------|
| Size | 1.45 inch |
| Resolution | 412 × 412 |
| Type | LCD touchscreen |
| Interface | SPI/I2C (per hardware diagram) |

**UI stack (firmware):** LVGL, typically designed with SquareLine Studio. Touch controller driver in SDK: `esp_lcd_touch_chsc6x`.

**CIRCE implication:** Round/square high-DPI UI; multi-step forms must be scrollable or paginated. Encoder + touch dual input is supported.

---

## Touch input

- Capacitive touchscreen on the 412×412 display.
- Firmware also supports **encoder (dial wheel)** input alongside touch.
- LVGL group management (`pm.c` / `pm.h`) synchronizes focus between touch and encoder.

**CIRCE implication:** All interactive controls should be reachable via wheel for accessibility; body-area maps may need encoder-friendly navigation.

---

## Camera

| Property | Value |
|----------|-------|
| Sensor | OV5647 |
| FOV | 120° |
| Focus | Fixed focal, ~3 m |
| Interface | MIPI (per hardware diagram) |

**Firmware image formats (task flow pipeline):**

- Large image: 640×480 JPEG (from Himax), often base64 in task-flow messages.
- Small image: 416×416 JPEG (inference crop).

Serial console documents camera-related task flows via `ai camera` module; shutter can be triggered by upstream data.

**CIRCE implication:** Photo capture for emotional entries is feasible via existing camera/Himax path; exact still-capture API for custom UI requires Phase 2 SDK investigation (`view_image_preview.c`, SSCMA client).

---

## Microphone

| Property | Value |
|----------|-------|
| Count | Single microphone |
| Interface | I2S |

**Serial console commands (documented on wiki):**

- `record -t <seconds> -f <path>` — record PCM to microSD.
- `vi_ctrl -s|-e|-c|-z` — voice interaction control (wakeup, record, stop, exit).

**CIRCE implication:** Future voice correlation and local voice systems are hardware-supported; Phase 1 does not assume a local STT/LLM stack on-device.

---

## Speaker

| Property | Value |
|----------|-------|
| Output | 1 W speaker |
| Interface | I2S |

Task-flow alarm modules can play MP3 audio. Circe spoken prompts are feasible via existing audio paths in factory firmware.

---

## Storage

| Type | Capacity / limit |
|------|------------------|
| Internal flash (ESP32) | 32 MB — firmware, partitions, SPIFFS |
| Internal flash (Himax) | 16 MB — AI models |
| microSD | Up to 32 GB, **FAT32** |

Extension via SPI microSD slot.

**CIRCE implication:** Emotional entry database and photos should target microSD with size budgeting; FAT32 filename and cluster constraints apply.

---

## Wi-Fi

| Property | Value |
|----------|-------|
| Standard | IEEE 802.11 b/g/n |
| Band | 2.4 GHz only |
| Range | Up to ~100 m (open space, manufacturer claim) |

Factory firmware includes Wi-Fi station mode (`wifi_sta` console command). SenseCraft app binding uses 2.4 GHz Wi-Fi during setup.

**CIRCE default:** No cloud upload. Wi-Fi reserved for optional LAN sync (Magic Mirror, Hades Watch) in future phases.

---

## Bluetooth LE

| Property | Value |
|----------|-------|
| Version | Bluetooth 5 |
| Antenna | Built-in (shared with Wi-Fi) |

Used for SenseCraft app device binding and task-flow delivery (task flow type `2` = Bluetooth per software framework docs).

---

## RGB LED

| Property | Value |
|----------|-------|
| Count | 1× RGB indicator |
| Control | Serial `rgb` command |

**Console syntax (wiki):**

```
rgb -r <0-255> -g <0-255> -b <0-255> -m <1|2|3> [-v step] [-t ms]
```

Modes: 1 = breath, 2 = blink, 3 = solid (default).

**CIRCE implication:** Ambient color feedback during entry flow (e.g., echo chosen mood color) without using the main display.

---

## Dial wheel (encoder)

| Property | Value |
|----------|-------|
| Functions | Scroll up/down, press (button) |
| Interface | GPIO/PWM |

Power on: hold wheel ~3 seconds. Primary navigation alongside touch in factory UI.

SDK component: `knob`.

---

## Extension interfaces

| Interface | Details |
|-----------|---------|
| Grove | 1× I2C |
| 2×4 header | 1× I2C, 2× GPIO, 2× GND, 1× 3.3 V out, 1× 5 V in |

---

## USB

| Port | Function |
|------|----------|
| Back USB-C | Power only |
| Bottom USB-C | Power + data (programming) |

**Programming notes:**

- Connecting via bottom port exposes **two UART devices** (ESP32-S3 and Himax); either may map to either port — try each when flashing/monitoring.
- Recommended flash baud: 2,000,000.
- Use **`app-flash`** subcommand to avoid erasing `nvsfactory` partition (contains factory calibration/data).
- Backup `nvsfactory` before full flash operations (documented offset/size on wiki).

---

## Power

| Property | Value |
|----------|-------|
| Input | 5 V DC (must not exceed 5 V) |
| Recommended adapter | 5 V / 1 A |
| Battery | 3.7 V, 400 mAh Li-ion backup |
| Operating temperature | 0–45 °C |

Console: `battery` command reports charge percentage.

---

## AI coprocessor capabilities (documented)

- Himax HX6538 runs vision models (object detection, gestures, pets, etc.) via SSCMA.
- Models can be OTA updated (`ota` console command: types 0=AI model, 1=Himax, 2=ESP32).
- Task-flow `ai camera` module: model config, conditions, silent periods, inference output with bounding boxes/classes.
- `image analyzer` module: can call external LLM for image analysis (cloud-oriented in factory firmware — **not used by CIRCE default privacy model**).

**CIRCE inference:** On-device Himax useful for future face/pose/proximity features only with explicit consent; not Phase 2 scope.

---

## Mounting

- 360° rotate bracket (included with device body).
- Optional 1/4" tripod bracket (sold separately).

---

## Open hardware artifacts

From [OSHW-SenseCAP-Watcher](https://github.com/Seeed-Studio/OSHW-SenseCAP-Watcher):

- `SenseCAP_Watcher_v1.0_SCH.pdf`
- ESP32-S3 datasheet
- HX6538-A datasheet
- Prebuilt ESP32 and Himax firmware binaries (reference only)

---

## Gaps requiring Phase 2 hardware validation

1. Exact still-photo capture API from custom LVGL UI (vs. task-flow pipeline JPEG).
2. Maximum practical microSD write rate and safe concurrent read (UI + logging).
3. Battery life during continuous Circe session (display + touch + optional camera).
4. Touch coordinate mapping on round 412×412 canvas edge zones.
5. Whether custom firmware can run **without** SenseCraft binding (standalone LAN-only mode).
