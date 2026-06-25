# Body Heat Map MVP

**Status:** Implemented in firmware (`circe_body_map.c`)  
**Reference:** `docs/CIRCE_future_functionality_roadmap_reference.md` — Section 3

---

## Purpose

Local, body-first summary of where sensations have appeared recently. Text/bar display only — no silhouette, no diagnosis, no cloud.

---

## Entry points

| Path | Action |
|------|--------|
| **REVIEW → BODY MAP** | Primary (memory menu) |
| **REVIEW → PATTERNS → BODY MAP** | Secondary shortcut |

Home slot-wheel unchanged.

---

## Scan window and caps

| Parameter | Value |
|-----------|--------|
| Window | Last 7 days when time is set; otherwise latest index entries |
| Hard cap | 16 entries (`CIRCE_BODY_MAP_SCAN_MAX`) |
| Minimum usable body entries | 2 (`CIRCE_BODY_MAP_MIN_ENTRIES`) |
| Display rows | Up to 5 (`CIRCE_BODY_MAP_MAX_ROWS`) |

Data loader: `circe_timeline_load_pattern_context()` — same lightweight path as Pattern Recognition.

**Included:** body check-in entries with a `body_area` field.  
**Excluded:** regulation entries.

---

## Aggregation and scoring

For each body area:

- `count` — appearances in scan window
- `max_intensity` — highest intensity seen
- `avg_intensity` — mean intensity (informational)
- `score = count + high_intensity_count` where high intensity is ≥ 8

Sort order:

1. Highest `score`
2. Highest `count`
3. Highest `max_intensity`

Top 5 areas shown.

---

## Display format

Fixed terminal feed rows (no scroll list, no object grid):

```text
BODY MAP

> CHEST       #######
> SHOULDERS   #####
> JAW         ###
```

Bar length scales to top row score (max 7 `#` characters). Uses ASCII `#` for Watcher readability.

Encoder rotate (when rows present): cycles detail subline for selected row:

- `CHEST appeared 4 times`
- `CHEST strongest signal: 9` (when max intensity ≥ 8)

Default subline when not rotating: `This is only an observation.`

---

## Worker path

1. UI shows `Loading body map...` and enqueues `CIRCE_WORKER_LOAD_BODY_MAP`
2. Worker runs `circe_body_map_load()` (heap-backed item buffer, no LVGL)
3. Completion via `lv_async_call` → routes OK / EMPTY / ERROR / STORAGE
4. Worker stack high-water logged (`log_worker_resources`)

---

## Empty / error states

| State | Line 1 | Line 2 |
|-------|--------|--------|
| Empty (&lt; 2 body entries) | Not enough body memory yet. | Start with the body when ready. |
| Storage unavailable | Memory unavailable. | Check diagnostics. |
| Scan failure | Couldn't load body map. | Try diagnostics. |

---

## Language guardrails

**Use:** appeared, shown up, body memory, signal, observation, no need to decide what it means  
**Avoid:** symptom, diagnosis, disorder, treatment, panic, disease, your body means, this proves, you are

Copy keys: `body_map.title`, `body_map.loading`, `body_map.empty_*`, `body_map.observation`, `body_map.error_*`, `body_map.storage_*`

---

## Known limitations

- No body silhouette or touchable diagram
- No medical interpretation or charts beyond text bars
- No cached summary persistence (recomputed each open)
- No export, cloud, ML, voice, or camera integration
- Does not fix REVIEW → TODAY browse display bug (separate issue)
- Daily Companion does not consume body map data in this phase

---

## Future path

- Small body silhouette with area overlays
- Today / this week view filters
- Optional rebuildable derived cache
- Tie-in to Daily Companion home lines when safe

See also: `docs/patterns/PATTERN_RECOGNITION_MVP.md`, `docs/memory/MEMORY_TIMELINE_MVP.md`
