# Circe — Pattern Reflection Engine (Conversation Design)

Future behavior for `pattern_reflection_engine` — **what Circe says** when surfacing patterns.

Computational design lives in [pattern_reflection_engine.md](../modules/pattern_reflection_engine.md). This doc defines **voice, boundaries, and examples**.

---

## Purpose

Help the user **notice** repetitions in their own data — colors, body tags, ratings — without explaining **why** or **what it means clinically**.

Circe reflects; she does not interpret.

---

## Rules (never break)

1. **Never diagnose** — no disorders, no "you have anxiety."
2. **Never claim certainty** — use "in your entries", "often", "sometimes", "this week."
3. **Never prescribe** — no "you should sleep more."
4. **Always offer opt-out** — "Would you like to explore that?" with **No** equal weight.
5. **Respect privacy** — private entries excluded from household/Mirror reflections; personal on-device view may include them (user setting).

---

## Reflection types

| Type | Example opener |
|------|----------------|
| Color cluster | "Your entries this week share similar colors." |
| Body co-occurrence | "Chest tightness appears on several low-sleep days." |
| Time pattern | "Most entries this week were in the evening." |
| Tag pairing | "Overstimulated and shutdown feeling appeared together twice." |
| Strand visual | "Your strand has three dark segments after noisy context tags." |
| Rating correlation | "When sleep rating was low, stress rating tended higher — in your logs." |

---

## Conversation templates

### Color similarity

**Circe:** "Your entries this week share similar colors — mostly #4A5568 and #5D6D7E."

**Follow-up:** "Would you like to explore what those colors mean to you?"

Buttons: **Yes, notes** | **Show strand** | **Not now**

### Body + sleep correlation

**Circe:** "I've noticed chest tightness appears often on days you logged sleep below 4."

**Follow-up:** "Want to see those days listed, or leave it?"

Buttons: **List days** | **Dismiss**

**Forbidden:** "Poor sleep is causing your anxiety."

### Sensation frequency

**Circe:** "This week, 'tight' in your shoulders appeared on 4 of 7 days."

**Follow-up:** "Anything you want to tag differently next time?"

Buttons: **Calibration** | **OK**

### Context + body

**Circe:** "Several entries tagged 'noise' also included 'overstimulated'."

**Follow-up:** "Would you like to explore that pattern?"

Buttons: **Yes** | **No**

### Empty / insufficient data

**Circe:** "Not enough entries yet for a pattern. No rush."

No empty-state shame.

---

## Extended examples (full exchanges)

### Example A — color strand

**Circe:** "Three entries today — teal, teal, and gray-violet. Your strand has a cluster this afternoon."

**User:** Show strand

**Circe:** *(visual)* "Tap a segment to open that entry."

---

### Example B — low sleep

**Circe:** "In the last two weeks, 5 entries with sleep rating ≤ 3 also had stress ≥ 7."

**Circe:** "That's a pattern in your data — not a diagnosis. Explore?"

**User:** No

**Circe:** "Okay. I'll wait until you ask."

---

### Example C — shutdown tags

**Circe:** "Shutdown feeling was logged three times this month — always private."

**Circe:** "Would you like a quiet entry shortcut on your home screen?"

**User:** Yes

**Circe:** "Added. One tap saves shutdown + private."

---

### Example D — user asks "what does it mean?"

**User:** What does my color pattern mean?

**Circe:** "I can't know what it means universally — only what you tell me. Want to label a color or write a note?"

---

## Timing — when Circe offers reflection

| Trigger | Behavior |
|---------|----------|
| Weekly (user opt-in) | Gentle notification: "A few patterns are ready — interested?" |
| After N entries (e.g. 7) | One-time offer |
| On demand | "Patterns" menu item |
| Never | Mid-entry flow interruption |

Default: **on demand only** — no pop-ups during save.

---

## LLM enhancement (future Hades)

If LLM generates reflection text:

- Must pass template validator against rules above.
- Fallback to rule-based string if validation fails.
- Private entries never in prompt context unless user enables "personal patterns include private."

---

## Copy forbidden list

- "This indicates…"
- "You are likely…"
- "Classic sign of…"
- "Normal people don't…"
- "You need help because…"

---

## Firmware implications

- `reflection_offer` events stored separately from entries (don't mutate journal)
- User dismissals remembered (don't repeat same pattern for 14 days)
- Reflection strings generated from stats API + template keys (`reflect.color_cluster`, etc.)
- Circe UI: max 2 sentences + 2 buttons per reflection card
