# Phase Report — Grounding + Breathing MVP

**Date:** 2026-06-25  
**Scope:** First offline regulation tool; no storage/worker architecture changes

---

## 1. Menu entry added

Home scroll menu now includes **REGULATE** between QUICK NOTE and REVIEW.

Flow: `HOME → GROUNDING → BREATHING | BODY ANCHOR | SAVE NOTE`

---

## 2. Breathing implementation

Module: `circe_regulation.c`

- Cycle: INHALE 4 / HOLD 2 / EXHALE 6
- 3 rounds default
- LVGL objects: root, ring arc, orb, phase label, count label (5)
- 1 Hz `lv_timer` — text/style updates only
- Encoder: press pause/resume, double back, long end

---

## 3. Body anchor implementation

Four static prompts; encoder rotates selection, press advances.

- LVGL objects: root, prompt label (2)
- Duration tracked via same 1 Hz timer
- Complete on last prompt press or long-press end

---

## 4. Timer / object strategy

| Screen | LVGL objects | Timer | Per-tick work |
|--------|--------------|-------|---------------|
| Breathing | 5 | 1 s | label text, orb opacity |
| Body anchor | 2 | 1 s | duration counter only |

No object creation per tick. No SD or worker in timer callbacks.

---

## 5. Optional session save

End/complete → `CIRCE_FLOW_REGULATION_SAVE`

- SAVE → `circe_regulation_apply_to_entry()` + existing `enqueue_save_async()`
- SKIP → home
- Writes `entry_mode=regulation`, type, rounds, duration, session_completed

---

## 6. Schema fields added

| Field | Notes |
|-------|-------|
| `entry_mode: regulation` | New enum value |
| `regulation_type` | `breathing` / `body_anchor` |
| `rounds_completed` | int |
| `duration_seconds` | int |
| `session_completed` | bool |

Updated `schemas/emotion-entry.schema.json`. Old entries compatible.

---

## 7. Build result

**PASS** — `circe.bin` ~730 KB

---

## 8. Flash result

**PASS** — `idf.py --port /dev/ttyACM1 -b 2000000 app-flash`

---

## 9. Hardware validation

| Check | Status |
|-------|--------|
| Boot | PASS (serial OK) |
| REGULATE menu | **Pending** manual |
| Breathing cycle | **Pending** manual |
| Body anchor | **Pending** manual |
| Save / review / delete | **Pending** manual |
| Body check-in regression | **Pending** manual |
| Color picker regression | **Pending** manual |
| Diagnostics regression | **Pending** manual |

---

## 10. Regressions tested

Automated build only. Manual regression checklist outstanding on device.

---

## 11. Remaining bugs

- Breathing orb uses theme focus color, not live picked mood color (intentional for MVP)
- EDIT on regulation review entries still shows body-edit options (minor)
- Encoder hints in HUD subline only

---

## 12. Commit hash

**Not committed** — requires on-device validation per commit rules.

Suggested message:

```
feat(regulation): add grounding and breathing MVP
```

---

## 13. Recommended next phase

1. **Hardware validation** — breathing/anchor/save/review/delete + journal regressions
2. **Regulation review polish** — hide body-edit for regulation entries
3. **Optional audio/haptic cues** — separate phase if desired
4. **Color picker gradient** — memory-safe canvas only after stability sign-off

---

## Files changed

- `circe_regulation.c` / `circe_regulation.h` (new)
- `circe_entry.h` / `circe_entry.c` — regulation fields + JSON
- `circe_save.c` — regulation save validation
- `circe_conversation_engine.h` — flow steps
- `circe_ui.c` — menus, flows, review display
- `CMakeLists.txt`
- `schemas/emotion-entry.schema.json`
- `docs/regulation/GROUNDING_BREATHING_MVP.md`
- `docs/conversation/CIRCE_BODY_FIRST_FLOW.md`
- `docs/memory/ENTRY_LIFECYCLE.md`
