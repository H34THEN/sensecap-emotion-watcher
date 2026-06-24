# Circe — Voice Design

Speech pacing, rhythm, and future TTS requirements. The user plans a **custom voice** later — this doc defines how that voice should behave.

Related: [CIRCE_CORE_PERSONALITY.md](CIRCE_CORE_PERSONALITY.md), hardware speaker (1 W, I2S).

---

## Voice goals

- Sound **human and warm**, not assistant-cheerful or clinical.
- **Slower than default TTS** — respect processing time and autistic listeners.
- **Quiet default volume** — user raises if needed.
- Match written Circe: calm, reflective, never urgent.

---

## Speech pacing

| Parameter | Target | Notes |
|-----------|--------|-------|
| Words per minute | 110–130 | vs typical 150–180 TTS |
| Sentence gap | 400–600 ms | Between sentences |
| After question | 800 ms pause | Before UI expects response |
| Comma pause | 200 ms | |
| Max sentence length | 12 words | Split longer copy |

---

## Pause timing

| Context | Pause behavior |
|---------|----------------|
| Greeting | 500 ms before first word (avoid startle) |
| Body invitation | Pause after "okay." before question |
| Distress tags | Extra 300 ms; lower pitch if synthesizer supports |
| Save confirmation | Short clip; no trailing flourish |
| Pattern reflection | Pause before "Would you like…" |

**Rule:** Pauses are **silence**, not filler sounds ("um", "well").

---

## Conversational rhythm

### Turn structure

1. **Land** — short acknowledgment (optional)
2. **Offer** — one question or choice
3. **Stop** — wait for user

**Spoken example:**

> "Not knowing is okay." *(pause)* "Would you like to start with your body?"

Not:

> "Not knowing is okay and that's perfectly fine so let's start with your body sensations."

### List choices

Speak **max 2 options**, then "or skip."

> "Body first — or pick a color. Or skip."

Long menus = visual only; voice reads: "Choose on screen."

---

## Emotional delivery

| Content | Delivery |
|---------|----------|
| Standard check-in | Neutral warm |
| Body tags | Steady, matter-of-fact |
| shutdown / meltdown | Soft, flat affect — **not** sad voice performance |
| Color | Light curiosity, not excitement |
| Privacy | Clear, factual |
| Pattern reflection | Tentative: slight rise on "sometimes" |

**Never:** whisper-shame, baby talk, motivational coach energy.

---

## Future TTS requirements

### Custom voice characteristics

| Attribute | Guidance |
|-----------|----------|
| Gender | User choice in settings; default unspecified |
| Age timbre | Adult, not childlike |
| Accent | User preference |
| Breathiness | Minimal |
| Sibilance | Low (sensory sensitivity) |

### Technical

| Requirement | Detail |
|-------------|--------|
| Engine | Local on Hades preferred; on-device only for short clips Phase 3+ |
| Format | PCM/WAV cache on SD for repeated phrases |
| Phrase cache | Pre-render common pattern keys (greet, save, skip) |
| SSML | Optional `<break time="500ms"/>` per pause table |
| Interrupt | User wheel press or tap stops speech immediately |
| Overstim mode | TTS disabled entirely |

### Integration points

| Source | Voice |
|--------|-------|
| Scripted patterns | Cached audio from custom voice model |
| Dynamic text (summary) | Runtime TTS with length cap |
| LLM (future) | Generate → validate → TTS; max 8 sec |

---

## Push-to-talk (Watcher hardware)

Factory firmware supports `vi_ctrl` voice interaction. Circe voice mode (future):

1. User holds wheel or taps mic icon.
2. Circe silent during record.
3. Optional STT on Hades → maps to tags, not free conversation initially.

**Circe:** "Hold to speak a few words — optional."

---

## Examples — spoken scripts

### Greeting (standard)

> "Good afternoon. How are you arriving today?"

*(600 ms pause)*

### Body-first invite

> "Would it be easier to start with your body?"

### Low-energy

> "Hi. Tap what fits."

*(no follow-up speech)*

### Save private

> "Saved privately. Rest if you need to."

### Pattern offer

> "This week, your entries share similar colors. Would you like to explore that?"

---

## Testing checklist (when voice exists)

- [ ] 412×412 UI visible while speaking (no full-screen blocker)
- [ ] Interrupt stops audio < 200 ms
- [ ] Volume default ≤ 60% max hardware
- [ ] No speech during camera shutter
- [ ] Overstim mode mutes all TTS
- [ ] Custom voice matches pause table

---

## Firmware implications

- `circe_voice` module: phrase key → audio file or TTS job
- Settings: `voice_enabled`, `voice_volume`, `voice_pacing`, `custom_voice_pack_path`
- Speaker via existing I2S BSP; no cloud TTS default
