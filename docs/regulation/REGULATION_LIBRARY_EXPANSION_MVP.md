# Regulation Library Expansion MVP

**Status:** Implemented in firmware (`circe_regulation.c`)  
**Reference:** `docs/CIRCE_future_functionality_roadmap_reference.md` — Section 6

---

## Purpose

Expand offline regulation tools beyond breathing and body anchor. Visual-first, no voice/camera/cloud, safe to stop anytime, worker save only on user confirm.

---

## Regulation menu (HOME → REGULATE)

| Option | Type key |
|--------|----------|
| BREATHING | `breathing` |
| BODY ANCHOR | `body_anchor` |
| 5-4-3-2-1 | `grounding_54321` |
| SENSORY RESET | `sensory_reset` |
| BILATERAL TAP | `bilateral_tap` |
| BACK | — |

SAVE NOTE removed from menu (body check-in remains on home wheel).

---

## Tool 1 — 5-4-3-2-1

Five copy-driven steps (`reg.54321_step_*`). Encoder rotate = prev/next; press = next; double press = back; long press = end incomplete.

Save JSON: `regulation_type: grounding_54321`, `steps_completed`, `duration_seconds`, `session_completed`.

---

## Tool 2 — Sensory reset

Low-input prompts on dark background, dim text opacity. Same step navigation as 54321.

Save JSON: `regulation_type: sensory_reset`, `steps_completed: 5`.

---

## Tool 3 — Bilateral tap

Two fixed dots (left/right). One LVGL timer alternates side opacity — no per-pulse object creation. Default pace ~1000 ms; rotate adjusts 700–1500 ms. Press = pause/resume; long press = end.

`rounds_completed` = left→right cycle count (one full pair per cycle).

---

## Controls summary

| Tool | Rotate | Press | Double press | Long press |
|------|--------|-------|--------------|------------|
| Breathing | — | pause/resume | back | end |
| Body anchor | prev/next prompt | next | back | end |
| 5-4-3-2-1 | prev/next step | next | back | end |
| Sensory reset | prev/next step | next | back | end |
| Bilateral tap | speed | pause/resume | back | end |

---

## Save path

Existing flow: session complete → `CIRCE_FLOW_REGULATION_SAVE` → user SAVE → worker `circe_regulation_apply_to_entry()` → reflection.

Optional JSON field: `steps_completed` (when > 0).

No SD writes during timer ticks.

---

## Review display

Regulation entries show friendly type label + duration/steps or cycles. No body EDIT actions (regulation review cleanup preserved).

---

## Object / timer strategy

- Objects created once on session start; timers deleted in `circe_regulation_destroy()`
- Step tools share `build_step_ui()` + `step_timer_cb()` (duration only)
- Bilateral uses single alternating timer callback

---

## Language guardrails

Observation/support copy only. No treatment, therapy, diagnosis, or “calm down” language.

---

## Known limitations

- Sensory dim is session-local (does not change global theme NVS)
- Bilateral has no audio/haptics
- REVIEW → TODAY timeline display issue remains a separate known bug

---

## Future

Emotional Color Intelligence, full regulation builder, audio cues — later roadmap phases.
