# CIRCE Theme Color Palettes

Ten local themes — hex values for LVGL `lv_color_hex()`.

**Default:** Circe Classic  
**Accessibility:** High Visibility (also usable by anyone)

No copyrighted logos or franchise artwork — **palette inspiration only** for Eva-01, Terminal Kitty, Ghost in the Code.

---

## Token legend

| Token | Role |
|-------|------|
| `bg` | Screen background |
| `surface` | Button / card fill |
| `surface_focus` | Encoder focus fill |
| `text` | Primary copy |
| `muted` | Status, hints |
| `accent` | Focus ring, highlights |
| `void` | Empty strand block |
| `danger` | Delete confirm (muted) |

---

## 1. CIRCE CLASSIC (default)

**Mood:** safe, gentle, reflective

| Token | Hex |
|-------|-----|
| bg | `#2D3748` |
| surface | `#4A5568` |
| surface_focus | `#5A6578` |
| text | `#F7FAFC` |
| muted | `#A0AEC0` |
| accent | `#9F7AEA` |
| void | `#1A202C` |
| danger | `#C05656` |

Soft slate + muted lavender accent. Low stimulation.

---

## 2. GHOST IN THE CODE

**Mood:** haunted technology, digital archive, cyber-occult  
Inspired by Hades Watch aesthetic direction.

| Token | Hex |
|-------|-----|
| bg | `#0A0A0F` |
| surface | `#1A1A24` |
| surface_focus | `#252532` |
| text | `#C0C0C8` |
| muted | `#6B6B7B` |
| accent | `#00E5FF` |
| void | `#12121A` |
| danger | `#8B4040` |

Terminal glow cyan on charcoal. Use monospace buttons optional.

---

## 3. TERMINAL KITTY

**Mood:** playful hacker, retro terminal, cozy cyberpunk

| Token | Hex |
|-------|-----|
| bg | `#0D0D0D` |
| surface | `#1E1E1E` |
| surface_focus | `#2A2A2A` |
| text | `#E8E8E8` |
| muted | `#888888` |
| accent | `#FF6EC7` |
| accent_alt | `#7DF9FF` |
| void | `#141414` |
| danger | `#994444` |

Neon pink primary accent; cyan for secondary focus ring alternate.

---

## 4. EVA-01

**Mood:** futuristic, energetic, high contrast  
**Palette only** — no logos, no NERV imagery.

| Token | Hex |
|-------|-----|
| bg | `#1A0A2E` |
| surface | `#2D1B4E` |
| surface_focus | `#3D2660` |
| text | `#E0E0E0` |
| muted | `#9B8FB8` |
| accent | `#39FF14` |
| void | `#120820` |
| danger | `#AA4444` |

Deep purple + neon green HUD accent. Higher contrast — not default for overload flows.

---

## 5. FALL OUT OF TIME

**Mood:** retro survival terminal

| Token | Hex |
|-------|-----|
| bg | `#0A0F0A` |
| surface | `#142014` |
| surface_focus | `#1A2A1A` |
| text | `#33FF33` |
| muted | `#228822` |
| accent | `#FFB000` |
| void | `#081008` |
| danger | `#AA5500` |

CRT green primary; amber for accent/warnings. Optional scanline overlay **off** by default (motion).

---

## 6. MOONLIT OBSIDIAN

**Mood:** night reflection, quiet solitude

| Token | Hex |
|-------|-----|
| bg | `#0B0E14` |
| surface | `#151A24` |
| surface_focus | `#1E2433` |
| text | `#E8ECF4` |
| muted | `#7A8499` |
| accent | `#8B9DC3` |
| void | `#080A10` |
| danger | `#884444` |

Black-indigo base, moon-white text, silver accent.

---

## 7. SUNRISE RECOVERY

**Mood:** hopeful, gentle, supportive

| Token | Hex |
|-------|-----|
| bg | `#3D2E28` |
| surface | `#5C4A42` |
| surface_focus | `#6E5A50` |
| text | `#FFF5EE` |
| muted | `#D4B8A8` |
| accent | `#F6AD55` |
| void | `#2A201C` |
| danger | `#B56565` |

Peach, warm gold, cream — highest warmth. Good for morning check-ins.

---

## 8. FOREST SIGNAL

**Mood:** grounding, nature, stability

| Token | Hex |
|-------|-----|
| bg | `#1A2F1A` |
| surface | `#2D4A2D` |
| surface_focus | `#3A5C3A` |
| text | `#E8F0E8` |
| muted | `#8FA88F` |
| accent | `#4FD1C5` |
| void | `#122012` |
| danger | `#996655` |

Deep green + moss; teal accent. Pairs with tree-ring strand concept.

---

## 9. OCEAN DEPTHS

**Mood:** calm, breathing, flow

| Token | Hex |
|-------|-----|
| bg | `#0A1628` |
| surface | `#1A3050` |
| surface_focus | `#243D60` |
| text | `#E6F0FA` |
| muted | `#7A9BB8` |
| accent | `#4ECDC4` |
| void | `#081020` |
| danger | `#886666` |

Navy to turquoise — breathing rhythm friendly.

---

## 10. HIGH VISIBILITY

**Mood:** accessibility, fatigue, low light  
**Designed for:** visual accessibility, not aesthetics alone.

| Token | Hex |
|-------|-----|
| bg | `#000000` |
| surface | `#1A1A1A` |
| surface_focus | `#FFFF00` |
| text | `#FFFFFF` |
| muted | `#CCCCCC` |
| accent | `#FFFF00` |
| void | `#333333` |
| danger | `#FF6666` |

| Override | Value |
|----------|-------|
| font_prompt | 22 px |
| font_button | 20 px |
| btn_min_h | 56 px |
| btn_radius | 8 px |
| border width | 2 px always |

Maximum contrast. Yellow focus ring. No thin gray-on-gray.

---

## Theme selection guidance

| User state | Suggested theme |
|------------|-----------------|
| Default / first use | Classic |
| Overload / night | Moonlit Obsidian or Classic |
| Need contrast | High Visibility |
| Playful comfort | Terminal Kitty |
| Grounding | Forest Signal |
| Morning hope | Sunrise Recovery |

Circe never auto-switches theme based on mood inference.

---

## Related

- [THEME_ARCHITECTURE.md](THEME_ARCHITECTURE.md)
- [THEME_ACCESSIBILITY_GUIDE.md](THEME_ACCESSIBILITY_GUIDE.md)
