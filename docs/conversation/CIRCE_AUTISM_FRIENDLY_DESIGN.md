# Circe — Autism-Friendly Design

Interaction rules for autistic users — including burnout, shutdown, overstimulation, low verbal bandwidth, decision fatigue, and sensory overload.

**Circe adapts to capacity; she does not measure it.**

---

## Design principles

1. **Fewer words when energy is low.**
2. **Icons before sentences when possible.**
3. **One decision per screen when overloaded.**
4. **Every screen: Skip, Back, Save.**
5. **No surprise sounds, flashes, or RGB pulses** during distress flows (disable ambient LED).
6. **User's "no" is complete** — no "are you sure?" loops except discard draft.

---

## State detection (how Circe adapts)

| Signal | Source | Adaptation |
|--------|--------|------------|
| User selects Low-energy mode | Settings or prompt | Short-answer + icon-first |
| Tags `overstimulated`, `shutdown_feeling`, `meltdown_warning` | Body step | Auto-suggest private save; reduce copy |
| Idle timeout mid-flow | UI | "Continue or save partial?" |
| Time of day + history | Optional | Offer quick entry (no guilt) |
| User enables Fast-entry default | Calibration | Skip conversation screens |

Circe **never says** "you seem autistic" or "you're overloaded" — she **offers modes**.

---

## Autistic burnout

**User experience:** Flat affect, reduced capacity, everything costs spoons.

**Circe behavior:**

- Shorter greetings: "Hi. Tap one thing or save."
- Hide ratings, context, summary, photo by default in **Low-energy mode**.
- Max 2 screens to save: body chips OR color OR both.

**Example:**

**Circe:** "Low day?"

Buttons: **Quick save** | **Body tap** | **Color only**

---

## Shutdown

**User experience:** Reduced speech, reduced motor planning, need minimal interaction.

**Circe behavior:**

- **Icon-first mode:** body area icons only, no paragraph text.
- Encoder: one focus per screen; wheel press = confirm.
- Auto-enable `private_locked` (already default).
- Optional: single **Shutdown entry** button on home — logs `shutdown_feeling` + timestamp, skip all else.

**Example flow (3 taps):**

1. Home → **Shutdown check-in** (icon)
2. Confirm **whole body** + **shutdown_feeling** (preselected)
3. **Save privately**

**Circe:** "Saved. Rest."

---

## Overstimulation

**User experience:** Sensory input feels too much; device itself can overwhelm.

**Circe behavior:**

- Disable animation (rlottie) in this mode.
- Mute Circe voice if voice enabled.
- Dim display brightness suggestion (system setting hook — firmware).
- Reduce color picker to **favorites only** (6 swatches max).
- No camera prompt.

**Example:**

User tags `overstimulated`:

**Circe:** "Quiet save?"

Buttons: **Save tags** | **Add one color** | **Done**

---

## Low verbal bandwidth

**User experience:** Reading or composing text is hard.

**Circe behavior:**

- **Short-answer mode:** all Circe lines ≤ 80 characters.
- No free-text summary unless user opens it.
- Emotion = chip list, not typing.
- Voice entry (future) as alternative to text summary.

**Example lines:**

| Standard | Short-answer |
|----------|--------------|
| "Would it be easier to start with your body?" | "Body first?" |
| "If this moment had a color today, what would it be?" | "Pick a color." |
| "Would you like me to remember this moment?" | "Save?" |

---

## Decision fatigue

**User experience:** Too many choices → freeze.

**Circe behavior:**

- **Progressive disclosure:** show 6 sensation chips; "More" expands rest.
- **Smart defaults:** intensity 5, gray color, private on.
- **Recommended next** single button: "Continue with defaults" on optional steps.
- Limit context tags to user's **favorites** (max 8) in fast modes.

**Example (ratings step skipped by default):**

**Circe:** "Skip sleep and stress today?"

Buttons: **Skip** (default focus) | **Add ratings**

---

## Sensory overload (device + environment)

| Overload source | Mitigation |
|-----------------|------------|
| Bright screen | Low-brightness theme for Circe |
| Touch uncertainty | List fallback alongside body map |
| Encoder detents | Clear focus ring |
| Speaker TTS | Off in overstim mode |
| RGB LED | Off during entry |
| Camera flash | No flash; optional step off |

---

## Interaction modes (summary)

### Short-answer mode

- Copy from short-answer column in patterns.
- Max 1 question per screen.
- Buttons: 2–3 only.

### Icon-first mode

- Greeting: icons for body / color / save / shutdown.
- Minimal text (labels under icons for accessibility).
- Suitable for shutdown and non-speaking days.

### Low-energy mode

- Subset of full flow: body OR color → save.
- No photo, no summary, no training prompt (defaults apply silently).
- Circe lines ≤ 80 chars.

### Fast-entry mode

- Target: **≤ 30 seconds**, **≤ 4 taps**.
- See [CIRCE_ENTRY_TYPES.md](CIRCE_ENTRY_TYPES.md).

---

## Mode selection UX

**On home screen:**

| Control | Action |
|---------|--------|
| Full reflection | Standard flow |
| Quick | Fast-entry |
| Quiet | Low-energy + icon-first |
| Settings → default mode | Persists in calibration |

**Circe:** "Which kind of check-in today?"

Icons: 🫀 body | 🎨 color | ⚡ quick | 🌙 quiet

(Use accessible labels, not emoji-only in production UI.)

---

## What Circe avoids in autism-friendly contexts

- Rhetorical questions stacked back-to-back
- "How does that make you feel?" after body tags
- Forced eye-contact metaphors
- Social judgment ("Did you talk to anyone?")
- Bright celebratory feedback on save
- Countdown timers

---

## Calibration hooks

User configures in calibration_mode:

- Default interaction mode
- Hidden sensation tags (reduce list)
- Favorite quick tags (e.g. overstimulated, tight, numb)
- Disable photo prompts globally
- Enable shutdown one-tap entry

---

## Firmware implications

- `entry_mode` enum on each entry: `full`, `quick`, `body_only`, `color`, `photo`, `voice`, `shutdown`
- `interaction_mode` flags: `short_answer`, `icon_first`, `low_energy`
- Copy resolver: `(pattern_key, interaction_mode) → string`
- Conditional step graph: skip nodes based on mode flags
