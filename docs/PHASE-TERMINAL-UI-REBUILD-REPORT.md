# Phase: Terminal UI Rebuild

Presentation-only milestone. No storage, worker, SD, RTC, conversation engine, voice, camera, or cloud changes.

## Goal

Transform CIRCE from smartwatch card UI to an **emotional terminal**: text-first, encoder-navigable, circular layout with subtle telemetry ring.

## What Changed

### New module: `circe_terminal.c/h`

- Terminal feed (3–5 lines, age-based fade, blinking cursor)
- Terminal selection rows (`> LABEL` focus, underline, no filled buttons)
- Encoder gestures: double-press → back, long-press → system menu (Settings)

### HUD (`circe_hud.c`)

- Terminal shell: **CIRCE** header + status line
- Transparent viewport (no card box)
- Outer telemetry ring: four subtle arc segments (CALM / FOCUS / ENERGY / RECOVERY positions)
- Selection menu anchored in lower safe area (no clipped gear button)

### UI flows (`circe_ui.c`)

- Home: terminal feed + `BODY CHECK-IN`, `QUICK NOTE`, `REVIEW`, `SETTINGS`
- All flows use terminal rows instead of filled buttons/cards
- Review empty: feed lines + single `HOME` row; encoder press returns home
- Color step: emotional labels (`CALM`, `FOCUSED`, `TIRED`, `STRESSED`, `OVERWHELMED`, `HOPEFUL`) mapped to existing hex values
- Save/delete/review worker paths unchanged (same `btn_event_cb` action IDs)

### Typography (`circe_fonts.c`)

| Role | Target px |
|------|-----------|
| Hero (CIRCE) | 32 |
| Terminal / selection | 24 |
| Metadata / status | 18 minimum |

### Themes (`circe_theme.c`)

- Transparent viewport, cards, and buttons
- Focus = brighter text + `>` prefix + thin underline (no filled surfaces)
- Theme differences via text, cursor, ring, and focus colors only

## Build

```bash
cd firmware/circe && idf.py build
# app-flash only @ 0x110000
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

Binary: `circe.bin` (~704 KB).

## Hardware Validation Checklist

Capture under `docs/ui/photos/terminal/`:

| File | Screen |
|------|--------|
| `home-terminal.jpg` | Home feed + menu |
| `body-terminal.jpg` | Body area selection |
| `review-empty-terminal.jpg` | Review with no entries |
| `review-entry-terminal.jpg` | Review with saved entry |
| `settings-terminal.jpg` | Settings / More menu |

Verify:

1. Readable at arm's length
2. No clipped controls at bottom arc
3. No large colored cards
4. Knob rotate highlights rows; press selects
5. Double-press back; long-press opens Settings
6. Save / delete / review still work

**Update (UI-Stability phase):** Strand disabled; see `docs/bugs/STRAND_LVGL_STACK_OVERFLOW.md`.

Do **not** proceed to voice, RTC, camera, AI, or animation work until terminal usability is validated on hardware.
