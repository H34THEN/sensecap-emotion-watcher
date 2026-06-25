# Memory Timeline MVP

Local timeline browser for saved entries. All index and JSON reads run on the worker task — never on the LVGL thread.

---

## Purpose

Review opens a **memory menu** instead of jumping to the latest entry. Users browse by time period, rotate through compact summaries, and open detail / delete without leaving the terminal UI.

---

## Categories

| Category | Filter |
|----------|--------|
| **TODAY** | Index `local_date` = today; if time unset, includes entries with empty `local_date` |
| **YESTERDAY** | Previous calendar date (requires time set) |
| **THIS WEEK** | Last 7 days by `local_date` (fallback: `created_at` date prefix) |
| **ALL ENTRIES** | All active index rows, newest first |

Cap: **16 entries** per category. If more exist: subline shows `recent` and status `showing recent 16`.

---

## Worker commands

| Command | Role |
|---------|------|
| `CIRCE_WORKER_LOAD_TIMELINE` | Rebuild index if dirty, scan index, load summaries |
| `CIRCE_WORKER_LOAD_ENTRY` | Load full entry by id for detail view |
| `CIRCE_WORKER_DELETE_ENTRY` | Unchanged — hard delete + index update |

UI flow:

1. Show `Loading memory...`
2. Post worker load
3. `lv_async_call` completion → empty / error / browse screen

Timeline category load uses **`item_from_json_path()`** (lightweight fields only, heap index rows) — not full `circe_entry_load()`. See `docs/bugs/WORKER_STACK_OVERFLOW_TIMELINE_LOAD.md`.

---

## Entry summary format

### Body check-in

```text
18:42
CHEST / TIGHT / 9
TONE OVERWHELMED
COLOR #8A4DFF
```

Missing tone → `TONE UNKNOWN`. Skipped color → `COLOR SKIPPED`.

### Regulation

```text
19:10
REGULATION BREATHING
72s / 3 rounds
COMPLETED YES
```

### Legacy / sparse

```text
--:--
ENTRY SAVED / 5
TONE UNKNOWN
COLOR SKIPPED
```

---

## UI flows

```
REVIEW (home / reflection) → MEMORY MENU → category → BROWSE (rotate) → VIEW → detail
                                                      → DELETE → confirm → reload category
```

Fixed terminal feed lines updated on encoder rotation. Buttons: **VIEW**, **DELETE**, **BACK**.

Encoder press on browse = **VIEW** (same as button).

---

## Empty states

| Category | Line 1 | Line 2 |
|----------|--------|--------|
| Today | no entries recorded today | start with the body when ready |
| Yesterday | no entry yesterday | quiet days are allowed |
| This week | not enough memory yet | check-ins will appear here |
| All | no entries found | storage is ready |

If time unset on Today: line 2 also notes UNSET folder.

---

## Index errors

If storage unavailable or collect fails:

```text
memory index needs repair
run diagnostics
```

Link to Diagnostics screen. No automatic rebuild from LVGL.

---

## Delete behavior

- From browse or detail (memory context): confirm → worker delete → reload current category
- On failure: `couldn't delete — entry remains saved`
- Index rewrite uses existing worker delete path

---

## Reflection display

Reflection text is **not stored** on entries yet. Detail view uses existing body/tone/color/regulation summary only. No reflection generated during browse.

---

## Privacy

Local SD only. No export, sync, or cloud.

---

## Limitations (MVP)

- No advanced filters
- 16-entry cap per category
- Yesterday / week require sensible manual time (or system time)
- Not a slot-wheel category menu (terminal rows only)
- **Body Heat Map** uses same index loader as patterns (`docs/body/BODY_HEAT_MAP_MVP.md`)
- **Daily Companion** reuses TODAY timeline load on worker for home feed (see `docs/daily/DAILY_COMPANION_MVP.md`)

---

## Modules

| File | Role |
|------|------|
| `circe_timeline.c/h` | Load, cache, format |
| `circe_memory_browser.c/h` | Encoder browse UI |
| `circe_index.c` | `circe_index_list_collect` |
| `circe_worker.c` | Timeline + entry load commands |
| `circe_ui.c` | Flow wiring |

---

## Future

Timeline index path feeds **recent pattern reflection** after save, **Pattern Recognition** (`REVIEW → PATTERNS`), and **Body Heat Map** (`REVIEW → BODY MAP`). See `docs/patterns/PATTERN_RECOGNITION_MVP.md` and `docs/body/BODY_HEAT_MAP_MVP.md`.

Memory menu / browse hints and index-repair copy are centralized in `circe_copy.c` (`memory.*` keys). Empty-state phrases in `circe_timeline_empty_copy()` match the companion spec. See `docs/conversation/CONVERSATION_ENGINE_COPY_POLISH.md`.

**Memory menu:** TODAY · YESTERDAY · THIS WEEK · ALL ENTRIES · **PATTERNS** · BACK
