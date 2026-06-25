# Home Slot-Wheel Menu

**Status:** Implemented — home screen only (`circe_home_wheel.c`)

Replaces the scrollable home button column with a centered rotary selector.

---

## Layout

```
CIRCE
online

> ready when you are

     QUICK NOTE        ← dim hint (previous)
     REGULATE          ← hero (selected)
     REVIEW            ← dim hint (next)
       3 / 5

rotate select  press enter   ← HUD subline
```

No scroll panel. No scrollbar. No terminal rows on home.

---

## Options (fixed set)

| Index | Label | Action |
|-------|-------|--------|
| 1 | BODY CHECK-IN | Body flow |
| 2 | QUICK NOTE | Quick presets |
| 3 | REGULATE | Grounding menu |
| 4 | REVIEW | Load latest entry |
| 5 | SETTINGS | Settings (`CIRCE_FLOW_MORE`) |

Wrap: SETTINGS ↔ BODY CHECK-IN

---

## Input

| Input | Behavior |
|-------|----------|
| Rotate CW/CCW | Change selection in place |
| Single press | Open selected option |
| Double press | No-op on home |
| Long press | Open SETTINGS |

Home disables global `circe_terminal_nav_poll()`; encoder handled by `circe_home_wheel_poll()`.

---

## LVGL object budget

Created once per home visit (5 labels inside 1 root):

| Object | Role |
|--------|------|
| `prev_lbl` | Dim previous option (18 px caption) |
| `current_lbl` | Selected option (28 px hero) |
| `next_lbl` | Dim next option (18 px caption) |
| `index_lbl` | `N / 5` |

Rotation updates `lv_label_set_text()` only — no `go_step()`, no feed rebuild.

Destroyed in `clear_content()` via `circe_home_wheel_destroy()`.

---

## Scope

**Home only.** Submenus (body, settings, regulation, etc.) still use terminal row lists.

---

## Related

- [PHASE-HOME-SLOT-WHEEL-MENU-REPORT.md](../PHASE-HOME-SLOT-WHEEL-MENU-REPORT.md)
- [COMPANION_INTERFACE_SPEC.md](COMPANION_INTERFACE_SPEC.md)
