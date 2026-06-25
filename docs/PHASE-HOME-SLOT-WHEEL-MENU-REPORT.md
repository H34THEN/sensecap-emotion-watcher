# Phase Report — Home Slot-Wheel Menu

**Date:** 2026-06-25  
**Scope:** Home navigation UX only

---

## 1. Home menu changes

Removed scrollable home button column (`create_scroll_panel` + 5 rows).

Added centered slot-wheel selector with prev/current/next hints and `N / 5` index.

Feed prompt: `> ready when you are`

---

## 2. Input behavior

| Input | Result |
|-------|--------|
| Rotate | Wrap selection across 5 options |
| Press | Open selected flow |
| Double press | No-op |
| Long press | SETTINGS |

---

## 3. LVGL object strategy

Module: `circe_home_wheel.c`

- 1 root + 4 labels (5 total widget count logged as 5 labels)
- Fixed allocation per home visit
- Poll at 50 ms via existing nav timer

---

## 4. Object reuse

Yes. Same labels for entire home session. Rotation only changes text and opacity styles.

Terminal feed initialized once on home entry — not destroyed per knob tick.

---

## 5. Build result

**PASS** — `circe.bin` ~732 KB

---

## 6. Flash result

**PASS** — `idf.py --port /dev/ttyACM1 -b 2000000 app-flash`

---

## 7. Hardware validation

| Check | Status |
|-------|--------|
| Boot | **Pending** manual |
| Slot rotation in place | **Pending** manual |
| Fast rotation no crash | **Pending** manual |
| All 5 options open | **Pending** manual |
| No home scrollbar | **Pending** manual |
| Save/review/regulation | **Pending** manual |

---

## 8. Regressions tested

Build-only. Manual regression checklist outstanding.

---

## 9. Remaining bugs

- Touch not mapped on home (encoder-only by design)
- SETTINGS reachable via both selecting SETTINGS and long-press

---

## 10. Commit hash

**Not committed** — awaiting hardware validation.

Suggested message:

```
feat(ui): replace home list with centered slot-wheel selector
```

---

## 11. Recommended next phase

1. Hardware validate slot wheel + regressions  
2. Optional: apply slot-wheel pattern to grounding sub-menu  
3. Color picker gradient (memory-safe, separate phase)

---

## Files changed

- `circe_home_wheel.c` / `circe_home_wheel.h` (new)
- `circe_ui.c` — home case, nav timer, open dispatch
- `CMakeLists.txt`
- `docs/ui/HOME_SLOT_WHEEL_MENU.md`
- `docs/ui/COMPANION_INTERFACE_SPEC.md`
