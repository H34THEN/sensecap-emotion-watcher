# Phase 4 Report — Circle-First UX + CIRCE Theme System

Generated: 2026-06-24

---

## Summary

Phase 4 is **design and documentation only** — no firmware feature development.

CIRCE boots on Watcher hardware (`Project name: circe`, `/dev/ttyACM1`). This phase defines circle-first layout rules, safe-area geometry, encoder-first recommendations, visual language, mood strand visualization study, and a 10-theme local architecture.

---

## Deliverables created

### UI (`docs/ui/`)

| Document | Purpose |
|----------|---------|
| [CIRCLE_FIRST_DESIGN.md](ui/CIRCLE_FIRST_DESIGN.md) | Six rules, screen audit |
| [SAFE_AREA_SPEC.md](ui/SAFE_AREA_SPEC.md) | Exact dimensions, touch minimums |
| [HOME_SCREEN_REVIEW.md](ui/HOME_SCREEN_REVIEW.md) | Body/Quick/Review/More evaluation |
| [MOOD_STRAND_VISUALIZATION_STUDY.md](ui/MOOD_STRAND_VISUALIZATION_STUDY.md) | Options A–E analysis |
| [CIRCE_VISUAL_LANGUAGE.md](ui/CIRCE_VISUAL_LANGUAGE.md) | Typography, spacing, motion |
| [ENCODER_FIRST_NAVIGATION.md](ui/ENCODER_FIRST_NAVIGATION.md) | Dial navigation requirements |

### Themes (`docs/themes/`)

| Document | Purpose |
|----------|---------|
| [THEME_ARCHITECTURE.md](themes/THEME_ARCHITECTURE.md) | Module design, LVGL integration |
| [THEME_COLOR_PALETTES.md](themes/THEME_COLOR_PALETTES.md) | All 10 theme hex palettes |
| [THEME_SWITCHING_MODEL.md](themes/THEME_SWITCHING_MODEL.md) | NVS, More → Appearance flow |
| [THEME_ACCESSIBILITY_GUIDE.md](themes/THEME_ACCESSIBILITY_GUIDE.md) | Contrast, overload, encoder |

---

## 1. Current UI issues found

| Issue | Source | Severity |
|-------|--------|----------|
| Rectangular layout on circular display | `LV_PCT(100)` containers | **High** |
| Buttons bottom-anchored, not center | `circe_ui_init()` geometry | **High** |
| Button height 36 px | `BTN_H` | **High** |
| No visible encoder focus | No `LV_STATE_FOCUS_KEY` styles | **High** |
| Default LVGL gray chrome | No theme system | **Medium** |
| Strand row horizontal full-width | Corner clip | **Medium** |
| Review screen dense text block | Cognitive load | **Medium** |
| Diagnostics bundles too much | More screen overload | **Medium** |
| SD I/O from LVGL callbacks | Stability risk (Phase 3 partial fix) | **Medium** |
| Interactive validation incomplete | Home/touch/flows PENDING | **Open** |

---

## 2. Circular display risks

- **Corner clipping:** 95–100% width elements lose tap targets in four corner wedges.
- **Bottom status bar:** y ≈ 408 with 95% width — text clipped on lower arc.
- **Thumb vs geometry mismatch:** Bottom stack helps thumb but worsens clip.
- **False affordance:** Rectangular scroll panels suggest more content in clipped zones.
- **Encoder focus hidden:** Users rotate dial without visual feedback.

---

## 3. Safe-area dimensions

| Zone | Radius | Diameter | Rect bounds (approx) |
|------|--------|----------|----------------------|
| Full visible | 206 px | 412 px | Full panel |
| **Safe content** | **194 px** | **388 px** | 12–400 px all axes |
| **Comfort (primary CTAs)** | **150 px** | **300 px** | 56–356 px |
| Max inscribed square (safe) | — | 274 px side | (69,69)–(343,343) |

Recommended content column: **260 px wide**, centered at x=206.

See [SAFE_AREA_SPEC.md](ui/SAFE_AREA_SPEC.md).

---

## 4. Screens requiring redesign

| Priority | Screen | Change |
|----------|--------|--------|
| P0 | Home | Center stack, 48 px buttons, safe width |
| P0 | All list screens | 260 px column, 48 px rows |
| P1 | Review | Icon summary, less monospace text |
| P1 | Diagnostics | Split Appearance vs Storage |
| P2 | Today strand | Radial arc (replace horizontal row) |
| P2 | Save done | Center confirm + Home |

---

## 5. Encoder recommendations

1. Visible focus ring using theme `accent` (all themes)
2. Focus **Body** on home entry
3. Paginate or favorite-row for 15+ item lists
4. Test full flows encoder-only on hardware
5. No audio feedback on focus change (default)

See [ENCODER_FIRST_NAVIGATION.md](ui/ENCODER_FIRST_NAVIGATION.md).

---

## 6. Theme system recommendations

- Implement `circe_theme.c` with 10 static palettes (Phase 5)
- NVS persist `theme_id` in `circe_ui` namespace
- **More → Appearance** picker (instant apply + fade)
- Default: **Circe Classic**
- Shared LVGL styles — not per-widget color hardcoding
- Entry `color_hex` independent of UI theme

See [themes/THEME_ARCHITECTURE.md](themes/THEME_ARCHITECTURE.md).

---

## 7. Accessibility findings

- Current 36 px buttons fail WCAG / CIRCE Rule 5
- High Visibility theme specified: 56 px, yellow focus, 22 px prompt
- No auto-theme from mood — required for trust
- Eva-01 / Fall Out themes need optional intensity warning copy
- Encoder-only path incomplete without focus visibility

See [THEME_ACCESSIBILITY_GUIDE.md](themes/THEME_ACCESSIBILITY_GUIDE.md).

---

## 8. Recommended default theme

**CIRCE CLASSIC** — soft slate `#2D3748`, ivory text `#F7FAFC`, lavender accent `#9F7AEA`.

Low stimulation, warm, non-clinical. Matches Circe personality docs.

---

## 9. Mood strand visualization recommendation

| Surface | Choice |
|---------|--------|
| Home today indicator | **Option B — Radial arc** (upper safe arc) |
| Today detail (More) | **Option E — Tree-ring segments** |
| Reject for core | Option D Constellation (diagnostic feel) |
| Mirror plugin (future) | Option A rows on rectangular display |

Do not implement in Phase 4. See [MOOD_STRAND_VISUALIZATION_STUDY.md](ui/MOOD_STRAND_VISUALIZATION_STUDY.md).

---

## 10. Should firmware UI changes begin?

**Yes — Phase 5**, scoped narrowly:

1. Safe-area layout refactor (`circe_ui_layout`)
2. 48 px buttons, centered column
3. Theme system + Appearance screen
4. Encoder focus styles
5. Hardware photo validation

**Do not** add cloud, voice, photos, GPU, Mirror, or Hades in Phase 5.

---

## 11. Recommended Phase 5 prompt

```markdown
# PHASE 5 — CIRCE Circle-First UI + Theme Implementation

Read: docs/ui/*, docs/themes/*, docs/PHASE-4-REPORT.md

Implement on firmware/circe:
1. Safe-area layout container (260px column, comfort zone centered)
2. 48px minimum buttons; home center stack
3. circe_theme.c — 10 palettes, NVS persist, More → Appearance
4. Encoder focus ring (all themes)
5. Replace home strand with radial arc (max 8 blocks)
6. Split More → Appearance | Storage
7. Hardware validation with photos: Home, Body, Quick, Review, Diagnostics
8. Complete interactive checklist in PHASE-3-HARDWARE-VALIDATION.md

Still standalone. No integrations.
```

---

## On-device validation (Phase 4)

| Item | Result |
|------|--------|
| Hardware connected | `/dev/ttyACM1` present |
| CIRCE boots | **PASS** (prior session) |
| Screenshot capture | **Not performed** — geometric/code audit only |
| Visual clip confirmation | **Predicted** — pending Phase 5 photos |
| Theme readability | **Design spec only** — not in firmware |

---

## Git

Documentation-only phase. Commit recommended as:

> Add Phase 4 circle-first UX and theme system design docs.

No firmware changes in this phase.

---

**Phase 4 complete. Ready for Phase 5 implementation.**
