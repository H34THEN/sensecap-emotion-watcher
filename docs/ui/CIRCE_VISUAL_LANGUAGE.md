# CIRCE Visual Language

Visual identity for the standalone Watcher experience.

CIRCE should feel **warm, supportive, private, calm** — never diagnostic, clinical, or medicalized.

---

## Design pillars

| Pillar | Expression |
|--------|------------|
| **Warm** | Soft backgrounds, rounded corners, ivory/slate tones |
| **Supportive** | Short copy, permission to skip, no judgment |
| **Private** | Lock icon subtle; privacy line calm, not alarming |
| **Calm** | Low contrast motion, no flash, no alarm red as default |

---

## Typography

| Role | Size (412 display) | Weight | Notes |
|------|-------------------|--------|-------|
| Circe prompt | 18–20 px | Regular | Max 2 lines on home; 3 elsewhere |
| Button label | 16–18 px | Medium | Sentence case, not ALL CAPS |
| Status / privacy | 14 px | Regular | Muted color |
| Review metadata | 14 px | Regular | Prefer icons over dense text |
| Diagnostics | 14 px | Regular | Split long text |

**Font:** LVGL Montserrat (factory default family) — use **18 px minimum** for body text in High Visibility theme.

**Avoid:** monospace for emotional prompts (reserve for Ghost in the Code / Terminal themes only).

---

## Spacing scale

| Token | px | Use |
|-------|-----|-----|
| `space-xs` | 4 | Icon padding |
| `space-sm` | 8 | Between buttons |
| `space-md` | 16 | Section gaps |
| `space-lg` | 24 | Prompt to content |
| `space-xl` | 32 | Screen edge inset |

Button stack: **8 px** gap minimum (motor accessibility).

---

## Shape language

| Element | Radius | Notes |
|---------|--------|-------|
| Buttons | 12 px | Soft pill feel |
| Strand blocks | 4 px | Small color chips |
| Screen container | 0 (full) | Circular clip at root |
| Focus ring | 2 px border | Theme accent |

No sharp 0-radius rectangles except intentional terminal themes.

---

## Icon direction (Phase 5+)

Prefer simple line icons over emoji:

| Action | Icon concept |
|--------|--------------|
| Body | Outline figure / torso |
| Quick | Single dot / lightning-soft |
| Review | Open book / eye |
| More | Three dots horizontal |
| Save | Soft check |
| Back | Chevron left |
| Private | Small lock, muted |

Icons optional on home if labels stay — autism-friendly doc allows icon-first mode.

---

## Color usage (semantic, not diagnostic)

Colors describe **moments**, not disorders.

| Semantic | Usage |
|----------|--------|
| `background` | Screen base |
| `surface` | Buttons, cards |
| `text_primary` | Prompts |
| `text_muted` | Status, hints |
| `accent` | Focus ring, primary CTA border |
| `strand_void` | Missing color block |
| `danger` | Delete confirm only — muted brick, not alarm red |

Entry `color_hex` is user-chosen — **never** mapped to "you are anxious."

---

## Motion

| Allowed | Forbidden |
|---------|-----------|
| 150 ms fade between screens | Bounce, shake |
| Soft opacity on focus | Flashing privacy banner |
| None on save | Celebratory confetti |

Align with autism-friendly: **no surprise motion** in default themes.

---

## Transitions

```
Screen change: cross-fade 120–150 ms OR instant for Quick path
Save done: static confirmation — no slide animation
Delete: instant — no dramatic wipe
Theme switch: 200 ms fade entire screen
```

---

## Tone vs visuals

Copy is warm; visuals must match:

| Copy | Visual must not contradict |
|------|---------------------------|
| "Not knowing is okay." | No red error styling |
| "Saved privately." | No cloud upload icon |
| "One tap saves." | Single large target |

---

## Anti-patterns (never)

- Stethoscope, brain scan, medical cross imagery
- Emotion → diagnosis labels on screen
- Traffic-light mood scoring (red=bad day)
- Gamification streaks / shame for missed days
- Pure `#FFFFFF` background at full brightness

---

## Theme relationship

Visual language is **theme-aware** but structure stays constant:

- Layout tokens fixed across themes
- Only palette + typography scale change
- High Visibility theme increases sizes, not layout logic

See [themes/THEME_COLOR_PALETTES.md](../themes/THEME_COLOR_PALETTES.md).

---

## Related

- [conversation/CIRCE_CORE_PERSONALITY.md](../conversation/CIRCE_CORE_PERSONALITY.md)
- [conversation/CIRCE_AUTISM_FRIENDLY_DESIGN.md](../conversation/CIRCE_AUTISM_FRIENDLY_DESIGN.md)
- [design/color-system.md](../design/color-system.md)
