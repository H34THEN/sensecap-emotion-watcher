# Circle-First Design — CIRCE on SenseCAP Watcher

Phase 4 design principle: the Watcher is a **412×412 circular display**, not a rectangle with rounded corners.

CIRCE must feel calm on a round face during overload, shutdown, and decision fatigue.

---

## The six rules (operational)

### Rule 1 — Safe area vs danger area

Every screen has two regions:

| Region | Definition |
|--------|------------|
| **Safe** | Inside radius 194 px (12 px bezel inset) |
| **Danger** | Corner wedges outside safe circle |

Nothing critical in danger zones. See [SAFE_AREA_SPEC.md](SAFE_AREA_SPEC.md).

**Current violation:** `LV_PCT(100)` width containers, bottom-anchored content, 95% width strand row.

---

### Rule 2 — Primary actions near center

Body, Quick, Continue, Save, Review must remain in **Comfort zone** (R ≤ 150 px).

**Current violation:** Home buttons are in a bottom-anchored 250 px panel — visually low on the round face, near clip zone, far from natural thumb center when holding Watcher in one hand.

**Recommendation:** Vertical stack centered at (206, 200) with 48 px buttons, 8 px gaps.

---

### Rule 3 — Maximum four primary actions

Home correctly exposes four actions: Body | Quick | Review | More.

Sub-screens often exceed four visible choices (15 body areas, 18 sensations). Acceptable **only** with scroll + clear Back, and never more than four **primary** actions without scrolling.

**Violations:**

- Review screen: 3 actions OK, but metadata label is dense
- Diagnostics: 4 buttons + long text block — cognitive load high
- Edit menu: 3 actions OK

---

### Rule 4 — Design for overwhelm

States: overwhelm, shutdown, burnout, overstimulation, decision fatigue.

| Principle | Implementation |
|-----------|----------------|
| One decision per screen when possible | Quick entry = 1 tap; Body = one step at a time ✓ |
| Skip always available | Color skip ✓; need Skip on intensity? optional |
| No guilt copy | Circe copy reviewed ✓ |
| Reduce motion | No rlottie in CIRCE ✓; theme switch should not animate flash |
| Private by default | ✓ |

**Gap:** No explicit **Low-energy mode** toggle on home yet (documented in autism-friendly design, not in firmware).

---

### Rule 5 — Large touch targets

Minimum **48×260 px** effective targets. Current **36 px** height fails Rule 5.

---

### Rule 6 — Encoder-first navigation

Every major flow: rotate → highlight → press.

| Flow | Encoder today | Gap |
|------|---------------|-----|
| Home | Group wired | No visible focus style |
| Body / sensation lists | Scroll + group | Long lists tedious; no wrap-around |
| Intensity slider | In group | Works |
| Quick | In group | Works |
| Review / delete | In group | Works |

See [ENCODER_FIRST_NAVIGATION.md](ENCODER_FIRST_NAVIGATION.md).

---

## Screen-by-screen circle audit

| Screen | Clip risk | Reach risk | Cognitive load | Priority fix |
|--------|-----------|------------|----------------|--------------|
| Home | Medium (strand top, status bottom) | High (bottom stack) | Low | Re-center buttons |
| Body area | High (full-width scroll) | Medium | Medium | Narrow column + 48px buttons |
| Sensation | High | Medium | High (18 items) | Same + favorites row later |
| Intensity | Low | Low | Low | OK |
| Add another | Low | Low | Low | OK |
| Color | Medium | Medium | Low | OK |
| Save done | Low | Low | Low | OK |
| Quick | Medium | Medium | Low | OK |
| Review | Medium (multi-line label) | Medium | Medium-high | Simplify label |
| Delete confirm | Low | Low | Low | OK |
| Diagnostics | High (long text) | Low | **High** | Split diagnostics / appearance |
| Today strand | Medium | Medium | Low | Radial study (Phase 5+) |

---

## Emotional comfort checklist

Circe must never feel clinical.

| Check | Status |
|-------|--------|
| Warm copy via pattern keys | ✓ |
| No diagnostic language | ✓ |
| No forced emotion label | ✓ |
| Privacy visible | ✓ |
| Soft visual palette | ✗ default LVGL theme |
| Rounded, gentle shapes | Partial (LVGL default radius) |
| No harsh pure white background | ✗ needs theme |

---

## Phase 5 firmware direction (not Phase 4 scope)

1. Safe-area layout container module (`circe_ui_layout.c`)
2. 48 px minimum buttons, centered column
3. Theme system apply on boot
4. Encoder focus visible ring (theme accent color)
5. More → Appearance (theme picker)
6. Optional circular clip mask

**Do not** add cloud, voice, photos, or integrations.

---

## Related

- [SAFE_AREA_SPEC.md](SAFE_AREA_SPEC.md)
- [CIRCE_VISUAL_LANGUAGE.md](CIRCE_VISUAL_LANGUAGE.md)
- [conversation/CIRCE_AUTISM_FRIENDLY_DESIGN.md](../conversation/CIRCE_AUTISM_FRIENDLY_DESIGN.md)
