# Pattern Recognition MVP

**Status:** Implemented in firmware (`circe_patterns.c`)  
**Reference:** `docs/CIRCE_future_functionality_roadmap_reference.md` — Section 4

---

## Purpose

Offline, worker-safe scan of recent local entries. CIRCE surfaces up to three gentle observations — not diagnosis, not AI, not analytics dashboards.

---

## Entry point

**REVIEW → PATTERNS** (memory menu, alongside TODAY / YESTERDAY / THIS WEEK / ALL ENTRIES)

---

## Scan window and caps

| Parameter | Value |
|-----------|--------|
| Window | Last 7 days when time is set; otherwise latest index entries |
| Hard cap | 16 entries (`CIRCE_PATTERNS_SCAN_MAX`) |
| Minimum for scan | 3 entries (`CIRCE_PATTERNS_MIN_ENTRIES`) |
| Detect internally | Up to 5 patterns |
| Display | Up to 3 patterns (`CIRCE_PATTERNS_DISPLAY_MAX`) |

Data loader: `circe_timeline_load_pattern_context()` — lightweight JSON fields only, heap-backed index rows (same path as recent reflection context).

---

## Pattern rules (MVP)

| Type | Threshold | Example copy |
|------|-----------|----------------|
| Repeated body area | Same area ≥ 3 | `Chest has appeared often recently.` |
| Repeated sensation | Same sensation ≥ 3 | `Tight has appeared more than once.` |
| Repeated tone | Same non-UNKNOWN tone ≥ 3 | `OVERWHELMED has shown up recently.` |
| Strong signal | Intensity ≥ 8 in ≥ 2 entries | `Strong body signals appeared more than once.` + grounding subline |
| Regulation thread | ≥ 2 regulation entries | `You have returned to regulation recently.` |
| Color traits | ≥ 3 colored entries, same trait ≥ 3 | `Your recent colors have stayed mostly cool.` / muted / warm / dark / bright |

Color traits use derived fields when present (`color_temperature`, `color_saturation_label`, `color_brightness_label`) via `circe_color_intel`; old entries compute from `color_hex` in the lightweight loader.

Subline default: `This is only an observation.` (copy key `patterns.observation`)

---

## Worker path

1. UI enqueues `CIRCE_WORKER_LOAD_PATTERNS`
2. Worker runs `circe_patterns_scan()` (not inside save path)
3. Result returned via `lv_async_call` completion
4. UI routes: patterns browse / empty / none / error

Worker stack high-water logged before/after each job (`log_worker_resources`).

---

## UI behavior

- Terminal feed: primary line + subline (fixed labels, no scroll list)
- HUD subline: `1 / 3` pattern index
- Encoder rotate: previous / next pattern
- Actions: **REGULATE** (if any high-intensity pattern), **REVIEW**, **BACK**
- No scrollbar, no per-pattern object grids

---

## Empty / error states

| Condition | Copy |
|-----------|------|
| &lt; 3 entries | Not enough memory yet / Check-ins will appear here |
| ≥ 3 entries, no rules match | No strong pattern yet / Keep checking in when ready |
| Scan / index failure | Could not load patterns / Try diagnostics |
| Storage unavailable | Storage is unavailable / Check diagnostics |

---

## Language guardrails

**Use:** observation language, optional grounding offer  
**Avoid:** diagnosis, symptom, crisis, `you are`, `this means`, color-as-emotion claims

---

## Known limitations

- No charts beyond text bars, or export
- No persistence of pattern results (recomputed each open)
- Rules shared conceptually with post-save recent reflection but thresholds differ (reflection uses 2+ for some rules)
- Strand, cloud, ML, voice not involved

---

## Related features

- **Body Heat Map** — implemented; text/bar area summary (`docs/body/BODY_HEAT_MAP_MVP.md`). Shares scan loader with patterns.
- **Daily Companion** — reuses TODAY timeline scan for home summary (see `docs/daily/DAILY_COMPANION_MVP.md`)

See also: `docs/reflection/RECENT_PATTERN_REFLECTION_MVP.md`, `docs/memory/MEMORY_TIMELINE_MVP.md`
