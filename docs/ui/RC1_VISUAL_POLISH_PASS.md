# RC1 Visual Polish + Navigation Accessibility Pass

**Phase:** RC1 usability (not a new roadmap feature)  
**Baseline tag:** `circe-standalone-mvp-rc1`

---

## Goals

- Single-focus circular selectors (no confusing multi-option scroll lists)
- Discoverable paths to Patterns, Body Map, Regulation, Voice Cues, Diagnostics
- Centered magenta status banner (black text) for critical status
- Triple-press Home from most screens
- Voice test tone + improved logging

---

## Screens converted to single-focus selector

| Screen | Component |
|--------|-----------|
| Home | `circe_home_wheel.c` — one strong option, index, hints (6 items incl. DIAGNOSTICS) |
| REVIEW menu | `circe_selector.c` |
| SETTINGS | `circe_selector.c` |
| REGULATE menu | `circe_selector.c` |
| VOICE CUES | `circe_selector.c` + TEST TONE |
| DIAGNOSTICS | `circe_selector.c` |
| APPEARANCE / themes | dynamic selector |
| Body area / sensation / tone | dynamic selector |
| Quick note | dynamic selector |

Encoder hints on selector screens: `rotate choose · press enter · triple home`

---

## How to reach every major screen

```text
HOME
  rotate to BODY CHECK-IN → press
  rotate to REVIEW → press
  rotate to REGULATE → press
  rotate to SETTINGS → press
  rotate to DIAGNOSTICS → press

REVIEW
  rotate to TODAY → press
  rotate to PATTERNS → press
  rotate to BODY MAP → press

REGULATE
  rotate to BREATHING → press
  rotate to BILATERAL TAP → press

SETTINGS
  rotate to VOICE CUES → press
```

---

## Center status banner

Module: `circe_status_banner.c/h`

- Centered on screen, magenta (`#FF2BD6`), black text
- Used for: SAVING, LOADING, DELETING, ENTRY SAVED, voice test, audio unavailable
- Bottom HUD subline cleared for critical status (non-critical hints only)

**Lifecycle (2026-06-26 fix):** Indefinite loading banners dismiss on screen transition and worker completion. Timed success/error banners auto-hide (800–1500 ms). `reset()` on triple-press Home. Stale worker callbacks no longer leave banners stuck. If a banner still sticks, capture screen/path and serial logs.

---

## Triple press Home

Module: `circe_encoder.c/h`

- Deferred single-press (550ms) so double/triple can be detected
- Double press → back (terminal nav + selector)
- Triple press → `go_home_safe()` (stops regulation timers, no save/delete)
- Long press → Settings (unchanged on terminal nav)

---

## Voice cues

Path: **HOME → SETTINGS → VOICE CUES**

- OFF (default unless NVS saved SOFT)
- SOFT — initializes speaker on enable, logs mode change
- TEST TONE — plays short tone or shows AUDIO UNAVAILABLE banner
- NVS namespace: `circe_voice` / key `mode`

---

## Manual validation checklist

See `docs/PHASE-RC1-VISUAL-POLISH-PASS-REPORT.md`

---

## Visual editing

See `docs/ui/UI_FILE_MAP.md` — section **START HERE FOR MANUAL UI EDITING**

**Post-RC1 (visual infrastructure pass):**

- **`circe_ui_tokens.h`** — positions, safe zones, banner/feed/selector dimensions
- **`circe_home_bg.c`** + **`assets/circe_homepage_bg.c`** — static Home HUD background (412×412 edge-to-edge)
- Disable background: `CIRCE_UI_HOME_USE_STATIC_BG 0` in tokens header
- Workflow: `docs/ui/MANUAL_UI_EDITING_WORKFLOW.md`

---

## Hardware sign-off (2026-06-26)

Serial boot validation passed on commit `63b794c`. Full on-device UI checklist and screen captures:

- `docs/releases/CIRCE_RC1_HARDWARE_SIGNOFF.md`
- `docs/ui/SCREEN_CAPTURE_GUIDE.md`
- `docs/PHASE-RC1-HARDWARE-SIGNOFF-REPORT.md`

Tag `circe-standalone-mvp-rc1-hardware-signed` deferred until user completes manual capture checklist on Watcher hardware.
