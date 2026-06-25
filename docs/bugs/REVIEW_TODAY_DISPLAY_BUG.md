# REVIEW → TODAY Display Bug

**Status:** Fixed in RC1 stabilization (`circe_ui.c`)  
**Symptom:** Serial logs showed timeline load success (`timeline today: N items truncated=1`) but the browse screen showed no entry lines.

---

## Root cause

Worker completion initialized the memory browser **before** transitioning to the browse screen:

1. `circe_ui_worker_done()` called `circe_memory_browser_begin(&s_memory_browser, count, 0)`
2. Then `go_step(CIRCE_FLOW_MEMORY_BROWSE)`
3. `circe_ui_show_step()` always calls `clear_content()` first
4. `clear_content()` calls `circe_memory_browser_destroy()`, setting `active=false` and `count=0`
5. `CIRCE_FLOW_MEMORY_BROWSE` called `circe_memory_browser_refresh()` on an inactive browser → early return, empty feed

Timeline data in `circe_timeline_get_cache()` was loaded correctly; only the browser UI state was wiped.

The same lifecycle bug affected **Patterns** and **Body Map** browse screens (browser init before `clear_content()`).

---

## Fix

Initialize browse state **inside** `circe_ui_show_step()` after `clear_content()`:

- **MEMORY_BROWSE:** `circe_memory_browser_begin()` using `circe_timeline_get_cache()->count`, then `refresh()`
- **PATTERNS:** `pattern_browser_begin(s_patterns_result.count)` before `pattern_browser_refresh()`
- **BODY MAP:** `body_map_browser_begin()` before `body_map_browser_refresh()`

Removed pre-`go_step()` browser init from worker completion handler.

---

## Files changed

- `firmware/circe/main/circe_ui.c`

---

## Validation

After fix: REVIEW → TODAY should show formatted entry lines in the terminal feed (time, body/tone/color). Encoder rotate updates entry index subline.

**Manual on-device confirmation recommended** after app-flash.

---

## Workaround (pre-fix)

Use **ALL ENTRIES** or summary screens (**PATTERNS**, **BODY MAP**) when TODAY browse appeared empty despite logs.
