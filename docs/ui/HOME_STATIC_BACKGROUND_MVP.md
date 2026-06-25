# CIRCE Home Static Background MVP

Embedded full-screen HUD background for the **Home** screen only.

---

## Source image

| Property | Value |
|----------|-------|
| Path | `docs/circe_homepage_bg.png` |
| Source dimensions | **1254 × 1254** px |
| Color mode | RGB (no alpha) |
| Design intent | Square PNG aligned for circular Watcher display |

---

## Generated asset

| Property | Value |
|----------|-------|
| C data | `firmware/circe/main/assets/circe_homepage_bg.c` |
| Header | `firmware/circe/main/assets/circe_homepage_bg.h` |
| Symbol | `circe_homepage_bg` |
| Output dimensions | **412 × 412** px |
| Format | **RGB565** (`LV_IMG_CF_TRUE_COLOR`) |
| Resized | **Yes** — uniform scale 1254 → 412 |
| Center-crop | **No** (source already square) |
| Runtime scaling | **No** — 1:1 blit at x=0, y=0 |
| Estimated flash cost | **~339 KB** rodata (`412×412×2`) |
| Estimated RAM cost | **0** (const flash data; LVGL uses pointer) |

---

## Edge-to-edge placement

The background fills the **full logical square canvas** (412×412):

- `CIRCE_UI_HOME_BG_X` = 0  
- `CIRCE_UI_HOME_BG_Y` = 0  
- `CIRCE_UI_HOME_BG_W` = `CIRCE_UI_DISPLAY_W` (412)  
- `CIRCE_UI_HOME_BG_H` = `CIRCE_UI_DISPLAY_H` (412)  

The physical circular display **clips the square corners** naturally. The firmware does **not** shrink the image into the circular safe zone, add letterboxing, or apply non-uniform stretch.

**Dynamic UI** (selector, daily feed, banners) still uses circular **safe-zone tokens** in `circe_ui_tokens.h` for readable text placement **on top** of the image.

---

## Integration files

| Role | File |
|------|------|
| Enable/disable + placement tokens | `firmware/circe/main/circe_ui_tokens.h` |
| Show/hide wrapper | `firmware/circe/main/circe_home_bg.c/h` |
| Home HUD layout (transparent chrome) | `firmware/circe/main/circe_hud.c` |
| Home step routing | `firmware/circe/main/circe_ui.c` |
| Embedded bitmap | `firmware/circe/main/assets/circe_homepage_bg.c` |

---

## Disable fallback

Set in `circe_ui_tokens.h`:

```c
#define CIRCE_UI_HOME_USE_STATIC_BG 0
```

Rebuild and app-flash. Home reverts to the Neon Terminal shell (`CIRCE` / `online` header) with theme background. No SD card or runtime PNG required.

---

## Replace or regenerate the image

1. Edit or replace `docs/circe_homepage_bg.png` (square recommended).
2. Install Pillow: `pip install Pillow`
3. Regenerate:

```bash
python3 scripts/convert_png_to_lvgl_rgb565.py docs/circe_homepage_bg.png \
  --target-size 412 \
  --out-c firmware/circe/main/assets/circe_homepage_bg.c \
  --out-h firmware/circe/main/assets/circe_homepage_bg.h \
  --symbol circe_homepage_bg
```

4. Rebuild and app-flash.

Generated files are **committed** so normal builds do not require Pillow.

---

## Layering

Bottom → top on Home:

1. Static background (`lv_img`, non-clickable, `move_background`)
2. HUD viewport (transparent) + terminal feed
3. Home slot-wheel selector
4. Magenta status banner (`move_foreground` on show)

Triple-press Home returns to Home and re-shows the background via normal `show_step(HOME)` — no leak or misalignment.

---

## Related docs

- `docs/ui/MANUAL_UI_EDITING_WORKFLOW.md`
- `docs/ui/UI_FILE_MAP.md`
- `docs/ui/SAFE_AREA_SPEC.md`
