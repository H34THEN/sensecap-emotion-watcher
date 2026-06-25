# CIRCE Screen Capture Guide

Practical guide for photographing or recording every important CIRCE screen on the SenseCAP Watcher circular display. Use this after flashing the latest RC1 UI-polish firmware.

**Baseline tags:** `circe-standalone-mvp-rc1`, `circe-standalone-mvp-rc1-ui-polish`  
**Flash command:** `idf.py --port /dev/ttyACM1 -b 2000000 app-flash` (from `firmware/circe`)

---

## Capture prep

1. Flash latest firmware from `origin/main` (app-flash only — do not erase flash).
2. Boot to **HOME** and wait for daily companion lines to load (~2–3 s).
3. Wipe the display; use stable lighting; avoid glare on the circular lens.
4. Rotate the encoder **slowly** — single-focus screens show one strong option at a time.
5. **Press** to enter (wait ~550 ms after press — deferred select for double/triple detection).
6. Capture **one screen at a time**; note index text (e.g. `2 / 6`) in the photo.
7. For status banners, trigger the action and capture while the magenta banner is visible.

**Input reminders**

| Gesture | Action |
|---------|--------|
| Rotate | Change selected option |
| Single press | Enter / select (after ~550 ms) |
| Double press | Back |
| Triple press | Home (from most screens) |
| Long press | Settings (terminal nav only) |

---

## Required captures

For each screen: **path**, **what should be visible**, **clipping checks**, **source files**.

### HOME

| Capture | Path | Visible / check | Files |
|---------|------|-----------------|-------|
| HOME | Boot → Home | Static HUD background edge-to-edge; one strong option (default BODY CHECK-IN), index `1 / 6`, rotate/press hints; daily feed lines over background; no letterboxing | `circe_home_bg.c`, `assets/circe_homepage_bg.c`, `circe_home_wheel.c`, `circe_daily.c`, `circe_ui.c` |

### Body check-in flow

| Capture | Path | Visible / check | Files |
|---------|------|-----------------|-------|
| BODY CHECK-IN entry | HOME → BODY CHECK-IN → press | Selector title BODY, one body area, index | `circe_ui.c`, `circe_selector.c`, `circe_entry.c` |
| BODY AREA | (above) | Selected area centered; no scroll list of all areas | same |
| BODY SENSATION | area → press → sensation | One sensation label dominant | same |
| INTENSITY | sensation → press | Slider 1–10, CONTINUE button; slider not clipped at bottom | `circe_ui.c`, `circe_theme.c` |
| EMOTIONAL TONE | intensity flow → tone | Single-focus tone selector | `circe_ui.c`, `circe_copy.c` |
| COLOR FIELD | tone → color picker | Canvas gradient, magnifier, hex label; no object grid | `circe_color_picker.c`, `circe_ui.c` |
| COLOR PRESETS | picker → presets (if shown) | Preset row or selector | `circe_ui.c`, `circe_color_intel.c` |
| ENTRY READY | confirm before save | Save / review actions readable | `circe_ui.c`, `circe_hud.c` |
| SAVING BANNER | press Save | **Centered magenta banner**, black SAVING text; not at bottom edge | `circe_status_banner.c`, `circe_ui.c` |
| REFLECTION | after save | Reflection text in terminal feed | `circe_reflection.c`, `circe_ui.c` |
| PHOTO MEMORY PROMPT | reflection → photo step | Consent / skip actions | `circe_photo.c`, `circe_ui.c` |
| CAMERA UNAVAILABLE | photo capture attempt | Fallback message (capture scaffold) | `circe_photo.c`, `circe_ui.c` |

### Review / memory

| Capture | Path | Visible / check | Files |
|---------|------|-----------------|-------|
| REVIEW MENU | HOME → REVIEW → press | Single-focus: TODAY, PATTERNS, BODY MAP visible in rotation | `circe_ui.c`, `circe_selector.c` |
| TODAY BROWSER | REVIEW → TODAY → press | Entry lines in feed (time, body/tone/color); index subline | `circe_memory_browser.c`, `circe_timeline.c`, `circe_ui.c` |
| ENTRY DETAIL | TODAY → select entry → press | Detail view; VIEW/DELETE actions | `circe_ui.c`, `circe_worker.c` |
| DELETING BANNER | detail → delete confirm | Center magenta DELETING / deleted banner | `circe_status_banner.c`, `circe_ui.c` |
| ALL ENTRIES | REVIEW → ALL ENTRIES | Browse list in feed | `circe_timeline.c`, `circe_memory_browser.c` |
| PATTERNS | REVIEW → PATTERNS → press | LOADING banner then pattern summary | `circe_patterns.c`, `circe_ui.c` |
| BODY MAP | REVIEW → BODY MAP → press | LOADING banner then `#` bar summary | `circe_body_map.c`, `circe_ui.c` |

### Regulation

| Capture | Path | Visible / check | Files |
|---------|------|-----------------|-------|
| REGULATE MENU | HOME → REGULATE → press | BREATHING, BILATERAL TAP, etc. one at a time | `circe_ui.c`, `circe_selector.c` |
| BREATHING | REGULATE → BREATHING → press | Phase label, timer updates | `circe_regulation.c`, `circe_ui.c` |
| 5-4-3-2-1 | REGULATE → 5-4-3-2-1 | Step prompts | `circe_regulation.c` |
| SENSORY RESET | REGULATE → SENSORY RESET | Reset steps | `circe_regulation.c` |
| BILATERAL TAP | REGULATE → BILATERAL TAP | Left/right pulse display | `circe_regulation.c`, `circe_ui.c` |

### Settings / voice / appearance

| Capture | Path | Visible / check | Files |
|---------|------|-----------------|-------|
| SETTINGS | HOME → SETTINGS → press | APPEARANCE, TIME, VOICE CUES in rotation | `circe_ui.c`, `circe_selector.c` |
| VOICE CUES | SETTINGS → VOICE CUES → press | SOFT, TEST TONE, OFF, BACK | `circe_ui.c`, `circe_voice.c` |
| VOICE TEST BANNER | VOICE CUES → enable SOFT → TEST TONE | PLAYING TONE / TONE SENT or AUDIO UNAVAILABLE banner | `circe_voice.c`, `circe_status_banner.c` |
| APPEARANCE | SETTINGS → APPEARANCE | Theme selector | `circe_theme.c`, `circe_ui.c` |
| TIME SET | SETTINGS → TIME | Time picker UI | `circe_time_picker.c`, `circe_ui.c` |

### Diagnostics

| Capture | Path | Visible / check | Files |
|---------|------|-----------------|-------|
| DIAGNOSTICS | HOME → DIAGNOSTICS → press | TEST SAVE, RUN PROBE, etc. | `circe_ui.c`, `circe_selector.c` |
| TEST SAVE RESULT | DIAGNOSTICS → TEST SAVE → press | Banner during test; result lines JSON OK / INDEX OK / LOAD OK / DEL OK | `circe_worker.c`, `circe_ui.c` |

---

## Clipping checklist (all captures)

- [ ] Hero text not cut off at top or bottom of circle
- [ ] Magenta banner fully inside readable disc (not on bottom rim)
- [ ] **Loading/saving banner disappears after operation completes**
- [ ] Selector index (`N / M`) readable
- [ ] Terminal feed lines (max 5) not overlapping selector
- [ ] No ghost list items implying false selectability

**Banner lifecycle (2026-06-26):** Banners should clear on completion, back, and triple-press Home. If stuck, note path and report via daily trial guide.

---

## Triple-press Home (optional capture)

From Body check-in, Color picker, Reflection, Review, Patterns, Body Map, Breathing, Settings, Voice Cues, Diagnostics: triple-press should return HOME without save/delete. Capture HOME after triple-press to confirm clean exit.

---

## Related docs

- `docs/ui/UI_FILE_MAP.md` — file ownership per screen
- `docs/ui/RC1_VISUAL_POLISH_PASS.md` — selector and banner behavior
- `docs/releases/CIRCE_RC1_HARDWARE_SIGNOFF.md` — validation pass/fail table
