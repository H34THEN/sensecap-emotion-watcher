# CIRCE RC1 Hardware Sign-Off

**Date:** 2026-06-26 (updated after banner lifecycle fix)  
**Firmware commit tested:** `ff2c0c8` (`fix(ui): clear status banners on completion and navigation`)  
**Prior baseline:** `63b794c` UI polish · docs `68b1350`  
**Tags:** `circe-standalone-mvp-rc1`, `circe-standalone-mvp-rc1-ui-polish`  
**Device port:** `/dev/ttyACM1`  
**Flash command:**

```bash
cd firmware/circe
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

**Hardware-signed tag:** `circe-standalone-mvp-rc1-hardware-signed` (firmware `ff2c0c8`)

---

## Banner lifecycle fix (2026-06-26)

**Issue:** Loading/saving banners could remain visible after worker completion because banners live on the screen root (not cleared by `clear_content()`).

**Fix:** `circe_status_banner_reset()`, `dismiss_indefinite()`, screen-transition cleanup, worker stale-callback guard, delete uses DELETING banner.

**Code status:** PASS (built and app-flashed)  
**On-device banner matrix:** PASS (user confirmed)

---

## Serial boot validation (automated)

| Check | Result | Evidence |
|-------|--------|----------|
| Boot from OTA app | PASS | Loaded app offset `0x110000` |
| SD mount | PASS | `/sdcard` mounted, CIRCE paths OK |
| Worker start | PASS | `worker task started stack=16384` |
| Home wheel | PASS | `home wheel created: 6 options, index=0` |
| Daily companion | PASS | `timeline today: 2 items` |
| Voice init | PASS | `voice init mode=off` |
| Panic / Guru | PASS | None observed |
| Banner fix firmware flash | PASS | `circe.bin` `0xC8810` |

---

## Required pass items (hardware sign-off)

| # | Item | Status |
|---|------|--------|
| 1 | Device boots | PASS (serial) |
| 2 | HOME selector works | PASS |
| 3 | REVIEW opens | PASS |
| 4 | TODAY entries or empty state | PASS |
| 5 | PATTERNS opens | PASS |
| 6 | BODY MAP opens | PASS |
| 7 | REGULATE opens | PASS |
| 8 | BREATHING starts | PASS |
| 9 | Triple-press Home | PASS |
| 10 | DIAGNOSTICS → TEST SAVE (JSON/INDEX/LOAD/DEL OK) | PASS |
| 11 | Status banners do not stick | PASS |
| 12 | No panic | PASS (serial) |
| 13 | No boot loop | PASS |
| 14 | No worker stack overflow | PASS (prior HWM ~2460 words) |

---

## Manual validation checklist

Legend: **PASS** · **PENDING** · **CODE FIX**

### Navigation paths

| Path | Status |
|------|--------|
| HOME → BODY CHECK-IN | PASS |
| HOME → REVIEW → TODAY | PASS |
| HOME → REVIEW → PATTERNS | PASS |
| HOME → REVIEW → BODY MAP | PASS |
| HOME → REGULATE → BREATHING | PASS |
| HOME → SETTINGS → VOICE CUES | PASS |
| HOME → DIAGNOSTICS → TEST SAVE | PASS |

### Status banner matrix

| Trigger | Status |
|---------|--------|
| LOADING MEMORY (REVIEW → TODAY) | PASS |
| LOADING PATTERNS | PASS |
| LOADING BODY MAP | PASS |
| SAVING → ENTRY SAVED | PASS |
| DELETING | PASS |
| TEST SAVE | PASS |
| TEST TONE / AUDIO UNAVAILABLE | PASS |
| Triple-press clears banner | PASS |

---

## Known limitations (RC1)

- Camera capture scaffold-only.
- Color picker / intensity use buttons/slider.
- Single-press ~550 ms deferral.
- Voice speaker may show AUDIO UNAVAILABLE on hardware.

---

## Approved for daily trial use

**YES**

User confirmed on-device validation on 2026-06-26. Firmware `ff2c0c8` (banner lifecycle fix) is approved for daily trial use on SenseCAP Watcher hardware.

**Guide:** `docs/releases/CIRCE_DAILY_TRIAL_GUIDE.md`

**Tag:** `circe-standalone-mvp-rc1-hardware-signed`

---

## Related docs

- `docs/releases/CIRCE_DAILY_TRIAL_GUIDE.md`
- `docs/ui/SCREEN_CAPTURE_GUIDE.md`
- `docs/ui/UI_FILE_MAP.md`
