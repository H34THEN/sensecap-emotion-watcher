# Phase Report — Voice Personality MVP

**Date:** 2026-06-25  
**Branch:** main  
**Roadmap:** `docs/CIRCE_future_functionality_roadmap_reference.md` — Section 9

---

## Summary

Added optional **Voice Cues** setting (OFF / SOFT, default OFF) with worker-safe background tone playback via BSP ES8311 speaker. No microphone, STT, recording, or cloud. Regulation sensory reset stays silent.

---

## Files added/modified

| File | Change |
|------|--------|
| `firmware/circe/main/circe_voice.c/h` | **New** — NVS, tones, play task |
| `firmware/circe/main/circe_ui.c` | Settings + save cue |
| `firmware/circe/main/circe_regulation.c` | Regulation/breathing cues |
| `firmware/circe/main/circe_copy.c/h` | Voice copy keys |
| `firmware/circe/main/circe_conversation_engine.h` | `CIRCE_FLOW_VOICE_CUES` |
| `firmware/circe/main/main.c` | `circe_voice_init()` |
| `firmware/circe/main/CMakeLists.txt` | Added `circe_voice.c` |
| `docs/voice/VOICE_PERSONALITY_MVP.md` | **New** |
| Docs updates | roadmap, companion spec, copy polish, regulation docs |

---

## Hardware findings

- Watcher has **I2S + ES8311 speaker DAC + ES7243 mic** in BSP
- Factory firmware uses `bsp_codec_init()` + `bsp_i2s_write()` / MP3/WAV player
- CIRCE uses **speaker-only** init to avoid microphone path
- 16 kHz mono PCM sine tones, low volume (~22/100), short duration

---

## Implementation mode

**Real soft tones** (not scaffold-only), with safe no-op if init fails.

---

## Validation checklist

- [x] Boot / home wheel / worker (serial)
- [x] `circe_voice` init logged, default off
- [ ] Settings → Voice Cues OFF/SOFT + reboot persist (manual)
- [ ] Save cue when SOFT enabled (manual)
- [ ] Regulation cues (manual)
- [ ] Regression: save, reflection, regulation, diagnostics (manual)

---

## Build / flash

**Build:** PASS — `circe.bin` `0xC64B0` (812208 bytes), 94% free  
**Flash:** PASS — app-flash `/dev/ttyACM1`  
**Boot:** PASS — no panic; home wheel created; voice init mode=off

---

## Git

Pending commit and push.
