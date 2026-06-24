# CIRCE Safe Area Specification — 412×412 Circular Display

Target: SenseCAP Watcher 1.45" display, **412×412 px**, circular viewport.

---

## Coordinate system

| Property | Value |
|----------|-------|
| Panel resolution | 412 × 412 px |
| Origin | Top-left (0, 0) |
| Center | **(206, 206)** |
| Full visible radius | **206 px** |
| Aspect | Square framebuffer, **circular mask** applied in hardware/firmware |

LVGL uses the full 412×412 rectangle. Content in square corners is **clipped or visually lost** on the round face.

---

## Zone definitions

### Zone A — Full circle (visible)

- Radius: **206 px** from center
- Diameter: **412 px**
- Anything outside this circle is not visible

### Zone B — Safe content (default layout)

- Radius: **194 px** (12 px inset from bezel edge)
- Diameter: **388 px**
- **All readable text and tappable controls must fit inside Zone B**

| Bound | Value |
|-------|-------|
| Left | x ≥ **12** |
| Right | x ≤ **400** |
| Top | y ≥ **12** |
| Bottom | y ≤ **400** |

### Zone C — Comfort zone (primary actions)

- Radius: **150 px** from center
- Diameter: **300 px**
- **Body, Quick, Continue, Save, Review** should live here when possible

| Bound | Value |
|-------|-------|
| Left | x ≥ **56** |
| Right | x ≤ **356** |
| Top | y ≥ **56** |
| Bottom | y ≤ **356** |

### Zone D — Danger corners (never place critical UI)

Four corner wedges of the 412×412 square that fall **outside Zone B**.

Approximate exclusion (12 px inset):

```
Corner TL: x < 69 AND y < 69
Corner TR: x > 343 AND y < 69
Corner BL: x < 69 AND y > 343
Corner BR: x > 343 AND y > 343
```

At y=44 (current strand row area), max half-width inside Zone B:

```
half_width = sqrt(194² - (194 - 44)²) ≈ 121 px
usable x range ≈ 85 to 327
```

At y=380 (current status bar), half-width ≈ 68 px → usable x ≈ 138 to 274.

**Bottom status text at LV_PCT(95) width will clip in corners.**

---

## Maximum inscribed rectangle (Zone B)

Square fully inside safe circle:

| Property | Value |
|----------|-------|
| Side length | **274 px** (194 × √2) |
| Top-left | **(69, 69)** |
| Bottom-right | **(343, 343)** |

Recommended content container for lists and buttons:

```
width:  260 px  (274 − 14 padding)
height: 240 px  (leave header/footer bands)
x:      76 px centered → left edge 76, right 336
```

---

## Touch target minimums

| Level | Height | Width | Use |
|-------|--------|-------|-----|
| **Minimum** | 44 px | 44 px | WCAG 2.5.5 |
| **CIRCE default** | **48 px** | ≥ 200 px (80% of safe width) | All primary buttons |
| **Quick / overload** | **56 px** | ≥ 220 px | Quick entry, Save, Home |
| **Spacing between targets** | **8 px** minimum | — | Prevent mis-taps |

**Current firmware:** `BTN_H = 36` — **below minimum**. Phase 5 should raise to 48 px minimum.

---

## Vertical band layout (recommended)

```
 y=12–40   Strand / ambient (optional, low priority)
 y=44–100  Circe prompt (wrap, max 2 lines)
 y=100–340 Content (buttons, scroll, slider)
 y=340–380 Status / privacy one-liner
 y=380–400 Avoid text (corner clip risk)
```

Current firmware places content at `ALIGN_BOTTOM_MID, 0, -44` with height 250 — bottom-weighted, which pushes controls into **lower corner clip zones**.

---

## Encoder focus ring

Encoder-highlighted objects should stay in **Zone C** when possible so focus outline remains fully visible on round display.

---

## LVGL implementation notes (Phase 5)

1. Add circular clip mask to root screen (`lv_obj_set_style_radius` + clip, or BSP round mask if available).
2. Replace `LV_PCT(100)` width containers with fixed **260 px** centered column.
3. Use `lv_obj_align(LV_ALIGN_CENTER)` for primary button stacks instead of bottom anchoring.
4. Validate every screen with overlay debug circles at R=194 and R=150.

---

## Related

- [CIRCLE_FIRST_DESIGN.md](CIRCLE_FIRST_DESIGN.md)
- [HOME_SCREEN_REVIEW.md](HOME_SCREEN_REVIEW.md)
- [SMALL_SCREEN_POLISH.md](SMALL_SCREEN_POLISH.md)
