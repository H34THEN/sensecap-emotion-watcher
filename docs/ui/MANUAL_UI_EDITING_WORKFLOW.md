# CIRCE Manual UI Editing Workflow

How to safely tune CIRCE visuals on the SenseCAP Watcher without breaking storage, navigation, or signed RC1 behavior.

**Start here:** `firmware/circe/main/circe_ui_tokens.h`

---

## 1. Start from a branch

```bash
git checkout -b ui-playground
```

Keep experiments off `main` until you are happy with a look.

---

## 2. Edit UI tokens first

Primary file:

`firmware/circe/main/circe_ui_tokens.h`

**Safe to edit in this file:**

- x/y positions and offsets
- widths/heights
- selector/home label spacing
- banner size and center offset
- terminal feed panel size and Y offset
- color field dimensions
- regulation screen spacing
- circle-safe margin constants
- selector label opacities
- Home static background enable/placement (`CIRCE_UI_HOME_USE_STATIC_BG`, `CIRCE_UI_HOME_BG_*`)

**Home background image (source PNG):**

1. Replace `docs/circe_homepage_bg.png` (square recommended).
2. Regenerate embedded asset (requires Pillow):

```bash
pip install Pillow
```

```bash
python3 scripts/convert_png_to_lvgl_rgb565.py docs/circe_homepage_bg.png \
  --target-size 412 \
  --out-c firmware/circe/main/assets/circe_homepage_bg.c \
  --out-h firmware/circe/main/assets/circe_homepage_bg.h \
  --symbol circe_homepage_bg
```

3. Rebuild and app-flash. See `docs/ui/HOME_STATIC_BACKGROUND_MVP.md`.

**Also edit for colors:**

`firmware/circe/main/circe_theme.c` — palette hex values (Neon Terminal green/magenta/text)

**Do not edit casually:**

- `circe_ui.c` — `go_step`, worker callbacks, save/delete
- `circe_worker.c` — storage/index
- NVS keys, SD paths, partition table
- `CMakeLists.txt` unless adding a new UI file

---

## 3. Build

```bash
cd firmware/circe
```

```bash
idf.py build
```

---

## 4. Flash app only

```bash
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

Do **not** erase flash or full flash.

---

## 5. Test on device

Minimum visual check after each tuning pass:

| Screen | Path |
|--------|------|
| Home | Boot → rotate options |
| Review | HOME → REVIEW |
| Body Check-In | HOME → BODY CHECK-IN |
| Color Picker | check-in flow → color field |
| Regulation | HOME → REGULATE → BREATHING |
| Settings | HOME → SETTINGS |
| Diagnostics | HOME → DIAGNOSTICS |
| Status Banner | save, load review, or TEST SAVE |

Confirm:

- Text not clipped at circle edge
- Magenta banner centered and readable
- Banner clears after load/save (RC1 fix)
- Triple-press Home still works

Serial boot shows token summary:

```text
CIRCE UI tokens loaded
CIRCE home static background enabled: 412x412 RGB565
```

(Background log appears when Home is shown and `CIRCE_UI_HOME_USE_STATIC_BG` is 1.)

---

## 6. Commit visual iteration

```bash
git add firmware/circe/main/circe_ui_tokens.h docs/ui/MANUAL_UI_EDITING_WORKFLOW.md docs/ui/UI_FILE_MAP.md
```

```bash
git commit -m "style(ui): tune circular layout"
```

---

## Safe vs dangerous edits

### Safe

| Area | File |
|------|------|
| Positions/sizes | `circe_ui_tokens.h` |
| Colors | `circe_theme.c` |
| Copy text | `circe_copy.c` |
| Font roles (if registered) | `circe_fonts.c` |

### Be careful

| Area | File |
|------|------|
| Screen routing | `circe_ui.c` |
| Selector logic | `circe_selector.c` |
| Banner lifecycle | `circe_status_banner.c` |
| Regulation timers | `circe_regulation.c` |

### Do not edit casually

- Worker / storage / index
- Entry schema / SD paths
- NVS keys / partition table
- Camera / voice hardware init paths

---

## Screen → token quick map

| What you want to move | Token prefix |
|-----------------------|--------------|
| Home menu item position | `CIRCE_UI_HOME_*` |
| Review/Settings selector | `CIRCE_UI_SELECTOR_*` |
| Magenta banner | `CIRCE_UI_STATUS_BANNER_*` |
| Terminal feed lines | `CIRCE_UI_TERMINAL_*` |
| HUD viewport | `CIRCE_UI_HUD_*` |
| Color field | `CIRCE_UI_COLOR_*` |
| Breathing / bilateral | `CIRCE_UI_REG_*` |
| Circle safe margins | `CIRCE_UI_SAFE_*` / `CIRCE_UI_COMFORT_*` |
| Home static background | `CIRCE_UI_HOME_USE_STATIC_BG`, `CIRCE_UI_HOME_BG_*` |

Full file map: `docs/ui/UI_FILE_MAP.md`

---

## Related docs

- `docs/ui/UI_FILE_MAP.md` — screen-by-screen file ownership
- `docs/ui/HOME_STATIC_BACKGROUND_MVP.md` — embedded Home HUD background
- `docs/ui/SAFE_AREA_SPEC.md` — 412×412 circular zones
- `docs/ui/SCREEN_CAPTURE_GUIDE.md` — photo checklist
- `docs/releases/CIRCE_DAILY_TRIAL_GUIDE.md` — daily use
