# CIRCE RC1 Hardware Sign-Off

**Date:** 2026-06-26  
**Firmware commit tested:** `63b794c` (`fix(ui): improve circular navigation and status banners`)  
**Tags on baseline:** `circe-standalone-mvp-rc1`, `circe-standalone-mvp-rc1-ui-polish`  
**Sign-off flash commit:** docs-only follow-up (this phase) — re-flashed same `63b794c` binary  
**Device port:** `/dev/ttyACM1`  
**Flash command:**

```bash
cd firmware/circe
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

**Hardware-signed tag:** Not created — full on-device UI walkthrough requires user confirmation (see below).

---

## Serial boot validation (automated)

| Check | Result | Evidence |
|-------|--------|----------|
| Boot from OTA app | PASS | Loaded app offset `0x110000` |
| SD mount | PASS | `/sdcard` mounted, CIRCE paths OK |
| Storage probe | PASS | PROBE write/read/delete OK |
| Worker start | PASS | `worker task started stack=16384` |
| Home wheel | PASS | `home wheel created: 6 options, index=0` |
| Daily companion | PASS | `timeline today: 2 items`, daily summary logged |
| Voice init | PASS | `voice init mode=off` (default OFF) |
| Panic / Guru | PASS | None observed |
| Worker stack HWM | PASS | ~2460 words free after daily job |

---

## Manual validation checklist

Legend: **PASS** = confirmed on device · **PENDING** = not confirmed in this session · **N/A**

### Navigation paths

| Path | Status |
|------|--------|
| HOME → BODY CHECK-IN | PENDING |
| HOME → REVIEW | PENDING |
| HOME → REVIEW → TODAY | PENDING (serial: 2 today items loaded) |
| HOME → REVIEW → PATTERNS | PENDING |
| HOME → REVIEW → BODY MAP | PENDING |
| HOME → REGULATE | PENDING |
| HOME → REGULATE → BREATHING | PENDING |
| HOME → REGULATE → BILATERAL TAP | PENDING |
| HOME → SETTINGS | PENDING |
| HOME → SETTINGS → VOICE CUES | PENDING |
| HOME → DIAGNOSTICS | PENDING |
| HOME → DIAGNOSTICS → TEST SAVE | PENDING |

### Full body entry flow

| Step | Status |
|------|--------|
| Body area → sensation → intensity → tone → color | PENDING |
| Save → SAVING banner (centered magenta) | PENDING |
| Reflection | PENDING |
| Photo skip / unavailable | PENDING |
| REVIEW → entry visible | PENDING |
| Delete test entry → DELETING banner | PENDING |

### Regulation

| Step | Status |
|------|--------|
| Breathing timer updates | PENDING |
| Triple-press Home stops breathing | PENDING |
| Bilateral tap display | PENDING |
| Triple-press Home from bilateral | PENDING |

### Voice

| Step | Status |
|------|--------|
| OFF / SOFT / TEST TONE visible | PENDING |
| SOFT persists in NVS | PENDING |
| TEST TONE banner + tone or AUDIO UNAVAILABLE | PENDING |
| No microphone enabled | PASS (code: output-only path) |

### Status banner

| Trigger | Status |
|---------|--------|
| SAVING / ENTRY SAVED | PENDING |
| LOADING MEMORY / PATTERNS / BODY MAP | PENDING |
| DELETING | PENDING |
| TEST SAVE | PENDING |
| Voice test / AUDIO UNAVAILABLE | PENDING |

### Triple-press Home

| From screen | Status |
|-------------|--------|
| Body check-in, color picker, reflection | PENDING |
| Review, patterns, body map | PENDING |
| Regulation, settings, voice, diagnostics | PENDING |

---

## Known limitations (RC1)

- Camera capture remains scaffold-only (CAMERA UNAVAILABLE fallback).
- Color picker and intensity still use buttons/slider (not single-focus selector).
- Single-press has ~550 ms deferral for double/triple detection.
- One unreadable SD entry skipped at boot (`skip unreadable entry id=271815EA`).
- Voice hardware may show AUDIO UNAVAILABLE if speaker init fails on device.

---

## Daily trial use approval

**Conditional — not fully signed off in automation.**

Serial boot, storage, worker, and navigation **code paths** are healthy. Full tactile UI validation and TEST SAVE on device were **not completed by the agent** in this session.

**Approved for daily trial use when:**

1. User completes `docs/ui/SCREEN_CAPTURE_GUIDE.md` checklist on hardware.
2. TEST SAVE reports JSON OK / INDEX OK / LOAD OK / DEL OK.
3. REVIEW → TODAY shows entry lines on display (not just in serial logs).

Until then, treat RC1 as **release-candidate for structured user testing**, not final hardware-signed release.

---

## Recommended next steps

1. User: walk capture guide and fill pass/fail in this doc.
2. If all paths pass: tag `circe-standalone-mvp-rc1-hardware-signed`.
3. Then: manual visual polish via `docs/ui/UI_FILE_MAP.md` or camera capture phase.
