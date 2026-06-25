# Conversation Engine Copy Polish

**Phase:** Conversation Engine Copy Polish + Regulation Review Cleanup  
**Firmware:** `firmware/circe`  
**Reference:** `docs/CIRCE_future_functionality_roadmap_reference.md` — Sections 1, 5, 6, Implementation Guardrails

---

## Purpose

Centralize companion-facing text in the copy engine and align on-screen language with CIRCE's calm, non-clinical tone. This phase is copy and review-behavior only — no storage, worker, or timeline logic changes.

---

## Copy engine

| File | Role |
|------|------|
| `circe_copy.h` | Pattern key enum |
| `circe_copy.c` | Key → phrase table |
| `circe_conversation_engine.c` | Step → prompt key routing |

Access: `circe_copy_get(CIRCE_PATTERN_*)`

---

## Copy groups added or updated

| Key group | Example phrase |
|-----------|----------------|
| `home.feed_ready` | Ready when you are. |
| `home.wheel_hint` | Rotate select · press enter |
| `tone.prompt` | Choose a word, or skip. |
| `tone.rough_ok` | A rough word is enough. |
| `color.field_hint` | Drag to tune the color. Hex updates as you move. |
| `color.presets_prompt` | Preset colors are available. |
| `memory.menu_prompt` | Browse saved entries. |
| `memory.browse_hint` | Rotate entry · press view |
| `memory.index_repair_*` | Memory index needs repair. / Run diagnostics when ready. |
| `reg.grounding_*` | Grounding sequence / notice / breathe |
| `reg.breath_*` | Follow the pulse if useful / You can stop anytime |
| `reg.anchor_*` | Find one contact point / Soften what can soften |
| `reg.save_prompt` | Save this session? |
| `status.*` | Saving… / Checking… / Loading… / load failed / delete failed |
| `review.empty_sub` | Start with the body when ready. |
| `storage.unavailable_*` | Storage unavailable / Check memory card |
| `photo.*` | Photo memory / optional / saved locally / capture / skip / unavailable / entry still saved |

Existing body prompts (`body.area_prompt`, `body.sensation_prompt`, `body.intensity_prompt`) were tightened to match the companion spec.

---

## Screens wired to copy keys

| Screen | Keys used |
|--------|-----------|
| Home feed + subline | `HOME_FEED_READY`, `HOME_WHEEL_HINT`, `STORAGE_UNAVAILABLE_*` |
| Emotional tone | `TONE_PROMPT`, `TONE_ROUGH_OK` |
| Color picker / presets | `COLOR_FIELD_HINT`, `COLOR_PRESETS_PROMPT` |
| Memory menu / browse / error | `MEMORY_*` |
| Review empty | `REVIEW_EMPTY`, `REVIEW_EMPTY_SUB` |
| Grounding / breathing / anchor intro | `REG_GROUNDING_*`, `REG_BREATH_*`, `REG_ANCHOR_*` |
| Regulation save confirm | `REG_SAVE_PROMPT` |
| Status sublines | `STATUS_*` |
| Photo memory consent / capture / result | `PHOTO_*` |

Reflection text remains in `circe_reflection.c` (rule-based, observation-safe). Timeline empty states remain in `circe_timeline_empty_copy()` with handoff-aligned phrases.

---

## Regulation review cleanup

**Problem:** Regulation entry review showed body edit actions (`EDIT` → CHANGE TONE / COLOR / ADD SENSATION).

**Fix:** `entry_is_regulation()` in `circe_ui.c`:

- Hides `EDIT` on `CIRCE_FLOW_REVIEW` for regulation entries.
- Blocks `edit` button handler and redirects `CIRCE_FLOW_EDIT` back to review.

Regulation review actions: **DELETE**, **BACK** (from timeline), **HOME**.

Body check-in review unchanged: full summary + EDIT + DELETE.

---

## Language guardrails

**Use:** I noticed… / I saved… / Not knowing is allowed. / This is only an observation. / We can return to this later. / You can stop anytime.

**Avoid:** diagnosis, disorder, symptom, crisis, compliance, harsh firmware labels (NO DATA, INVALID, PICK ONE).

---

## Remaining hardcoded strings

Intentionally local (not copy keys):

- Button labels: SAVE, SKIP, HOME, DELETE, encoder hints (PRESS PAUSE, TURN PROMPT)
- Body area / sensation / tone preset names from `circe_entry_modes`
- Entry summary lines (`body … / tone … / regulation …`) in review feed
- Diagnostics worker summaries and technical save error codes
- Reflection rule output in `circe_reflection.c`
- Timeline category titles and empty-state lines in `circe_timeline.c`
- Anchor rotation prompts in `circe_regulation.c` (`s_anchor_prompts[]`)
- Pattern primary/subline text generated in `circe_patterns.c` (rule engine); shell copy uses `patterns.*` keys in `circe_copy.c`

These can migrate to copy keys in a later localization pass if needed.

---

## Stability

Copy polish phase: no new worker commands. Pattern Recognition MVP adds `CIRCE_WORKER_LOAD_PATTERNS` (separate from save path). Pattern scan uses lightweight `circe_timeline_load_pattern_context()` — not inside `run_save_entry()`. Worker stack unchanged (16 KB).
