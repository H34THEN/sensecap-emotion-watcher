# Recent Pattern Reflection MVP

Extends post-save reflection with **one** gentle observation from recent local entries. Worker-loaded; never parsed on the LVGL task.

---

## Architecture (Option A)

After a successful save to `CIRCE_FLOW_REFLECTION`:

1. Worker completes save, then calls `circe_reflection_load_recent_context()` **after** save stack unwinds.
2. Lightweight loader `circe_timeline_load_pattern_context()` reads up to **10** entries with minimal JSON field extraction (not full `circe_entry_load`).
3. UI calls `circe_reflection_generate()` which reads the static recent cache and applies pattern rules.
4. If history load fails or no pattern matches → immediate (current-entry) reflection.

No new worker command type. Save path stays primary; pattern failure never blocks save.

---

## History window

| Condition | Source |
|-----------|--------|
| Time set | `CIRCE_TIMELINE_CAT_THIS_WEEK` (last 7 days) |
| Time unset | `CIRCE_TIMELINE_CAT_ALL` (capped) |

Hard cap: **16 entries** (same as timeline MVP).

Requires **≥ 2 entries** in cache before any pattern runs.

---

## Pattern rules (priority — first match wins)

| Priority | Rule | Example text |
|----------|------|----------------|
| 1 | ≥ 2 recent intensity ≥ 8 (current also ≥ 8) | Strong signals have appeared more than once recently. |
| 2 | Current body area appears ≥ 2 times | Your chest has appeared more than once recently. |
| 3 | Current tone (not UNKNOWN) appears ≥ 2 times | OVERWHELMED has shown up before. |
| 4 | ≥ 1 regulation entry in recent window (body save) | You have used regulation recently. I can keep that thread. |
| 5 | Current color family matches ≥ 2 recent colors | Your recent colors have been mostly cool. / stayed near this range. |
| 6 | Immediate reflection (existing MVP rules) | I noticed your chest carried this entry. |

**Regulation save:** if ≥ 2 regulation entries in window → *This regulation session joins your recent grounding record.* else session immediate reflection.

---

## Subline behavior

| Pattern | Subline | REGULATE button |
|---------|---------|-----------------|
| High intensity repeat | Would grounding help? | Yes |
| Body / tone / color / regulation thread | This is only an observation. / No need to decide… | No (unless intensity rule) |
| Immediate intensity ≥ 8 | Would a grounding sequence help? | Yes |

---

## Color family (simple HSV)

No emotional meaning assigned. Buckets only:

- **Cool** — hue 120°–270°
- **Warm** — other saturated hues
- **Dark** — value &lt; 0.35
- **Bright** — high value, low saturation

Skipped colors excluded from counts.

---

## Language guardrails

**Use:** appeared, shown up, recently, I can remember, no need to decide, this is only an observation.

**Avoid:** means, proves, you are, disorder, crisis, panic pattern, diagnosis.

---

## Fallback

| Condition | Result |
|-----------|--------|
| Index/history load fails | Immediate reflection |
| &lt; 2 recent entries | Immediate reflection |
| No pattern match | Immediate reflection |
| Null entry | Saved. I can remember this with you. |

---

## Persistence

Patterns are generated live only. Optional JSON fields (`reflection_type = recent_pattern`) **not** written in this phase.

---

## Modules

| File | Role |
|------|------|
| `circe_reflection.c/h` | Context load + pattern engine |
| `circe_timeline.c` | Index/history load (reused) |
| `circe_worker.c` | Load context after save |

---

## Future

Full **Pattern Recognition** (Section 4) adds analytics screens, heat maps, and richer rules — not in this MVP.
