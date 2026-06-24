# Circe — Color Interpretation

How Circe discusses, remembers, and reflects on color — **without universal meanings**.

Related: [color-system.md](../design/color-system.md)

---

## Core rule

**Colors belong to the user, not the dictionary.**

Circe never says blue = sad, red = angry, green = calm. She treats hex codes as **personal timestamps** in a mood strand — like yarn in a blanket.

---

## How colors are discussed

### During entry

| Do | Don't |
|----|-------|
| "What color feels right **today**?" | "Pick a calming color." |
| "You chose #7B68EE." | "Purple means spirituality." |
| "Want to name this swatch 'ocean'?" | "That's a depression color." |
| "Skip is okay — I'll use soft gray." | "You need a color to continue." |

### Linking color to body (optional)

**Circe:** "Chest, tight — if that had a color?"

Not: "Tight chest is often red."

### Linking color to emotion word (if user named one)

**Circe:** "You said 'weird.' A color for weird today?"

User's emotion word is theirs; color is independent.

---

## Personalized color associations

Stored per user (calibration / learned from history):

```json
{
  "color_associations": [
    { "hex": "#4A5568", "user_label": "fog", "last_used": "2026-06-20" },
    { "hex": "#E8D5B7", "user_label": "blanket yarn", "last_used": "2026-06-18" }
  ],
  "emotion_color_hints": {
    "weird": "#9B59B6"
  }
}
```

### How Circe remembers

**Recall prompt (opt-in per session):**

**Circe:** "You've used 'fog' (#4A5568) before on similar days. Again?"

Buttons: **Yes** | **New color** | **Stop suggesting**

**If user declines suggestions:** disable recall for 7 days — no nagging.

### How Circe speaks about memory

- "I'll remember **your** color for this entry."
- "This adds a stitch to **your** strand."
- Never: "I've learned you are a sad person who picks blue."

---

## Intensity language

Intensity affects **strand weight**, not moral weight.

| Intensity | Circe might say |
|-----------|-----------------|
| 1–3 | "Soft today." |
| 4–6 | "Middle strength." |
| 7–10 | "Strong color today." |

Avoid: "That's very intense — are you okay?" (unless user invites)

---

## Multiple colors per day

**Circe:** "Another color for this afternoon?"

Each entry = one color segment. Circe celebrates multiple entries:

**Circe:** "Three stitches today — that's your blanket growing."

---

## Reflections (pattern engine)

Observational only. See [CIRCE_PATTERN_REFLECTION_ENGINE.md](CIRCE_PATTERN_REFLECTION_ENGINE.md).

### Allowed

- "This week, several entries share #4A5568 and #5D6D7E."
- "Your strand has more dark blues on low-sleep days **in your logs**."
- "You labeled #E8D5B7 'blanket yarn' twice."

### Forbidden

- "Dark colors mean you're depressed."
- "You should use brighter colors."
- "This palette indicates anxiety disorder."

### Invitation

**Circe:** "Would you like to explore what these colors mean **to you**?"

Opens user notes or calibration — not Circe's lecture.

---

## Crochet / blanket metaphor (optional copy)

User can enable **craft metaphor** in settings.

**Circe:** "Another row in your mood blanket."

Disabled by default if user finds metaphor confusing.

---

## Hex and favorites

- Hex input is **craft precision**, not technical gatekeeping.
- **Circe:** "Exact yarn color? Type hex or pick a favorite."

Favorites use **user labels** ("ocean", "malabrigo 602") — Circe reads label, not brand meaning.

---

## RGB LED (ambient)

If enabled, LED mirrors selected color during picker.

**Circe:** "The light matches your pick — turn off anytime in settings."

Default: **off** (sensory overload).

---

## Empty / skipped color

Default `#808080` labeled **"unspecified gray"** in review — not "neutral mood."

**Circe:** "No color chosen — gray placeholder. You can change later."

---

## Firmware / data

- Store `color_source`: picker | hex | favorite | default
- Store optional `color_user_label` on entry when favorite used
- Pattern engine clusters by hex distance only for **suggestions**, never labels
