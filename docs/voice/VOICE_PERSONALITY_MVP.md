# Voice Personality MVP

**Phase:** Voice Personality MVP  
**Firmware:** `firmware/circe`  
**Status:** Implemented — optional soft local tones via BSP speaker (output only)

---

## Purpose

Optional, local audio cues for save confirmation and regulation support. Not conversational AI, not STT, not recording, not cloud TTS.

---

## Privacy guarantees

- **No microphone capture** — ES7243 mic path is not initialized
- **No speech recognition**
- **No audio recording**
- **No cloud or network audio**
- **No training data**
- **Default OFF** — user must enable SOFT in Settings

---

## Settings

Settings → **VOICE CUES**

| Option | Behavior |
|--------|----------|
| OFF | No tones (default) |
| SOFT | Gentle sine tones on selected events |
| unavailable | Speaker init failed — safe no-op |

Stored in NVS: namespace `circe_voice`, key `mode`.

---

## Hardware

SenseCAP Watcher BSP (`sensecap-watcher`):

| Component | Detail |
|-----------|--------|
| Speaker codec | ES8311 (I2S DAC) |
| Microphone | ES7243 (not used in CIRCE voice) |
| Sample rate | 16 kHz, 16-bit mono |
| API | `bsp_audio_codec_speaker_init()`, `esp_codec_dev_write()` |

CIRCE initializes **speaker output only** — not `bsp_codec_init()` (which also opens microphone).

Reference: factory `app_audio_player.c`, BSP `sensecap-watcher.c`.

---

## Architecture

| File | Role |
|------|------|
| `circe_voice.c/h` | NVS mode, tone generation, background play task |
| `circe_ui.c` | Settings screen, save-success cue |
| `circe_regulation.c` | Regulation start / breathing phase / session complete cues |

Events are queued to a low-priority FreeRTOS task — LVGL is not blocked.

---

## Events wired

| Event | When | Tone |
|-------|------|------|
| `SAVE_OK` | Body save → reflection | Soft C5 blip (~70 ms) |
| `REGULATION_START` | Breathing, anchor, 5-4-3-2-1, bilateral start | Soft G4 (~90 ms) |
| `BREATHE_INHALE/HOLD/EXHALE` | Breathing phase transitions only | Quieter phase tones |
| `SESSION_COMPLETE` | Regulation session completed | Soft A4 (~100 ms) |

**Sensory reset:** silent (low-input tool).

---

## Copy keys

`VOICE_TITLE`, `VOICE_CUES`, `VOICE_OFF`, `VOICE_SOFT`, `VOICE_UNAVAILABLE`, `VOICE_ENABLED`, `VOICE_DISABLED`

---

## Failure / fallback

If speaker init fails:

- Settings show **Audio unavailable**
- `circe_voice_play_event()` no-ops safely
- Journaling and regulation unchanged

---

## Known limitations

- Tones only — no spoken phrases yet
- No volume slider (fixed low output level)
- Breathing phase cues may feel subtle; user can disable
- **REVIEW → TODAY display bug** unchanged
- **Camera capture** still scaffolded
- Daily Companion home feed is separate from voice cues

---

## Future path

- Pre-recorded phrase library mapped to pattern keys
- On-device TTS phrase clips from SD
- Optional LAN TTS (Hades Watch) — out of core scope
