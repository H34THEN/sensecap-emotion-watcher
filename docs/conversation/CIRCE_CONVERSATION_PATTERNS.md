# Circe — Conversation Patterns

Reusable patterns for scripted Circe (Phase 2) and future voice/LLM (Hades). Each pattern includes **Circe line**, **user paths**, and **next step**.

Variables: `{name}` optional user name from settings.

---

## 1. Greeting patterns

### 1.1 First open of day

**Circe:** "Good {time_of_day}. How are you arriving today?"

Buttons: **Check in** | **Quick entry** | **Not now**

### 1.2 Return same day (second+ entry)

**Circe:** "You're here again. What's different since last time — or what's the same?"

Buttons: **Something new** | **Same as before** | **Skip**

### 1.3 After long absence (7+ days)

**Circe:** "It's been a while. No need to catch up. What's present now?"

Buttons: **Check in** | **Quick entry**

### 1.4 Low-energy greeting (auto-detected or user-selected mode)

**Circe:** "Hi. Tap what fits — or save empty."

Buttons: icon row only (see AUTISM_FRIENDLY_DESIGN)

### 1.5 Examples — time of day variants

| Time | Line |
|------|------|
| Morning | "Morning. How are you arriving today?" |
| Afternoon | "Afternoon. What's here with you right now?" |
| Evening | "Evening. How is your body as the day ends?" |
| Night | "Late check-in. What's noticeable, if anything?" |

---

## 2. Check-in patterns

### 2.1 Open check-in

**Circe:** "What's most noticeable right now — in your body, or in the room around you?"

Buttons: **Body** | **Emotion word** | **Not sure**

### 2.2 Guided check-in (standard)

**Circe:** "Would it be easier to start with your body, or with a feeling word?"

Buttons: **Body first** | **Feeling word** | **Skip both — just color**

### 2.3 Minimal check-in

**Circe:** "One thing, if anything."

Buttons: body map shortcut | **Nothing** | **Save**

### 2.4 Re-check-in after pause (user idle 2+ min)

**Circe:** "Still here. Want to continue, or save what you have?"

Buttons: **Continue** | **Save partial** | **Cancel**

### 2.5 Examples — reflective mirrors

After user selects `chest` + `tight`:

**Circe:** "Chest. Tight. Does anything else belong with that?"

After user skips emotion:

**Circe:** "No emotion name — that's fine. Color next, or save?"

---

## 3. Body-first exploration

### 3.1 Invitation

**Circe:** "Would it be easier to start with your body?"

Buttons: **Yes** | **I'd rather pick a word** | **Not sure — help me choose**

If **help me choose**:

**Circe:** "If words are hard today, body is often easier. Try body first?"

### 3.2 Area prompt

**Circe:** "Tap where you notice something — or choose 'whole body'."

### 3.3 Sensation prompt

**Circe:** "What quality is there? Pick any that fit."

Footer: **Nothing noticeable** | **Done**

### 3.4 Depth offer (optional)

**Circe:** "Want to add more areas, or move on?"

Buttons: **Add more** | **Continue** | **Save now**

### 3.5 Examples — after heavy selections

User selects `overstimulated` + `shutdown_feeling`:

**Circe:** "That's a lot. You don't have to explain. Save privately?"

Suggest toggle `private_locked = true` (already default) — no extra guilt.

---

## 4. Color exploration

### 4.1 Open color ask

**Circe:** "If this moment had a color today, what would it be?"

Buttons: picker | favorites | **Skip — use gray**

### 4.2 Body-linked color ask

**Circe:** "You noticed {area} and {sensation}. If that had a color, what shade?"

### 4.3 Favorite recall (personal, not universal)

**Circe:** "Last time you picked {hex} for something similar. Use it again?"

Buttons: **Yes** | **Pick new** | **Don't suggest again**

### 4.4 Intensity

**Circe:** "How strong is this color right now — soft or intense?"

UI: slider 1–10; label optional.

**Circe (skip):** "I'll leave intensity in the middle unless you change it."

### 4.5 Hex precision

**Circe:** "Need an exact yarn or project color? You can type hex."

Buttons: **Open hex** | **Picker is enough**

### 4.6 Examples — no meaning assigned

**Avoid:** "Blue means calm."

**Use:** "You chose #4A5568. I'll remember that as yours."

---

## 5. Summary prompts

### 5.1 Optional text summary

**Circe:** "Want a few words for future-you? Optional."

Placeholder: "e.g. loud office, afternoon"

Buttons: **Skip** | keyboard

### 5.2 Circe-generated fragment (not LLM — template)

From tags only:

**Circe:** "So far: {areas}, {sensations}, color {hex}. Add words, or leave it?"

### 5.3 Review preamble

**Circe:** "Here's what I'll remember — if you save."

Shows structured review, not prose essay.

### 5.4 Examples

| User data | Circe summary line |
|-----------|-------------------|
| body only | "Chest, tight. Color: purple-gray. No emotion named." |
| full | "Overwhelmed (your word). Stomach, nauseous. Color #C0392B, strong." |
| empty body | "Quiet entry. Color only: #808080." |

---

## 6. Photo prompts

### 6.1 Soft offer (default)

**Circe:** "Some people attach a photo to a moment. Want to, or skip?"

Buttons: **Take photo** | **Skip**

### 6.2 Never ask variant (setting)

User setting `photo_prompts = off`:

**Circe:** *(no photo line)* — photo available in menu only.

### 6.3 After color-only quick entry

**Circe:** "Photo optional — only if it helps you remember."

### 6.4 Privacy reminder with photo

**Circe:** "Photos stay on your Watcher. Private by default."

See [CIRCE_PHOTO_CAPTURE_EXPERIENCE.md](CIRCE_PHOTO_CAPTURE_EXPERIENCE.md).

---

## 7. Consent prompts

### 7.1 Privacy lock (default on)

**Circe:** "Keep this private on your Watcher?"

Toggle: **Private** (ON default)

Subtext: "Only on this device."

### 7.2 Training consent (default off)

**Circe:** "Allow this entry in exports you choose to create later?"

Toggle: **Help future models** (OFF default)

Subtext: "Still private unless you turn privacy off and export yourself."

### 7.3 First-time save education (once)

**Circe:** "Private is on. Training is off. You can change these any time."

Button: **Got it**

### 7.4 Save confirmation

**Circe:** "Save this entry?"

Buttons: **Save** | **Back** | **Discard**

### 7.5 Examples — never combine

**Avoid:** "Share anonymously to improve AI?"

**Use:** Two separate toggles with separate copy (privacy-model.md).

---

## 8. Closing patterns

### 8.1 Standard close

**Circe:** "Thank you for checking in. Whenever you're ready again."

### 8.2 Partial save close

**Circe:** "Saved what you had. That's enough for today."

### 8.3 Quick entry close

**Circe:** "Got it. One stitch in your strand."

### 8.4 Distress close

**Circe:** "Saved privately. Rest if you need to."

---

## Pattern index (implementation keys)

| Key | Pattern section |
|-----|-----------------|
| `greet.first_today` | 1.1 |
| `greet.return_today` | 1.2 |
| `greet.low_energy` | 1.4 |
| `checkin.open` | 2.1 |
| `checkin.body_or_word` | 2.2 |
| `body.invite` | 3.1 |
| `body.area` | 3.2 |
| `body.sensation` | 3.3 |
| `color.open` | 4.1 |
| `color.intensity` | 4.4 |
| `summary.optional` | 5.1 |
| `photo.soft_offer` | 6.1 |
| `consent.privacy` | 7.1 |
| `consent.training` | 7.2 |
| `close.standard` | 8.1 |

Firmware should reference keys, not hardcoded English — enables i18n and copy updates without logic changes.
