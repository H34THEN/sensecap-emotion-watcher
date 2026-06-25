# CIRCE Future Functionality Roadmap Reference

**Project:** CIRCE — SenseCAP Watcher emotional regulation companion  
**Purpose of this document:** A Cursor reference roadmap for future CIRCE functionality phases.  
**Important:** This is not a Cursor prompt. It is a planning and implementation reference that future handoff prompts can cite.

---

## 0. Current Stable Direction

CIRCE has moved beyond the initial firmware proof-of-concept and is becoming a standalone emotional regulation companion. The current foundation should be preserved while future phases build more intelligence, memory, and supportive regulation features.

### Stable Foundation To Preserve

Future phases should avoid regressing these systems:

- App-only flashing to the ESP32-S3 OTA app slot.
- Factory partitions preserved.
- SD storage under `/sdcard/CIRCE`.
- Entry files using FAT-safe `.JSN` filenames.
- Worker-backed storage operations.
- Save / review / delete functionality.
- Manual time setting and date-based entry folders.
- Theme persistence through NVS.
- Terminal-style UI direction.
- Home slot-wheel selector.
- Body-first check-in flow.
- Emotional tone and mood color as separate steps.
- Grounding / breathing MVP.
- Diagnostics moved off LVGL where possible.
- Privacy defaults:
  - `training_ok = false`
  - `private_locked = true`
  - no automatic cloud upload
  - no automatic external sync

### Overarching Product Goal

CIRCE should feel like:

- a living emotional regulation companion
- a futuristic therapeutic terminal
- a portable emotional observatory
- a private body-first reflection system

CIRCE should not feel like:

- a generic smartwatch
- a wellness tracker
- a fitness app
- a clinical assessment form
- a cloud assistant
- a settings-heavy embedded demo

### Interaction Principles

CIRCE is designed for a circular display and physical rotary input.

Preferred interaction model:

- Rotary knob selects.
- Press confirms.
- Double press backs out.
- Long press opens system tools or exits a session.
- Touch is used where it adds real value, such as the mood color field.
- Text should be large, centered, and readable.
- Slot-wheel style selection is preferred for crowded top-level menus.
- Do not add scrollbars or tall lists unless there is no better option.
- Avoid constant rebuilding of LVGL objects during knob movement.
- Avoid storage or SD work on the LVGL task.
- Avoid full-screen colorful panels unless they are part of a deliberate color-selection tool.

---

## 1. Reflection Engine MVP

**Status (2026-06-24):** Implemented — current-entry rule engine, post-save `CIRCE_FLOW_REFLECTION` screen. **Recent pattern observations** added (worker-loaded last 7 days / 16 entries). See `docs/reflection/RECENT_PATTERN_REFLECTION_MVP.md`. History-only patterns deferred before this update; now live for post-save.

### Priority

Very high.

### Purpose

Make CIRCE respond intelligently after check-ins without requiring an AI model, cloud service, or GPU. This is the first phase where CIRCE begins to feel like it remembers and notices the user.

### Core Idea

After a user saves a check-in, CIRCE should offer a small reflection based on the current entry and recent local history.

This should not diagnose. It should observe gently.

### What It Should Accomplish

- Turn saved entries into meaningful feedback.
- Make the journal feel alive.
- Give the user a reason to keep checking in.
- Begin the foundation for future AI-assisted reflection.
- Preserve privacy and standalone operation.

### Example Reflections

Use cautious, non-diagnostic language:

- `I noticed your shoulders carried most of this entry.`
- `This is the third time chest tension has appeared recently.`
- `Your color today is darker than your last saved color.`
- `You chose OVERWHELMED, but your color is very soft. That contrast may be worth noticing.`
- `No need to name it perfectly. I saved what you noticed.`
- `This entry is stored. We can come back to it later.`
- `Would a grounding sequence help right now?`

### Reflection Types

#### Immediate Reflection

Generated right after save.

Examples:

- body-focused
- tone-focused
- color-focused
- intensity-focused
- regulation suggestion

#### Recent Pattern Reflection

Uses the last few entries.

Examples:

- repeated body area
- repeated tone
- repeated color range
- rising intensity
- recurring time of day

#### Gentle Invitation

Optional next action.

Examples:

- `REGULATE`
- `REVIEW`
- `SAVE NOTE`
- `HOME`

### Suggested Data Inputs

- current entry body area
- sensation
- intensity
- emotional tone
- color hex
- time of day
- entry mode
- last 3–10 entries
- recent regulation sessions

### Suggested Output Fields

If reflections are stored:

```json
{
  "reflection_id": "8HEXID",
  "entry_id": "8HEXID",
  "reflection_type": "immediate",
  "text": "I noticed your shoulders carried most of this entry.",
  "confidence": "low",
  "generated_by": "rule_engine",
  "created_at": "2026-06-24T19:42:00"
}
```

### Guardrails

Do not say:

- `You are anxious.`
- `You are depressed.`
- `This means you have...`
- `You always...`
- `You should...`

Prefer:

- `I noticed...`
- `This may be worth noticing.`
- `It might help to...`
- `Would you like...`
- `This has appeared before.`
- `No need to decide what it means yet.`

### Success Criteria

- Reflection appears after save.
- Reflection uses current entry data.
- Reflection never blocks save.
- Reflection works offline.
- Reflection does not diagnose.
- Reflection can be skipped or dismissed.
- Save / review / delete still work.
- Reflection generation does not run on the LVGL task if it requires storage/history reads.
- Hardware remains stable after repeated saves.

---

## 2. Memory Timeline

**Status (2026-06-24):** Implemented — category menu, worker-backed load, 16-entry cap, encoder browse. See `docs/memory/MEMORY_TIMELINE_MVP.md`.

### Priority

Very high.

### Purpose

Transform Review from a simple latest-entry screen into CIRCE's local memory interface.

### Core Idea

CIRCE should let the user browse entries in a simple timeline:

- Today
- Yesterday
- This Week
- Older

The goal is not analytics yet. The goal is accessible memory.

### What It Should Accomplish

- Make saved entries easy to find.
- Support multiple entries per day.
- Make emotional journaling feel meaningful.
- Provide a foundation for future pattern recognition.
- Keep review lightweight enough for the Watcher.

### Suggested Navigation

Home slot-wheel:

- REVIEW

Review screen slot-wheel:

- TODAY
- YESTERDAY
- THIS WEEK
- ALL ENTRIES
- BACK

Within a day:

```text
18:42
CHEST / TIGHT / 9
OVERWHELMED
CUSTOM #8A4DFF
```

### Entry Summary Format

Keep compact and readable:

```text
18:42
CHEST / TIGHT / 9
TONE OVERWHELMED
COLOR #8A4DFF
```

For regulation entries:

```text
19:10
REGULATION BREATHING
72s / 3 rounds
COMPLETED
```

### Actions On Entry

- VIEW
- DELETE
- REFLECT
- FAVORITE
- BACK

Favorite can be planned but does not need to be fully implemented immediately.

### Technical Notes

- Use index where available.
- Fall back to folder scan only via worker.
- Do not scan large SD directories from LVGL.
- Keep folder walking asynchronous.
- Use `.JSN` entries first.
- Maintain legacy compatibility with `19700101`, `UNSET`, `.JSON`, `.DAT`, and bare names if already supported.

### Success Criteria

- User can open Review and see a list of entries.
- Multiple same-day entries appear.
- Entries show tone and color separately.
- Regulation entries display clearly.
- Empty state is explicit:
  - `no entries recorded yet`
  - `begin a check-in to create one`
- Delete still works.
- Review does not crash when index is dirty.
- Review does not block UI during folder scans.
- Hardware remains stable with at least 20 test entries.

---

## 3. Body Heat Map

### Priority

High.

### Purpose

Make body-first emotional awareness visible.

### Core Idea

CIRCE should summarize body sensations visually over time using a lightweight body map or text-based intensity map.

This is especially important because the project is designed for someone whose emotional experiences are strongly tied to physical sensations.

### MVP Version

Text heat map:

```text
CHEST       ███████
SHOULDERS   █████
JAW         ███
STOMACH     ██
HANDS       █
```

This is much safer than immediately implementing a body silhouette.

### Later Version

A small body silhouette with overlays:

- chest
- head
- jaw
- shoulders
- stomach
- hands
- legs
- back

### What It Should Accomplish

- Show where sensations cluster.
- Help the user recognize body patterns.
- Give CIRCE a more embodied memory.
- Support future pattern recognition.

### Possible Views

#### Today

Body areas from today.

#### This Week

Most repeated areas.

#### Current Entry

Shows the active entry's strongest body locations.

### Language Examples

- `Your chest has appeared often today.`
- `Shoulders are the strongest signal this week.`
- `Your body map is quiet right now.`
- `No strong pattern yet.`

### Technical Notes

- Compute summaries in worker.
- Avoid large visual overlays initially.
- Text bar chart is safest MVP.
- Store derived summaries as rebuildable cache if needed.
- Do not treat high intensity as medical data or diagnosis.

### Success Criteria

- Body map opens from Review or Patterns. **Implemented (MVP):** `REVIEW → BODY MAP` and `PATTERNS → BODY MAP`. See `docs/body/BODY_HEAT_MAP_MVP.md`.
- Summary is generated from saved entries.
- No SD scanning on LVGL task.
- Display is readable on circular screen.
- No diagnosis or medical claim.
- Entry deletion updates or invalidates cached summary. **MVP:** no cache; recomputed each open.
- Empty state works.

---

## 4. Pattern Recognition

### Priority

Very high.

### Purpose

Use local memory to help CIRCE notice recurring patterns.

### Core Idea

Pattern recognition should be simple, explainable, local, and non-diagnostic.

No AI model is required for the MVP.

### What It Should Accomplish

- Help the user see repeated body sensations.
- Help the user notice color shifts.
- Help the user notice time-of-day patterns.
- Help the user compare regulation sessions with check-ins.
- Provide insight without claiming certainty.

### Pattern Categories

#### Body Patterns

- repeated body area
- repeated sensation
- repeated intensity over threshold

#### Tone Patterns

- repeated emotional tone
- tone shifts across day
- unknown/numb frequency

#### Color Patterns

- darker/lighter than recent entries
- higher saturation
- repeated hue families

#### Time Patterns

- morning vs evening
- late-night check-ins
- repeated entries on certain days

#### Regulation Patterns

- check-in before regulation
- check-in after regulation
- intensity before/after if user logs again

### Example Reflections

- `Chest tension appears often in evening entries.`
- `Your colors have been cooler this week.`
- `NUMB has appeared more than once recently.`
- `Grounding sessions often happen after high intensity entries.`
- `I do not know what this means yet, but I can remember it with you.`

### Data Model Suggestion

```json
{
  "pattern_id": "8HEXID",
  "pattern_type": "body_area_recurrence",
  "period": "7d",
  "summary": "Chest appeared in 4 of 7 recent entries.",
  "evidence_count": 4,
  "confidence": "low",
  "created_at": "2026-06-24T20:00:00"
}
```

### Guardrails

- Use `appears`, not `means`.
- Use `may`, not `is`.
- Do not diagnose.
- Do not infer cause.
- Do not make health claims.
- Do not push action.
- Always allow dismissal.

### Success Criteria

- Pattern scan works offline.
- Pattern scan runs in worker.
- Results are visible in Review or a new Patterns view.
- User can dismiss or ignore.
- Pattern text is gentle.
- No pattern is shown with fewer than a safe minimum number of entries.
- No crash with empty or small datasets.
- No SD scan on LVGL task.

---

## 5. Conversation Engine

### Priority

Very high.

### Purpose

Make CIRCE's text feel like it comes from one consistent companion personality instead of scattered labels.

### Core Idea

All user-facing text should come from pattern keys and a small response engine. This supports future voice, accessibility, localization, and emotional consistency.

### What It Should Accomplish

- Replace robotic UI phrases.
- Keep CIRCE's tone consistent.
- Prepare all screen text for future TTS.
- Make flows feel relational.
- Reduce hardcoded string drift.

### Tone

CIRCE should sound:

- calm
- grounded
- direct
- warm
- non-clinical
- not overly cheerful
- not corporate
- not diagnostic

### Phrase Examples

#### Home

- `ready when you are`
- `start with the body`
- `I am here`
- `no need to name it yet`

#### Body Flow

- `Where is your body speaking?`
- `What does that area feel like?`
- `How strong is the signal?`
- `Not knowing is allowed.`

#### Tone Flow

- `Choose a word, or skip.`
- `This can stay unknown.`
- `A rough word is enough.`

#### Color Flow

- `Choose the color of this moment.`
- `Let the color be approximate.`
- `You can skip this.`

#### Save Confirmation

- `Saved.`
- `I will remember this.`
- `This entry is private.`
- `We can return to it later.`

#### Regulation

- `Follow the pulse if useful.`
- `You can stop anytime.`
- `No need to force calm.`
- `Stay with what is here.`

### Suggested Architecture

- `circe_copy.c/h`
- pattern key system
- mode-aware copy
- future voice-friendly text
- no scattered UI strings

### Success Criteria

- Major screens use copy keys.
- No hardcoded emotional copy in UI implementation files.
- Text remains short enough for circular display.
- Copy is reusable for future TTS.
- User can distinguish CIRCE personality from generic firmware.
- No medical or diagnostic language.

---

## 6. Regulation Library

### Priority

High.

### Purpose

Expand CIRCE from journaling into practical emotional regulation support.

### Core Idea

The existing breathing and body anchor MVP should become a small library of offline regulation tools.

### Tool Candidates

#### 1. Breathing

Already started.

Future variations:

- 4-2-6
- box breathing
- longer exhale
- gentle pacing

#### 2. Body Anchor

Already started.

Future improvements:

- more prompts
- custom favorite anchors
- sensory-friendly prompts

#### 3. 5-4-3-2-1 Grounding

Prompt sequence:

- name 5 things you see
- 4 things you feel
- 3 things you hear
- 2 things you smell
- 1 thing you can taste or imagine tasting

For Watcher MVP, avoid requiring text entry unless voice or input is ready. It can be guided silently.

#### 4. Bilateral Tapping

Visual left/right cue.

- left pulse
- right pulse
- slow rhythm
- optional haptic/audio later

#### 5. Progressive Muscle Release

Gentle prompt sequence:

- hands
- shoulders
- jaw
- stomach
- legs

Avoid medical claims.

#### 6. Autism Overload Routine

Low-stimulation mode:

- fewer words
- dimmer visuals
- no forced emotion naming
- body-first choices
- sensory reset prompts

Example phrases:

- `less input now`
- `choose one anchor`
- `no words needed`
- `stay with one sensation`

### Save Behavior

Regulation session entries should store:

- `entry_mode = regulation`
- `regulation_type`
- `duration_seconds`
- `rounds_completed`
- `session_completed`
- optional before/after intensity later

### Success Criteria

- Regulation menu remains simple.
- Each regulation tool can be started quickly.
- No tool requires cloud.
- No tool uses diagnostic language.
- Regulation save works.
- Review distinguishes regulation entries from check-ins.
- Existing breathing and body anchor do not regress.

---

## 7. Emotional Color Intelligence

### Priority

High.

### Purpose

Make color data meaningful without requiring AI.

### Core Idea

Every selected color can be converted into simple properties:

- hue
- saturation
- value/brightness
- warmth
- darkness
- contrast from previous entry
- color family

CIRCE can use these to observe changes over time.

### What It Should Accomplish

- Make color logging more valuable.
- Support future mood strands.
- Support pattern recognition.
- Help CIRCE notice shifts without interpreting too much.

### Derived Fields

From `color_hex`, compute:

```json
{
  "color_hue": 210,
  "color_saturation": 0.67,
  "color_value": 0.92,
  "color_family": "blue",
  "color_temperature": "cool",
  "color_brightness_label": "bright"
}
```

### Example Observations

- `This color is cooler than your last entry.`
- `Today's colors are mostly muted.`
- `This is one of the brightest colors this week.`
- `Your color shifted from red to blue after grounding.`

### Guardrails

Do not say:

- `Blue means sadness.`
- `Red means anger.`
- `This color proves...`

Prefer:

- `You chose...`
- `This color is...`
- `Compared with recent entries...`
- `Only you decide what it means.`

### Touch Picker Integration

The future improved color picker should store:

- `color_source = touch_picker`
- `color_hex`
- derived HSV fields
- optional nearest preset

### Success Criteria

- HSV derived correctly.
- Review can show color family if useful.
- Pattern engine can use color properties.
- No universal color meanings are asserted.
- Old entries with only hex still work.
- Computation is lightweight and does not touch LVGL.

---

## 8. Camera Memories

### Priority

Medium-high.

### Purpose

Begin using the Watcher camera as a memory sensor, not yet as emotion recognition.

### Core Idea

Photo capture can be linked to an emotional entry when the user consents.

This is not facial recognition yet.

This is not automatic emotion detection yet.

It is user-controlled memory capture.

### What It Should Accomplish

- Allow user to connect a moment with an image.
- Build a dataset for future emotion model training if consented.
- Preserve privacy.
- Prepare for later camera/AI work.

### Flow

After save:

```text
ADD PHOTO?
SAVE IMAGE
SKIP
```

Or under entry details:

```text
ADD MEMORY PHOTO
```

### Consent Rules

Before first photo:

- explain local storage
- explain that photo is optional
- explain training consent separately

Possible copy:

- `A photo can help future CIRCE learn your patterns.`
- `This is optional.`
- `Saved locally unless you export it.`
- `Use this for training? yes / no`

### Storage

Suggested:

```text
/sdcard/CIRCE/PHOTOS/YYYYMMDD/8HEXID.JPG
/sdcard/CIRCE/PHOTOS/YYYYMMDD/8HEXID.META
```

Use FAT-safe names.

### Entry Fields

```json
{
  "photo_id": "8HEXID",
  "photo_path": "/sdcard/CIRCE/PHOTOS/20260624/8HEXID.JPG",
  "photo_consent": true,
  "training_ok": false
}
```

### Guardrails

- Photo step must be optional.
- Never force photo during distress.
- Hide or soften photo prompt after high overwhelm/shutdown tags.
- Delete entry should offer photo delete.
- Training consent must be separate from photo capture.
- No automatic upload.

### Success Criteria

- User can skip photo.
- User can capture photo after entry.
- Photo links to entry.
- Delete removes or offers to remove photo.
- Training consent is stored separately.
- No camera work blocks journaling.
- No cloud upload.
- No emotion recognition claim.

---

## 9. Voice Personality

### Priority

Medium-high.

### Purpose

Give CIRCE a spoken presence without implementing full conversational AI.

### Core Idea

Start with local phrase playback or TTS phrase library. Voice should support regulation and confirmations, not become the primary input yet.

### What It Should Accomplish

- Make breathing and grounding more accessible.
- Make CIRCE feel alive.
- Prepare for future voice conversation.
- Avoid cloud dependency.

### MVP Voice Features

- session start tone
- soft confirmation tone
- breathing cadence spoken or tonal
- save confirmation
- regulation prompts

### Example Voice Lines

- `Begin when ready.`
- `Inhale.`
- `Hold.`
- `Exhale.`
- `Saved.`
- `I will remember this.`
- `You can stop anytime.`

### Audio Tone

Should be:

- soft
- low-pressure
- minimal
- not phone-like
- not alarm-like
- not arcade-like

### Architecture

Future pipeline:

```text
pattern key
→ screen text
→ optional spoken phrase
```

Do not create separate voice-only copy that drifts from screen copy.

### Hardware Notes

The Watcher has speaker/I2S paths in factory firmware. Future implementation should reuse known BSP audio paths where possible.

### Guardrails

- Voice off by default.
- Mute always available.
- No cloud TTS by default.
- No always-listening mode at this phase.
- No recorded audio unless explicitly added later with consent.

### Success Criteria

- Voice phrases can be enabled/disabled.
- Regulation can run silently.
- Voice does not block UI.
- No crashes from audio playback.
- All spoken phrases are also represented by copy keys.
- User can mute quickly.

---

## 10. Daily Companion

### Priority

Very high, but should come after memory and reflection basics.

### Purpose

Make CIRCE feel present across the day.

### Core Idea

CIRCE can use time and local entries to offer gentle daily continuity.

### What It Should Accomplish

- Encourage consistent check-ins.
- Make CIRCE feel like a companion.
- Help user close the day.
- Summarize without diagnosing.

### Daily Moments

#### Morning

- `Good morning.`
- `Would you like to start with your body?`
- `No need to know the mood yet.`

#### Midday

- `You have one entry today.`
- `Would a quick check-in help?`

#### Evening

- `Before today ends, would you like to leave one note?`
- `I can remember the day quietly.`

#### After Multiple Entries

- `You checked in three times today.`
- `Your body mentioned chest twice.`
- `Would you like a short reflection?`

### Daily Summary

Example:

```text
TODAY
3 entries
body: chest, shoulders
tones: unknown, tired
colors: cool / muted
regulation: breathing
```

### Guardrails

- No nagging.
- No streak pressure.
- No gamification.
- No badges.
- No shame for missing days.
- Missing days should be neutral.

Possible copy:

- `No entry yesterday. That is allowed.`
- `The record can have quiet days.`

### Success Criteria

- Daily summary can be generated locally.
- Missing days handled gently.
- No cloud needed.
- No pushy reminders.
- User can ignore.
- Review and Reflection Engine can reuse summary.

---

## Suggested Roadmap Order

Recommended order from current firmware state:

1. Reflection Engine MVP
2. Memory Timeline
3. Conversation Engine cleanup — **done** (see `docs/conversation/CONVERSATION_ENGINE_COPY_POLISH.md`)
4. Pattern Recognition — **done** (see `docs/patterns/PATTERN_RECOGNITION_MVP.md`)
5. Regulation Library expansion — **done** (see `docs/regulation/REGULATION_LIBRARY_EXPANSION_MVP.md`)
6. Emotional Color Intelligence — **done** (see `docs/color/EMOTIONAL_COLOR_INTELLIGENCE_MVP.md`)
7. Improved Color Picker gradient / field polish — **done** (see `docs/color/COLOR_PICKER_FIELD_POLISH_MVP.md`)
8. Camera Memories — **scaffold** (see `docs/camera/CAMERA_MEMORIES_MVP.md`)
9. Voice Personality — **done** (see `docs/voice/VOICE_PERSONALITY_MVP.md`)
10. Daily Companion — **done** (see `docs/daily/DAILY_COMPANION_MVP.md`)

### Why This Order

The fastest way to make CIRCE feel alive is not more sensors. It is memory plus reflection.

The strongest short-term path is:

```text
entries
→ memory
→ reflection
→ patterns
→ regulation suggestions
```

Only after this should CIRCE move deeply into camera, voice, cloud, or ML.

---

## Implementation Guardrails For All Future Phases

### Stability

- Build before flashing.
- App-flash only.
- Preserve partitions.
- Do not touch `nvsfactory`.
- Avoid SD work inside LVGL callbacks.
- Use worker tasks for file/index/history operations.
- Avoid huge LVGL object grids.
- Avoid large stack buffers.
- Log exact errors with `esp_err_to_name()` or `errno`.

### UI

- Prefer terminal selection.
- Prefer slot-wheel selection for crowded menus.
- Keep text readable.
- Avoid tiny captions.
- Avoid filled card UI.
- Avoid scrollbars on circular display.
- Do not rebuild full screens on every knob tick.
- Create objects once when possible.
- Update text/styles rather than recreating objects.

### Privacy

- Local-first.
- No automatic upload.
- Consent before photo/audio/training.
- Training defaults false.
- Private defaults true.
- Delete should delete associated media when applicable.
- Export should be user-initiated.

### Language

CIRCE observes. CIRCE does not diagnose.

Use:

- `I noticed...`
- `This appeared...`
- `Would you like...`
- `No need to decide yet.`
- `I can remember this.`

Avoid:

- `You are...`
- `This means...`
- `You should...`
- `Diagnosis`
- `Treatment`
- `Disorder`
- `Compliance`

---

## Future Reference Notes

### Mood Color Picker v2 Direction

The current touch picker is functional but visually abstract. The desired future version is:

- a large circular color field
- touch-drag selection
- magnifier orb
- live hex value
- preset fallback
- no heavy LVGL object grid
- no full-screen color wash
- memory-safe gradient implementation

The color picker must remain separate from emotional tone selection.

### Strand Visualization

Strand remains disabled due to LVGL stack and SD access issues. Future strand work should:

- run data loading in worker
- avoid SD/index reads in UI build
- use cached colors
- render lightweight arcs
- avoid large object counts
- treat missing days neutrally

### Camera / Voice / ML

These should remain deferred until:

- memory timeline exists
- reflection engine exists
- consent model is clear
- export/training data model is stable
- local journaling remains reliable

---

## Standalone MVP RC1 (2026-06)

Current standalone feature set is documented in **`docs/releases/CIRCE_STANDALONE_MVP_RC1.md`**.

RC1 stabilization fixed the REVIEW → TODAY timeline browse UI (browser init after `clear_content()`). UI editing reference: **`docs/ui/UI_FILE_MAP.md`**.

Post-RC1 priorities: camera capture integration, body map silhouette (optional), conversation deepening, Magic Mirror (deferred).

---

## Definition Of A Good CIRCE Phase

A good future phase should:

1. Add one meaningful user-facing capability.
2. Preserve current stable storage and UI.
3. Use worker tasks for storage/history work.
4. Include clear success criteria.
5. Include on-device validation.
6. Avoid unnecessary cloud/AI complexity.
7. Make CIRCE feel more like a companion.
8. Avoid making the circular UI feel crowded.
9. Respect emotional regulation needs.
10. Leave the firmware more stable than before.
