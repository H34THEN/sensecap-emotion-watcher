# Home Background Blue/Cyan Artifact Bug

**Status:** Fixed (2026-06-25)  
**Scope:** Visual only — Home static HUD background  
**Baseline:** CIRCE Standalone MVP RC1

---

## Symptom

After adding the embedded Home background (`docs/circe_homepage_bg.png`), the Watcher showed unwanted **blue/cyan blotches or flares** rising from the top and bottom of the circular display.

---

## Root cause (investigated)

### Primary: unsafe PNG → RGB conversion pipeline

The original converter used `Image.open(path).convert("RGB")` without an explicit **RGBA → black composite** step. That pattern is unsafe when a PNG contains transparent or semi-transparent pixels: hidden RGB values in those pixels can become visible after conversion.

**Current source file check (2026-06-25):**

| Property | Value |
|----------|-------|
| IHDR color type | 2 (RGB, no alpha channel) |
| Dimensions | 1254 × 1254 |
| Transparent pixels | 0 |
| Semi-transparent pixels | 0 |

So the **on-disk PNG today has no alpha**, but the converter was still fixed to always:

1. Open as RGBA  
2. Resize to target square  
3. `Image.alpha_composite(black, rgba)`  
4. Convert to RGB  
5. Encode RGB565  

This prevents future PNG exports with alpha from leaking hidden channel data.

### Secondary: prior RGB565 encode bug (already fixed in prior pass)

An earlier asset generator wrote 16-bit values into a `uint8_t` array (one byte per pixel). That truncated encoding corrupts colors — magenta HUD accents can decode as **cyan/blue** when LVGL reads byte pairs incorrectly.

The corrected pipeline emits **two bytes per pixel** (native little-endian `lv_color_t` layout, `LV_COLOR_16_SWAP=0`).

### Not the cause (ruled out)

- Source flattened preview contains **0 cyan/blue splotch pixels**
- RGB565 roundtrip preview contains **0 cyan/blue splotch pixels**
- Asset `data_size` matches `412 × 412 × 2 = 339488` bytes
- Descriptor width/height = 412 × 412

### Visible HUD accents (not artifacts)

The source art includes **magenta accent glow** near 12 o'clock and 6 o'clock (top/bottom center of the circular display). These are intentional HUD elements in the PNG, not conversion defects. If they appear **cyan** on hardware, that indicates byte-order or stale asset corruption — not the source art color.

---

## Fix

1. Updated `scripts/convert_png_to_lvgl_rgb565.py` — alpha flatten on black, investigation report, preview outputs  
2. Regenerated `firmware/circe/main/assets/circe_homepage_bg.c/h`  
3. Removed `lv_obj_set_size()` on the Home `lv_img` (REAL mode 1:1 blit; avoids scaling blur)  
4. Added `lv_img_set_antialias(false)` on Home background  

---

## Preview files (debug)

| File | Purpose |
|------|---------|
| `firmware/circe/main/assets/circe_homepage_bg.preview.png` | Flattened RGB before RGB565 encode |
| `firmware/circe/main/assets/circe_homepage_bg.rgb565_preview.png` | Decoded back from RGB565 bytes |

Inspect these **before flashing** when replacing the source PNG.

---

## Regenerate

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

If colors are still wrong on device (unlikely with current SDK), retry with `--swap-bytes` and compare previews.

---

## Disable background

In `firmware/circe/main/circe_ui_tokens.h`:

```c
#define CIRCE_UI_HOME_USE_STATIC_BG 0
```

Rebuild and app-flash only.

---

## Related

- `docs/ui/HOME_STATIC_BACKGROUND_MVP.md`
- `docs/ui/MANUAL_UI_EDITING_WORKFLOW.md`
