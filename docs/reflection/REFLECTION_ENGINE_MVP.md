# Reflection Engine MVP

Local, rule-based reflection shown after a successful save. No cloud, no ML, no diagnosis.

---

## Purpose

After the worker confirms a save, CIRCE shows a short observation from **recent patterns** (when available) or the **current entry**. The user can dismiss to HOME, open REVIEW, or start REGULATE when intensity suggests grounding.

See also: [RECENT_PATTERN_REFLECTION_MVP.md](RECENT_PATTERN_REFLECTION_MVP.md)

---

## Module

| File | Role |
|------|------|
| `firmware/circe/main/circe_reflection.h` | `circe_reflection_t`, generate API |
| `firmware/circe/main/circe_reflection.c` | Deterministic rule engine |

---

## UI flow

```
Entry Ready → SAVE → worker save success → CIRCE_FLOW_REFLECTION → REGULATE / REVIEW / HOME
```

Regulation saves:

```
Session save → worker success → Reflection (SESSION SAVED) → REVIEW / HOME
```

Edit-existing saves still go directly to REVIEW (no reflection).

Fallback if generation fails or entry is null:

> Saved. I can remember this with you.

---

## Rules (priority)

### Body check-in (`entry_mode` body / quick)

1. **Body area present** — `I noticed your [area] carried this entry.`
2. **Emotional tone** (not UNKNOWN / not skipped) — `I saved this as [label].`
3. **Custom color** (`color_source` = `touch_picker`) — `I saved the color you chose: [hex].`
4. **Preset color** (`color_source` = `preset`) — `I saved this color as [label] [hex].`
5. **High intensity** (`intensity >= 8`) — `That signal was strong. I saved it.`
6. **Tone unknown / skipped** — `Not knowing is allowed. I saved what you noticed.`
7. **Fallback** — `Saved. I can remember this with you.`

**Subline**

- Intensity ≥ 8: `Would a grounding sequence help?` (shows REGULATE button)
- Otherwise optional: `We can return to this later.` or empty

### Regulation (`entry_mode` = regulation)

- **Session completed** — `Session complete. I saved this regulation entry.`
- **Otherwise** — `Session saved. You stayed with it for [N] seconds.`
- Subline: `We can return to this later.`
- No REGULATE suggestion on regulation reflection

---

## Recent history

**Implemented (2026-06-24).** Worker loads up to 16 entries (THIS WEEK, or ALL if time unset) after save. Pattern engine may emit one observation; otherwise immediate rules apply. See [RECENT_PATTERN_REFLECTION_MVP.md](RECENT_PATTERN_REFLECTION_MVP.md).

---

## Persistence

Reflection text is **not** written to entry JSON in this phase. No schema change required.

Optional future fields (not implemented):

- `reflection_text`
- `reflection_type` = `immediate`
- `reflection_generated_by` = `rule_engine`

---

## Language guardrails

CIRCE observes; it does not diagnose.

**Use:** I noticed…, I saved…, Not knowing is allowed., Would a grounding sequence help?

**Avoid:** You are anxious., This means…, You should…, disorder, treatment, medical certainty.

---

## Privacy

All reflection runs on-device from the saved entry copy returned in the worker completion. No network. No history scan in MVP.
