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

**Hardware-signed tag:** `circe-standalone-mvp-rc1-hardware-signed` — **not created** (full on-device checklist not completed in automation).

---

## Banner lifecycle fix (2026-06-26)

**Issue:** Loading/saving banners could remain visible after worker completion because banners live on the screen root (not cleared by `clear_content()`).

**Fix:** `circe_status_banner_reset()`, `dismiss_indefinite()`, screen-transition cleanup, worker stale-callback guard, delete uses DELETING banner.

**Code status:** PASS (built and app-flashed)  
**On-device banner matrix:** PENDING user confirmation

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
| 2 | HOME selector works | PENDING (manual) |
| 3 | REVIEW opens | PENDING |
| 4 | TODAY entries or empty state | PENDING (serial: 2 today items) |
| 5 | PATTERNS opens | PENDING |
| 6 | BODY MAP opens | PENDING |
| 7 | REGULATE opens | PENDING |
| 8 | BREATHING starts | PENDING |
| 9 | Triple-press Home | PENDING |
| 10 | DIAGNOSTICS → TEST SAVE (JSON/INDEX/LOAD/DEL OK) | PENDING |
| 11 | Status banners do not stick | CODE FIX · PENDING manual |
| 12 | No panic | PASS (serial) |
| 13 | No boot loop | PASS |
| 14 | No worker stack overflow | PASS (prior HWM ~2460 words) |

---

## Manual validation checklist

Legend: **PASS** · **PENDING** · **CODE FIX**

### Navigation paths

| Path | Status |
|------|--------|
| HOME → BODY CHECK-IN | PENDING |
| HOME → REVIEW → TODAY | PENDING |
| HOME → REVIEW → PATTERNS | PENDING |
| HOME → REVIEW → BODY MAP | PENDING |
| HOME → REGULATE → BREATHING | PENDING |
| HOME → SETTINGS → VOICE CUES | PENDING |
| HOME → DIAGNOSTICS → TEST SAVE | PENDING |

### Status banner matrix

| Trigger | Status |
|---------|--------|
| LOADING MEMORY (REVIEW → TODAY) | CODE FIX · PENDING |
| LOADING PATTERNS | CODE FIX · PENDING |
| LOADING BODY MAP | CODE FIX · PENDING |
| SAVING → ENTRY SAVED | CODE FIX · PENDING |
| DELETING | CODE FIX · PENDING |
| TEST SAVE | CODE FIX · PENDING |
| TEST TONE / AUDIO UNAVAILABLE | CODE FIX · PENDING |
| Triple-press clears banner | CODE FIX · PENDING |

---

## Known limitations (RC1)

- Camera capture scaffold-only.
- Color picker / intensity use buttons/slider.
- Single-press ~550 ms deferral.
- Voice speaker may show AUDIO UNAVAILABLE on hardware.

---

## Approved for daily trial use

**CONDITIONAL**

Banner lifecycle fix is in firmware and serial boot passes. Full tactile validation (especially TEST SAVE and banner matrix) still requires user confirmation on Watcher hardware.

**Use:** `docs/releases/CIRCE_DAILY_TRIAL_GUIDE.md`

**Upgrade to YES when:**

1. Banner matrix passes on device.
2. TEST SAVE reports JSON OK / INDEX OK / LOAD OK / DEL OK.
3. REVIEW → TODAY shows entries on display.

Then tag `circe-standalone-mvp-rc1-hardware-signed`.

---

## Related docs

- `docs/releases/CIRCE_DAILY_TRIAL_GUIDE.md`
- `docs/ui/SCREEN_CAPTURE_GUIDE.md`
- `docs/ui/UI_FILE_MAP.md`
