# Companion Interface Specification — Phase 6B

**Device:** SenseCAP Watcher 412×412  
**Experience target:** Premium emotional companion · cyberpunk HUD · digital guide

---

## Design philosophy (from concept image)

| Adopt | Avoid |
|-------|-------|
| Circular chrome framing the face | Rectangles filling the circle |
| Center as Circe's presence | Form fields and menus |
| Thin arcs and hairlines | Solid neon blocks |
| Calm, permission-giving copy | Clinical interrogatives |
| Quiet intelligence | Arcade RGB overload |

Reference aesthetics: **Ghost in the Code**, Hades Watch direction, premium wearables.

---

## Screen anatomy

Every primary screen uses the **Companion Shell**:

```
        ╭──── safe ring ────╮
       ╭┤  strand · voice   ├╮
      │ │  ┌─ viewport ─┐   │ │
      │ │  │ ○ Circe    │   │ │
      │ │  │ greeting   │   │ │
      │ │  │ prompt     │   │ │
      │ │  └────────────┘   │ │
       ╰┤  ○ Ready  ○ Quick ├╯
        ╰── Review · More ──╯
              status line
```

---

## Companion viewport

**Purpose:** Circe's conversation space — not a form container.

| Slot | Widget | Font | Visible when |
|------|--------|------|--------------|
| Presence | 28 px orb, accent ring | — | Always (companion mode) |
| Greeting | `heading` label | Montserrat 28 | Home |
| Prompt | `prompt` label | Montserrat 20 | Home + conversation |
| Response | `response` label | Montserrat 16 muted | Future user echo / voice |

### Home layout (slot wheel)

```
CIRCE / online

> ready when you are

   [ dim previous ]
   REGULATE          ← 28 px hero, centered
   [ dim next ]
     3 / 5

rotate select  press enter
```

Submenus still use vertical terminal rows in the bottom band.

### Conversation layout

```
○  presence orb
Not knowing is okay…   ← prompt only
                       ← response hidden until voice phase
```

---

## Action band

### Home — slot-wheel selector (2026-06-25)

Home uses a **centered rotary slot wheel** instead of a scrollable button column.

| Rotate to | Action |
|-----------|--------|
| BODY CHECK-IN | → Body flow |
| QUICK NOTE | → Quick presets |
| REGULATE | → Grounding / breathing |
| REVIEW | → Latest entry |
| SETTINGS | → Settings & tools |

Press to enter. Long-press opens SETTINGS from any selection.

See [HOME_SLOT_WHEEL_MENU.md](HOME_SLOT_WHEEL_MENU.md).

### Home — arc pill grid (legacy spec)

| Pill | Pattern key | Action |
|------|-------------|--------|
| Ready | `home.ready` | → Ready screen |
| Quick | `home.quick` | → Quick preset picker |
| Review | `home.review` | → Latest entry |
| More | `home.more` | → Settings & tools |

Pills: 112×52 px, full pill radius, theme border + focus glow.

### Ready screen

| Element | Copy key |
|---------|----------|
| Viewport prompt | `ready.prompt` — "Ready to check in?" |
| Primary action | `ready.body` — "Start with your body." |
| Back | `nav.back` |

### Other screens

Vertical stack in bottom band (240 px column centered) — unchanged for body/quick/review flows. Phase 6C may migrate to arc segments.

---

## Arc chrome

| Arc | Angle range | Purpose |
|-----|-------------|---------|
| Safe ring | 0°–360° | Wearable boundary guide @ 20% |
| Top | 35°–145° | Mood strand blocks |
| Left | 155°–225° | State chrome (future: privacy, mode) |
| Right | 315°–45° | Voice state indicator |

---

## Copy architecture

All user-facing strings originate from `circe_copy.c` pattern keys.

Future TTS **must** use the same keys as on-screen text (see [VOICE_STATE_UI.md](VOICE_STATE_UI.md)).

### Companion language principles

- Calm, supportive, private
- Short sentences
- Permission-giving ("or skip", "not knowing is okay")
- No corporate or clinical tone

### Phase 6B copy table

| Key | Text |
|-----|------|
| `home.heading` | Welcome back. |
| `home.prompt` | Ready to check in? |
| `home.subline` | Saved privately on this device. |
| `home.ready` | Ready |
| `ready.prompt` | Ready to check in? |
| `ready.body` | Start with your body. |
| `quick.one_tap` | Take a quick note. |
| `more.menu` | Settings & tools |

### Post-save reflection (MVP)

After worker save success, terminal screen `CIRCE_FLOW_REFLECTION`:

- Header: `SAVED` or `SESSION SAVED`
- Main: one rule-based observation (see `docs/reflection/REFLECTION_ENGINE_MVP.md`)
- Subline: optional grounding offer when intensity ≥ 8
- Actions: `REGULATE` · `REVIEW` · `HOME` (regulation: `REVIEW` · `HOME` only)

No cards, no scroll — same companion shell typography.

### Memory timeline (MVP)

REVIEW → category menu (TODAY / YESTERDAY / THIS WEEK / ALL ENTRIES) → worker load → encoder browse (fixed feed lines) → VIEW / DELETE / BACK. See `docs/memory/MEMORY_TIMELINE_MVP.md`.

---

## Theme integration

Viewport border, arc chrome, presence orb, and pill focus use theme tokens:

`accent_primary`, `accent_secondary`, `accent_muted`, `border`, `focus`, `surface`, `surface_alt`

Default starter theme (fresh NVS): **Neon Terminal** — black ground, `#39FF14` green, `#FF2BD6` magenta. Ghost in the Code and other presets remain selectable.

---

## Related

- [HUD_REBUILD_PLAN.md](HUD_REBUILD_PLAN.md)
- [CYBERPUNK_HUD_GUIDE.md](CYBERPUNK_HUD_GUIDE.md)
- [../voice/VOICE_ARCHITECTURE.md](../voice/VOICE_ARCHITECTURE.md)
