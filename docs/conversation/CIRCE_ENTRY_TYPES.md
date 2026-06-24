# Circe — Entry Types

Specialized entry modes compared by use case, flow, and Circe behavior.

Each entry stores `entry_mode` (proposed schema field) plus standard `EmotionEntry` fields.

---

## Mode comparison

| Mode | Target time | Taps | Circe talk | Photo | Emotion required |
|------|-------------|------|------------|-------|------------------|
| **Quick Entry** | 15–30 s | 2–4 | Minimal | No | No |
| **Body-Only Entry** | 30–60 s | 3–6 | Short | No | No |
| **Color Entry** | 20–40 s | 2–5 | Short | No | No |
| **Full Reflection Entry** | 2–5 min | 12+ | Full | Soft offer | No (skippable) |
| **Photo Entry** | 1–2 min | 4–8 | Medium | **Yes** | No |
| **Voice Entry** | 1–3 min | 3–6 | Spoken | Optional | No |

---

## 1. Quick Entry

**Use when:** Decision fatigue, between tasks, need a strand stitch without narrative.

**User story:** "I just need to log that today feels like mud."

### Flow

1. Home → **Quick**
2. Pick **one**: body quick-tags OR color favorite OR mood phrase chip
3. Save privately (defaults silent)

### Circe

**Circe:** "One tap saves."

Optional single line: "Got it."

### Data

- `entry_mode: quick`
- `completion_status: partial`
- May set only `color_hex` OR only `body_sensations[]` OR both

### Advantages

- Lowest friction
- Builds mood strand over time
- Autism-friendly default for burnout

### Disadvantages

- Less context for pattern engine
- No photo/ratings

---

## 2. Body-Only Entry

**Use when:** Emotion words unavailable; somatic awareness is the whole point.

**User story:** "My chest is tight and I don't want to name anything else."

### Flow

1. Greeting → **Body first** OR phrase "I don't know what I feel"
2. Body areas + sensations
3. Optional color
4. Save (skip emotion, ratings, photo)

### Circe

Follow [CIRCE_BODY_FIRST_FLOW.md](CIRCE_BODY_FIRST_FLOW.md).

**Close:** "Body saved. No emotion required."

### Data

- `entry_mode: body_only`
- `flow_path: body_first`
- `emotion_skipped: true`

### Advantages

- Honors core design requirement
- Rich body data for patterns
- No misleading emotion labels

### Disadvantages

- No strand color if user skips color step

---

## 3. Color Entry

**Use when:** User thinks in yarn/weather-blanket colors; body words hard today.

**User story:** "Today is slate blue. That's the whole entry."

### Flow

1. Home → **Color**
2. Picker / favorite / hex
3. Intensity (optional skip)
4. Save

### Circe

**Circe:** "Pick today's color."

**Close:** "One stitch in your strand."

### Data

- `entry_mode: color`
- `body_areas: []` allowed
- `color_hex` required

### Advantages

- Fast strand contribution
- Craft tracking alignment
- Low verbal load

### Disadvantages

- Minimal body correlation data

---

## 4. Full Reflection Entry

**Use when:** Capacity for full check-in; end of day; processing complex day.

**User story:** "I want the whole conversation."

### Flow

Standard [user-flow.md](../design/user-flow.md) with body-first option at check-in.

### Circe

Full [CIRCE_CONVERSATION_PATTERNS.md](CIRCE_CONVERSATION_PATTERNS.md).

### Data

- `entry_mode: full`
- `completion_status: complete` if user finishes all optional steps they choose

### Advantages

- Richest dataset for personal patterns
- Complete review experience

### Disadvantages

- Long; not for shutdown days
- Higher abandonment risk without save shortcuts

---

## 5. Photo Entry

**Use when:** Visual memory is the anchor — sunset, workspace, face, object.

**User story:** "This scene is how I feel."

### Flow

1. Home → **Photo entry**
2. Camera capture (required for this mode)
3. Optional: one body tag OR color OR single word
4. Save private

### Circe

**Circe:** "This entry is for a photo. Ready?"

After: "Want to add a color or body tag, or save photo alone?"

### Data

- `entry_mode: photo`
- `photo_path` required

### Advantages

- Strong episodic memory
- Future ML (consented export only)

### Disadvantages

- Camera may overwhelm when overstimulated
- Storage use

---

## 6. Voice Entry (future)

**Use when:** Typing/tapping hard; speaking is easier.

**User story:** "I'll say it out loud."

### Flow

1. Home → **Voice**
2. Hold-to-record (Watcher mic → Hades STT or on-device if available)
3. Circe confirms extracted tags OR user corrects on screen
4. Save

### Circe

Spoken prompts per [CIRCE_VOICE_DESIGN.md](CIRCE_VOICE_DESIGN.md).

**Circe:** "Say a few words — or name a body sensation."

### Data

- `entry_mode: voice`
- `assistant_transcript` with audio path optional
- Requires explicit consent for audio in exports

### Advantages

- Low motor + reading load
- Natural for some autistic users

### Disadvantages

- Privacy complexity (audio files)
- Depends on Hades/local STT
- Not Phase 2

---

## Mode selection on home

**Circe:** "What kind of check-in?"

| Button | Mode |
|--------|------|
| Quick | quick |
| Body | body_only |
| Color | color |
| Full | full |
| Photo | photo |
| Voice | voice (future, grayed) |

User default in calibration overrides which button is focused.

---

## Recommended defaults (see Phase 1.5 report)

| Recommendation | Mode |
|----------------|------|
| Default entry | **Body-Only Entry** (with color offer) |
| Fast-entry | **Quick Entry** |
| Body-first path | **Body-Only Entry** flow from ambiguous phrases |

Rationale in [PHASE-1.5-REPORT.md](../PHASE-1.5-REPORT.md).

---

## Schema addition (proposed)

Add to `EmotionEntry`:

```json
{
  "entry_mode": "quick | body_only | color | full | photo | voice",
  "interaction_mode": {
    "short_answer": false,
    "icon_first": false,
    "low_energy": false
  }
}
```

Update [emotion-entry.schema.json](../../schemas/emotion-entry.schema.json) in Phase 2 when implementing.
