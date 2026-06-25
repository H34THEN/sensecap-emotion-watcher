# Grounding + Breathing MVP

**Status:** Implemented in firmware (`circe_regulation.c`)

Offline regulation tools for in-the-moment body support. Not medical intervention — grounding, breathing pace, and body anchor language only.

Optional **voice cues** (Settings → SOFT): gentle tones on session start, breathing phase changes, and completion. Sensory reset stays silent. See `docs/voice/VOICE_PERSONALITY_MVP.md`.

---

## Home entry

Home menu includes **REGULATE** (scrollable list):

```
BODY CHECK-IN
QUICK NOTE
REGULATE
REVIEW
SETTINGS
```

---

## Grounding screen

Terminal feed:

```
> grounding sequence
> notice one thing you can feel
> breathe when ready
```

Options:

| Button | Action |
|--------|--------|
| BREATHING | Start paced breathing pacer |
| BODY ANCHOR | Start body anchor prompt sequence |
| SAVE NOTE | Opens body check-in (optional journal) |
| BACK | Home |

---

## Breathing MVP

**Cycle:** INHALE 4 → HOLD 2 → EXHALE 6  
**Default:** 3 rounds

Visual (objects created once):

- Subtle ring arc + pulsing orb
- Phase label (`INHALE` / `HOLD` / `EXHALE`)
- Countdown number

**Encoder controls:**

| Input | Action |
|-------|--------|
| Press | Pause / resume |
| Double press | Back to grounding menu |
| Long press | End session → optional save |

No audio. No SD/worker during timer ticks. LVGL timer updates labels/styles only (1 Hz).

---

## Body anchor MVP

Prompt sequence (rotate with encoder, press to advance):

1. find contact with the ground
2. notice your hands
3. soften your jaw if you can
4. name one safe object nearby

**Encoder controls:**

| Input | Action |
|-------|--------|
| Rotate | Previous / next prompt |
| Press | Next prompt (complete on last) |
| Double press | Back |
| Long press | End session → optional save |

---

## Optional session save

After breathing or body anchor completes (or long-press end):

```
save this session?
```

| Button | Action |
|--------|--------|
| SAVE | Worker save via existing path |
| SKIP | Return home |
| BACK | Grounding menu |

---

## Entry JSON fields

When `entry_mode = "regulation"`:

| Field | Example |
|-------|---------|
| `entry_mode` | `regulation` |
| `regulation_type` | `breathing` or `body_anchor` |
| `rounds_completed` | `3` |
| `duration_seconds` | `72` |
| `session_completed` | `true` |
| `emotion` | `unknown` |
| `training_ok` | `false` |
| `private_locked` | `true` |
| `color_hex` | `null` (skipped) |

Existing journal entries unchanged.

---

## Review display

```
regulation breathing
duration 72s rounds 3
completed yes
```

---

## Performance

| Rule | Implementation |
|------|----------------|
| No SD during ticks | Timer callback only updates UI |
| No worker during ticks | Save only after user confirms |
| Fixed object budget | ~5 LVGL objects breathing, ~2 body anchor |
| Destroy on exit | `circe_regulation_destroy()` in `clear_content()` |

---

## Out of scope

Voice, audio cues, camera, ML, cloud, Strand, medical claims.

---

## Related

- [PHASE-GROUNDING-BREATHING-MVP-REPORT.md](../PHASE-GROUNDING-BREATHING-MVP-REPORT.md)
- [ENTRY_LIFECYCLE.md](../memory/ENTRY_LIFECYCLE.md)
- [CONVERSATION_ENGINE_COPY_POLISH.md](../conversation/CONVERSATION_ENGINE_COPY_POLISH.md)

---

## Copy polish + review (2026-06-24)

Grounding, breathing, and anchor intro feeds use `reg.grounding_*`, `reg.breath_*`, `reg.anchor_*` copy keys. Anchor rotation prompts: find one contact point · notice your hands · soften what can soften · name one safe object nearby.

**Regulation review:** DELETE / BACK / HOME only — body EDIT actions hidden for regulation entries.

**Library expansion (2026-06-24):** See [REGULATION_LIBRARY_EXPANSION_MVP.md](REGULATION_LIBRARY_EXPANSION_MVP.md) — 5-4-3-2-1, sensory reset, bilateral tap.
