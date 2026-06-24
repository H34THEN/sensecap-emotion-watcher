# Encoder-First Navigation — CIRCE

The SenseCAP Watcher dial is a **first-class input**, not an accessibility afterthought.

Reference: factory `pm.c` pattern; CIRCE `circe_ui.c` encoder group wiring.

---

## Hardware

| Input | GPIO / driver | LVGL type |
|-------|---------------|-----------|
| Dial rotate | Encoder A/B (41, 42) | `LV_INDEV_TYPE_ENCODER` |
| Dial press | Knob button | Encoder enter |
| Touch | SPD2010 | Pointer |

BSP initializes both via `bsp_lvgl_init()`.

---

## Current CIRCE implementation

```c
// Each screen rebuild:
lv_group_remove_all_objs(s_group);
// add buttons + slider to group
lv_indev_set_group(encoder_indev, s_group);
```

**Strengths:**

- All buttons on each screen are in the group
- Intensity slider included
- Group recreated per step — no stale focus

**Weaknesses:**

| Gap | Impact |
|-----|--------|
| No visible focus style | User cannot see selection |
| No `lv_group_focus_obj()` on screen entry | Focus may start on wrong item |
| Scroll lists + encoder | Encoder scrolls focus, not list — 18 sensations = many rotations |
| No wrap preference documented | Unknown if focus wraps at list ends |
| Storage actions from LVGL callback | SD I/O blocks UI thread (separate issue) |

---

## Encoder-first flow requirements

Every major flow must support **rotate → highlight → click** without touch.

| Flow | Steps | Encoder path | Touch required? |
|------|-------|--------------|-----------------|
| Home | 4 buttons | Rotate ×4, press | No |
| Quick | 5 presets + back | Rotate, press | No |
| Body area | 15 + back | Rotate ×16 | No (tedious) |
| Sensation | 18 + back | Rotate ×19 | No (tedious) |
| Intensity | Slider + continue + back | Rotate slider, focus continue | No |
| Color | 4 + skip + back | Rotate | No |
| Save done | Home | Press | No |
| Review | Edit, delete, home | Rotate ×3 | No |
| Delete confirm | Yes, cancel | Rotate ×2 | No |
| Diagnostics | 4 buttons | Rotate | No |

**Verdict:** Encoder-complete but **not comfortable** for long lists.

---

## Recommendations

### P0 — Visible focus (Phase 5)

Apply theme accent on focused object:

```c
lv_obj_set_style_border_width(obj, 2, LV_STATE_FOCUS_KEY);
lv_obj_set_style_border_color(obj, theme.accent, LV_STATE_FOCUS_KEY);
lv_obj_set_style_bg_color(obj, theme.surface_focus, LV_STATE_FOCUS_KEY);
```

Also enable `LV_STATE_FOCUSED` from encoder (`lv_group_set_editing` as needed).

### P1 — Focus on screen entry

```c
lv_obj_t *first = /* first primary button */;
lv_group_focus_obj(first);
```

Home: focus **Body** (not Quick) — body-first product principle.

### P2 — List ergonomics

For body/sensation screens (>8 items):

| Option | Description |
|--------|-------------|
| **A. Paginate** | 4 items per page + encoder Next/Back |
| **B. Favorites row** | Top 4 encoder-reachable; rest in scroll |
| **C. Encoder scroll mode** | Map dial to scroll when list focused |

Recommend **B + scroll** for Phase 5; paginate for Phase 6 if needed.

### P3 — Factory-style page manager (optional)

Long-term: adopt simplified `pm.c` pattern for focus memory across Back navigation.

### P4 — Haptic / audio

**Do not add** sound on focus change (autism-friendly). Optional single gentle tick is user setting default **off**.

---

## Encoder vs touch priority

| Context | Primary | Secondary |
|---------|---------|-----------|
| Overload / Quick | Encoder or touch | — |
| Body exploration | Touch (chips) | Encoder |
| Diagnostics | Encoder | Touch |
| One-hand use | Encoder | — |

---

## Testing checklist (hardware)

On `/dev/ttyACM1`:

1. Home: rotate through 4 items — focus visible?
2. Press dial — opens Body?
3. Body list: reach bottom item without touch
4. Intensity: adjust with dial, press Continue
5. Quick: 2 encoder presses to save (focus Quick preset, press)
6. Back always reachable without touch

---

## Related

- [CIRCLE_FIRST_DESIGN.md](CIRCLE_FIRST_DESIGN.md)
- [HOME_SCREEN_REVIEW.md](HOME_SCREEN_REVIEW.md)
- [themes/THEME_ARCHITECTURE.md](../themes/THEME_ARCHITECTURE.md)
