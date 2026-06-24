# Phase 1.5 Handoff Report — Circe Conversation & Experience Design

Generated: 2026-06-24

---

## 1. Documents created

```
docs/conversation/
├── README.md
├── CIRCE_CORE_PERSONALITY.md
├── CIRCE_CONVERSATION_PATTERNS.md
├── CIRCE_BODY_FIRST_FLOW.md
├── CIRCE_AUTISM_FRIENDLY_DESIGN.md
├── CIRCE_COLOR_INTERPRETATION.md
├── CIRCE_PHOTO_CAPTURE_EXPERIENCE.md
├── CIRCE_PATTERN_REFLECTION_ENGINE.md
├── CIRCE_VOICE_DESIGN.md
└── CIRCE_ENTRY_TYPES.md
```

**10 files** — complete conversational architecture for Circe before firmware implementation.

---

## 2. Recommended default entry mode

**Body-Only Entry** (with optional color step) — not Full Reflection.

**Rationale:**

- Matches primary user requirement: body sensations easier than emotions.
- Completes in 30–60 seconds with high-quality somatic data.
- Avoids decision fatigue of full flow on daily use.
- Full Reflection remains available explicitly from home.

Default home focus: **Body** → body map → sensations → color offer → save.

---

## 3. Recommended fast-entry mode

**Quick Entry** — 2–4 taps, ≤ 30 seconds.

**Flow:** Home → Quick → one favorite color OR one quick body tag pair → silent private save.

**Circe copy:** "One tap saves." / "Got it."

**Rationale:** Supports burnout, between-task logging, and mood-strand continuity without narrative pressure.

---

## 4. Recommended body-first mode

**Body-Only Entry** triggered by:

- Home **Body** button (default focus), OR
- Phrase chips: "I don't know what I feel", "I feel weird", "I feel off", "My body feels wrong", "I feel nothing"

**Flow:** See CIRCE_BODY_FIRST_FLOW.md — emotion step skippable in one tap; save shortcut after sensations.

**Circe anchor line:** "Not knowing is okay. Would you like to start with your body?"

---

## 5. Risks discovered

| Risk | Impact | Mitigation |
|------|--------|------------|
| Full flow as default | Abandonment, overload | Default Body-Only, not Full |
| Photo prompt during distress | Harmful UX | Never prompt on shutdown/overstim tags |
| Color "meanings" creep in | Shame, mislabeling | COLOR_INTERPRETATION forbidden list + validator |
| Pattern reflections feel diagnostic | User distrust | Template rules + "in your logs" hedging |
| Copy hardcoded in firmware | Circe drift from design | Pattern keys + string tables from docs |
| Too many home mode buttons | Decision paralysis | Default + "More modes" submenu |
| Voice before STT ready | Broken promise | Gray out Voice Entry until Phase 4+ |
| LLM overrides personality | Harmful outputs | Templates first; validator on generated text |
| Crisis tags without policy | Liability / harm | Phase 2 decision: resources link vs local-only (documented, not resolved) |

---

## 6. Firmware implications

1. **Conversation state machine** separate from LVGL pages — driven by pattern keys (`greet.first_today`, etc.).
2. **Copy resolver:** `(pattern_key, interaction_mode) → string` with short-answer variants.
3. **Conditional step graph:** skip photo, ratings, emotion based on `entry_mode` + tags + settings.
4. **New entry fields:** `entry_mode`, `interaction_mode` flags (schema update).
5. **Home screen:** Body default, Quick secondary, Full in "More".
6. **Phrase chips** on greeting for five ambiguous-feeling flows.
7. **No LLM in Phase 2** — scripted templates only from CONVERSATION_PATTERNS.
8. **TTS hook** reserved — phrase cache paths per VOICE_DESIGN.
9. **Reflection engine Phase 3** — template keys only, dismiss memory 14 days.
10. **Settings in NVS:** default entry mode, photo_prompts, interaction mode, craft metaphor toggle.

---

## 7. Module list changes recommended

| Change | Type | Notes |
|--------|------|-------|
| **circe_conversation_engine** | **Add** | State machine + pattern keys; sits above circe_assistant |
| **circe_copy** | **Add** | String table / i18n loader from conversation docs |
| **circe_entry_modes** | **Add** | Mode selection + step graph variants |
| **circe_assistant** | **Refine** | Becomes thin layer calling conversation_engine |
| **circe_voice** | **Add (Phase 3+)** | TTS phrase cache per VOICE_DESIGN |
| **pattern_reflection_engine** | **Refine** | Add conversation templates from PATTERN_REFLECTION doc |
| **calibration_mode** | **Refine** | Add default entry mode, photo_prompts, interaction modes |
| **emotion_tracker** | **Refine** | Add phrase-chip quick emotions (weird, off, nothing) |

No modules removed. Core storage and UI modules unchanged.

Update [module-map.md](../architecture/module-map.md) in Phase 2 when implementing.

---

## 8. Recommended firmware Phase 2 implementation prompt

```markdown
# PHASE 2 CURSOR HANDOFF — CIRCE Firmware MVP (Conversation-Driven)

Read first (mandatory):
- docs/conversation/CIRCE_CORE_PERSONALITY.md
- docs/conversation/CIRCE_CONVERSATION_PATTERNS.md
- docs/conversation/CIRCE_BODY_FIRST_FLOW.md
- docs/conversation/CIRCE_AUTISM_FRIENDLY_DESIGN.md
- docs/conversation/CIRCE_ENTRY_TYPES.md
- docs/roadmap/phase-2-implementation.md
- docs/sdk/sdk-research-notes.md
- schemas/emotion-entry.schema.json

Implement Circe — not generic forms.

Goal: Vertical slice on hardware —
  Home (Body default) → phrase/body-first flow → body_sensation_tags →
  optional color_picker → privacy toggles (defaults) → local_storage →
  entry_review with Circe copy from pattern keys.

Architecture:
1. circe_conversation_engine — state machine + pattern key resolver
2. circe_copy — string tables from CONVERSATION_PATTERNS (English v1)
3. circe_entry_modes — body_only + quick modes only in MVP
4. Script UI shows Circe lines from engine, not hardcoded in LVGL

Constraints:
- ESP-IDF v5.2.1, factory_firmware base
- Scripted Circe only — no LLM
- Defaults: training_ok=false, private_locked=true
- Body-first: emotion skippable one tap; save after body step
- Quick entry: ≤ 4 taps
- No photo prompt in body_only/quick MVP
- Short-answer copy variant when low_energy flag set

Schema additions:
- entry_mode, interaction_mode (document in entry-schema.md)

Deliverables:
1. firmware/circe builds and flashes
2. Conversation engine with ≥ 10 pattern keys wired
3. microSD JSON save with entry_mode
4. docs updated: module-map, entry-schema, decisions log

Do not implement: voice, patterns, Magic Mirror, ML, photo (Phase 2b).
```

---

**Phase 1.5 complete. Do not begin firmware in this session.**
