# Phase UI-Stability Report

Stability-only pass. No storage, save, delete, worker, or partition changes.

## 1. Strand / Test Strand crash

**Root cause:** `circe_storage_today_strand()` on LVGL task during `CIRCE_FLOW_STRAND` screen build, stacked with terminal feed destroy/create → LVGL stack overflow.

**Resolution:** Strand **disabled**. Menu entry removed; flow shows `strand unavailable` without SD access. Arc visualization suppressed.

See `docs/bugs/STRAND_LVGL_STACK_OVERFLOW.md`.

## 2. Terminal feed lifecycle

- `circe_terminal_feed_destroy()` stops timer first, nulls cursor/lines, deletes panel, zeros struct.
- Feed panel positioned below header (`CIRCE_TERMINAL_FEED_Y_OFS = 58`) to avoid overlap with title band.
- All LVGL entry points guard NULL.

## 3. Top text overlap

**Cause:** Terminal feed panel at viewport y=0 overlapped CIRCE heading and status in the same viewport.

**Fix:**

| Region | Content | Layout |
|--------|---------|--------|
| Viewport top | CIRCE | y=4 |
| Below | online | y=38 |
| Feed panel | terminal lines | y=58+ |
| Viewport | aligned screen y=72 | |
| Bottom | menu rows | actions area |

Home feed text simplified:

```
> ready to check in
> start with body
```

## 4. Font / readability

| Role | Size |
|------|------|
| Hero (CIRCE) | 32 px |
| Prompt / menu | 26 px (Montserrat 28 glyph) |
| Status / caption | 18 px min |
| Row height | 34 px |

## 5. Preserved behavior

Save, delete, review, worker async paths, FAT/SD probe — **unchanged**.

## Build / flash

```bash
cd firmware/circe && idf.py build
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

## Validation checklist

1. Boot — no loop
2. Home — CIRCE / online / feed / menu, no overlap
3. Settings/More — opens
4. Diagnostics — no Today Strand button; no crash
5. Save → Review → Delete — still works

## Remaining UI issues

- Diagnostics still runs `circe_storage_health_check()` on LVGL task (acceptable for now; monitor if crashes return).
- Strand feature fully disabled until worker-backed implementation.
- Long body/sensation lists still require many encoder steps.

## Commit recommendation

Single commit when hardware-validated:

```
fix(ui): disable strand SD scan, fix feed layout overlap, bump terminal fonts
```

Include: `circe_ui.c`, `circe_terminal.c`, `circe_hud.c`, `circe_fonts.c`, bug doc, this report.
