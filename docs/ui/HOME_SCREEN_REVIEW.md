# Home Screen Review — Circle-First Evaluation

Evaluates current CIRCE home: **Body | Quick | Review | More** on 412×412 circular Watcher.

Firmware reference: `circe_ui.c` — `CIRCE_FLOW_HOME`.

---

## Current layout (as built)

```
┌─────────────────────────────┐  ← square framebuffer (corners clipped on device)
│  [strand row 95% × 24px]    │  y ≈ 4
│                             │
│  "How are you arriving..."  │  prompt y=32, width 95%
│                             │
│                             │
│      (empty middle)         │
│                             │
│  ┌─────────────────────┐    │
│  │ Body                │    │  content panel: 100% × 250px
│  │ Quick               │    │  bottom-aligned, y offset -44
│  │ Review              │    │  buttons: 92% × 36px
│  │ More                │    │
│  └─────────────────────┘    │
│  status line (95% width)    │  y ≈ -4 from bottom
└─────────────────────────────┘
```

---

## Usability findings

### Readability

| Element | Assessment |
|---------|------------|
| Greeting prompt | Readable if ≤2 lines; 95% width risks corner clip on long copy |
| Button labels | Short ✓ — single words |
| Status line | Often empty on home; when set, bottom clip risk |
| Strand row | Placeholder void block — low information, OK |

### Touch comfort

| Issue | Severity |
|-------|----------|
| Buttons at **bottom third** of circle | **High** — farthest from center, near lower corner clip |
| Button height **36 px** | **High** — below 44 px minimum |
| Full-width 92% buttons extend into corner clip at bottom | **Medium** |
| Four stacked buttons = good count | ✓ Rule 3 |

**Thumb reach (one-hand hold):** Users typically grip Watcher by bezel; thumb reaches **center-lower** quadrant. Bottom stacking partially aligns with thumb, but **corner clip** removes tap area at button edges.

**Better:** Centered vertical stack in comfort zone (y ≈ 140–280).

### Encoder comfort

| Aspect | Assessment |
|--------|------------|
| Four items in focus group | Good — matches four actions |
| Order: Body → Quick → Review → More | Logical priority ✓ |
| Visible focus indicator | **Missing** — user cannot see selection without theme |
| Press-to-activate | Supported via BSP knob button |

**Recommendation:** Add 2 px accent border + 4 px scale on focused button; keep focus order matching visual top-to-bottom.

### Emotional accessibility

| Positive | Negative |
|----------|----------|
| Only 4 choices | No "I'm overwhelmed" shortcut yet |
| Calm copy | Default gray LVGL chrome feels generic |
| Body-first priority (top button) | "More" bundles diagnostics — slightly technical |

---

## Rule compliance matrix

| Rule | Home compliance |
|------|-----------------|
| 1 Safe area | **Partial fail** — 95%/100% widths |
| 2 Center primary actions | **Fail** — bottom anchored |
| 3 Max four actions | **Pass** |
| 4 Overload-friendly | **Pass** (Quick available) |
| 5 Large targets | **Fail** — 36 px |
| 6 Encoder-first | **Partial** — wired, not visible |

---

## Recommendations (priority order)

### P0 — Layout

1. Replace bottom panel with **centered column** 260×220 px in comfort zone
2. Increase button height to **48 px** (Quick/Body **52 px** optional emphasis)
3. Constrain widths to **260 px** fixed, not `LV_PCT(92)`

### P1 — Visual

4. Apply **CIRCE Classic** theme (soft slate background, warm text)
5. Add visible encoder focus ring
6. Move strand to **arc above prompt** (future radial strand) or slim center strip

### P2 — Comfort

7. Add optional ** fifth hidden path**: long-press Quick = last preset (overload)
8. Split **More** → Settings menu: Appearance | Storage | About
9. Keep greeting to **one short line** on home; move longer copy to first Body step only

---

## Proposed home wireframe (Phase 5)

```
        ╭──────────────╮
       ╭┤  ○ ○ ○  today ├╮   ← strand arc (optional)
      │ │              │ │
      │ │  How are you │ │   ← 1 line prompt
      │ │  arriving?   │ │
      │ │              │ │
      │ │  [  Body   ] │ │   ← 48px, 260px wide
      │ │  [  Quick  ] │ │
      │ │  [ Review  ] │ │
      │ │  [  More   ] │ │
      │ ╰──────────────╯ │
       ╰────────────────╯
```

---

## On-device validation status

| Check | Result |
|-------|--------|
| CIRCE boots | **PASS** (monitor) |
| Home visible | **PENDING** — user visual confirm |
| Touch on four buttons | **PENDING** |
| Encoder through four items | **PENDING** |
| Corner clip visible | **PENDING** — geometric analysis predicts yes |

Hardware available on `/dev/ttyACM1`. Capture photos during Phase 5 implementation.

---

## Related

- [CIRCLE_FIRST_DESIGN.md](CIRCLE_FIRST_DESIGN.md)
- [SAFE_AREA_SPEC.md](SAFE_AREA_SPEC.md)
- [ENCODER_FIRST_NAVIGATION.md](ENCODER_FIRST_NAVIGATION.md)
