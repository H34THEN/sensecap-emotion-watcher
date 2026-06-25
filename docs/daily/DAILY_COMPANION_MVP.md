# Daily Companion MVP

**Phase:** Daily Companion MVP  
**Firmware:** `firmware/circe`  
**Status:** Implemented — contextual home feed via worker-backed daily summary

---

## Purpose

Make CIRCE feel present across the day using **local time** and **local memory**. Not reminders, notifications, streaks, or nagging.

---

## Home behavior

On **HOME** entry:

1. Render safe default feed (`Ready when you are.`)
2. Create slot-wheel unchanged
3. Enqueue `CIRCE_WORKER_LOAD_DAILY_COMPANION` if worker free
4. Update feed lines in place when worker completes (only if still on HOME)

Feed + subline update only — no home rebuild, no new menu items.

---

## Time windows (local hour, when time set)

| Window | Hours |
|--------|-------|
| Morning | 05:00–11:59 |
| Afternoon | 12:00–16:59 |
| Evening | 17:00–21:59 |
| Night | 22:00–04:59 |

When time unset: neutral copy, no time-specific greeting.

---

## Daily summary fields

| Field | Source |
|-------|--------|
| `entries_today` | Non-regulation entries in TODAY timeline (max 16) |
| `regulation_today` | Regulation sessions today |
| `high_intensity_today` | Intensity ≥ 8 |
| `repeated_body_area` | Most frequent body area (≥2) |
| `primary_line` / `subline` | Built copy for home feed |

---

## Worker strategy

```
HOME → default feed → CIRCE_WORKER_LOAD_DAILY_COMPANION
  → circe_timeline_load_category(TODAY)
  → circe_daily_load()
  → lv_async_call → update feed if step == HOME
```

No SD reads on LVGL task. If worker busy, home keeps default feed.

---

## Copy rules

Gentle, private, non-diagnostic. Avoid streak/guilt/reminder language.

Examples: Good morning. / No entry yet today. / Quiet records are allowed. / Your body has been heard.

---

## Privacy

Local only. No cloud, notifications, microphone, or camera.

---

## Voice

No new audio in this phase. Voice cues unchanged (optional SOFT tones elsewhere).

---

## Known limitations

- **REVIEW → TODAY display bug** unchanged
- **Camera capture** still scaffolded
- Manual on-device validation gap for feed refresh after save/regulation return
- Summary capped at 16 TODAY index rows

---

## Future path

- Richer daily summary screen (optional)
- Evening close-the-day prompt flow
- Deeper pattern tie-in without rewriting timeline
