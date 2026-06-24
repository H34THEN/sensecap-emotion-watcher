# CIRCE Theme Accessibility Guide

How themes support visual accessibility, fatigue, overload, and encoder-only use.

---

## Design principles

1. **Theme is user choice** — Circe never assigns theme from mood data.
2. **High Visibility is first-class**, not a "punishment mode."
3. **Focus must be visible in every theme** — non-negotiable for encoder users.
4. **Motion stays minimal** across all themes.
5. **Contrast ratios** target WCAG AA where emotionally appropriate.

---

## High Visibility theme

Primary accessibility theme. See palette in [THEME_COLOR_PALETTES.md](THEME_COLOR_PALETTES.md).

| Feature | Implementation |
|---------|----------------|
| Text | 20–22 px minimum |
| Buttons | 56 px height |
| Contrast | #FFFFFF on #000000 |
| Focus | #FFFF00 2 px border |
| Muted text | #CCCCCC minimum (not #888 on #888) |

---

## Contrast targets (other themes)

| Pair | Minimum ratio | Notes |
|------|---------------|-------|
| text on bg | 4.5:1 | WCAG AA body |
| muted on bg | 3:1 | Large text only |
| accent on surface | 3:1 | Focus ring |
| danger on bg | 4.5:1 | Delete screen only |

Themes **Eva-01**, **Fall Out of Time**, **Terminal Kitty** run higher contrast — warn in Appearance:

> "High contrast theme — may feel intense."

Not blocking — user choice.

---

## Overload / autism-friendly alignment

From [CIRCE_AUTISM_FRIENDLY_DESIGN.md](../conversation/CIRCE_AUTISM_FRIENDLY_DESIGN.md):

| Requirement | Theme response |
|-------------|----------------|
| No flash on theme switch | 150 ms fade max |
| No sound on switch | Default silent |
| No auto-switch | Never |
| Low stimulation default | **Classic** default |
| Icon-first mode | Independent of theme (Phase 6) |

**Recommended themes for overload:**

1. Circe Classic  
2. Moonlit Obsidian  
3. Ocean Depths  

**Avoid suggesting during overload:** Eva-01, Fall Out of Time (high energy palette).

---

## Color-blind considerations

| Issue | Mitigation |
|-------|------------|
| Strand blocks rely on hue | Intensity → opacity + stroke weight (already in strand spec) |
| Focus not color-only | Border width + brightness shift |
| Red/green themes | Never use red=bad green=good semantics |

High Visibility yellow focus helps protanopia/deuteranopia.

---

## Encoder-only accessibility

Users who cannot use touch rely on:

- Visible focus ring (all themes)
- 48–56 px vertical targets
- Consistent focus order
- Back always reachable

Test Appearance screen with encoder only before release.

---

## Fatigue / low light

| Theme | Use case |
|-------|----------|
| Moonlit Obsidian | Night, low light |
| Classic | General |
| High Visibility | Eye strain needing contrast |
| Sunrise Recovery | Morning, gentle brightness |

No pure white `#FFFFFF` full-screen backgrounds in any theme except High Visibility text on black.

---

## Photosensitivity

| Avoid | Allow |
|-------|-------|
| Strobe on theme apply | Static fade |
| Pulsing accent | Static accent |
| CRT scanline animation (Fall Out) | Off by default, opt-in setting later |

---

## Testing checklist

- [ ] Each theme: home readable at arm's length on Watcher
- [ ] Focus visible on every screen
- [ ] High Visibility: 56 px buttons, no clip
- [ ] Delete confirm readable in all themes
- [ ] Theme switch without panic/reboot
- [ ] NVS persist across reboot

---

## Related

- [THEME_ARCHITECTURE.md](THEME_ARCHITECTURE.md)
- [../ui/ENCODER_FIRST_NAVIGATION.md](../ui/ENCODER_FIRST_NAVIGATION.md)
- [../ui/SAFE_AREA_SPEC.md](../ui/SAFE_AREA_SPEC.md)
