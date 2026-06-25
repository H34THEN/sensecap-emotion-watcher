# CIRCE UI File Map

Reference for manual visual layout and styling edits. Paths are relative to `firmware/circe/main/`.

**Rule of thumb:** Tune colors, spacing, and copy in theme/terminal/HUD files. Avoid SD reads, worker routing, and timer lifecycle in UI callbacks.

---

## START HERE FOR MANUAL UI EDITING

1. **`firmware/circe/main/circe_ui_tokens.h`** — positions, sizes, spacing (edit this first)
2. **`circe_theme.c/h`** — palette colors, button radius/height
3. **`circe_selector.c/h`** — single-focus menu behavior (not just layout)
4. **`circe_status_banner.c/h`** — magenta banner lifecycle + dimensions
5. **`circe_terminal.c/h`** — terminal feed layout
6. **`circe_color_picker.c/h`** — color field canvas
7. **`circe_regulation.c/h`** — regulation visuals
8. **`firmware/circe/main/assets/circe_homepage_bg.c/h`** — embedded Home HUD background (generated)
9. **`docs/circe_homepage_bg.png`** — source PNG for Home background (regenerate asset after edits)
10. **Avoid** worker/storage files unless debugging behavior

Workflow: `docs/ui/MANUAL_UI_EDITING_WORKFLOW.md`

### Screen-by-screen editing map

| Screen | Main layout file | Visual tokens | Copy file | Safe to edit |
| ------ | ---------------- | ------------- | --------- | ------------ |
| Home | `circe_home_wheel.c`, `circe_ui.c`, `circe_home_bg.c` | `CIRCE_UI_HOME_*`, `CIRCE_UI_HOME_BG_*` | `circe_copy.c` | Yes — tokens + bg toggle |
| Review menu | `circe_selector.c`, `circe_ui.c` | `CIRCE_UI_SELECTOR_*` | `circe_ui.c` labels | Yes — tokens |
| Today browser | `circe_memory_browser.c`, `circe_terminal.c` | `CIRCE_UI_TERMINAL_*` | `circe_timeline.c` | Yes — feed layout |
| Entry detail | `circe_ui.c`, terminal | `CIRCE_UI_TERMINAL_*`, `CIRCE_UI_CONTENT_*` | `circe_copy.c` | Partial — buttons in ui.c |
| Patterns | `circe_patterns.c`, `circe_ui.c` | `CIRCE_UI_TERMINAL_*` | `circe_copy.c` | Yes — feed |
| Body Map | `circe_body_map.c`, `circe_ui.c` | `CIRCE_UI_TERMINAL_*` | `circe_copy.c` | Yes — feed |
| Body Check-In | `circe_ui.c`, `circe_selector.c` | `CIRCE_UI_SELECTOR_*`, `CIRCE_UI_CONTENT_*` | `circe_copy.c` | Yes — tokens |
| Sensation | `circe_ui.c`, `circe_selector.c` | `CIRCE_UI_SELECTOR_*` | `circe_copy.c` | Yes |
| Intensity | `circe_ui.c` slider | `CIRCE_UI_CONTENT_*` | `circe_copy.c` | Partial — slider in ui.c |
| Tone | `circe_ui.c`, `circe_selector.c` | `CIRCE_UI_SELECTOR_*` | `circe_copy.c` | Yes |
| Color Field | `circe_color_picker.c` | `CIRCE_UI_COLOR_*` | `circe_copy.c` | Yes |
| Color Presets | `circe_ui.c` | `CIRCE_UI_CONTENT_*` | `circe_copy.c` | Partial |
| Reflection | `circe_reflection.c`, `circe_ui.c` | `CIRCE_UI_TERMINAL_*` | `circe_copy.c` | Yes — feed |
| Photo prompt | `circe_photo.c`, `circe_ui.c` | `CIRCE_UI_CONTENT_*` | `circe_copy.c` | Partial |
| Regulation menu | `circe_selector.c`, `circe_ui.c` | `CIRCE_UI_SELECTOR_*` | `circe_ui.c` | Yes |
| Breathing | `circe_regulation.c` | `CIRCE_UI_REG_*` | `circe_copy.c` | Yes — not timers |
| 5-4-3-2-1 | `circe_regulation.c` | `CIRCE_UI_REG_STEP_*` | `circe_copy.c` | Yes |
| Sensory Reset | `circe_regulation.c` | `CIRCE_UI_REG_STEP_*` | `circe_copy.c` | Yes |
| Bilateral Tap | `circe_regulation.c` | `CIRCE_UI_REG_DOT_*` | `circe_copy.c` | Yes |
| Settings | `circe_selector.c`, `circe_ui.c` | `CIRCE_UI_SELECTOR_*` | `circe_ui.c` | Yes |
| Voice Cues | `circe_selector.c`, `circe_voice.c` | `CIRCE_UI_SELECTOR_*`, banner | `circe_copy.c` | Yes |
| Diagnostics | `circe_selector.c`, `circe_ui.c` | `CIRCE_UI_SELECTOR_*` | `circe_ui.c` | Yes |
| Status Banner | `circe_status_banner.c` | `CIRCE_UI_STATUS_BANNER_*` | `circe_copy.c` | Yes |

**Banner lifecycle:** fixed in RC1 — banners clear on completion/navigation. If stuck, see `docs/ui/RC1_VISUAL_POLISH_PASS.md`.

---

## Global UI / Navigation

| File | Owns | Safe to tune | Do not edit casually |
|------|------|--------------|----------------------|
| `circe_ui.c` | All screen routing (`circe_ui_show_step`), button handlers, encoder nav timer, worker completion UI, `clear_content()` lifecycle, scroll panels, most `add_btn()` rows | Button label strings passed to `add_btn`, row spacing via `COL_W`, back-step targets | `go_step` routing, worker callbacks, save/delete paths, `clear_content()` destroy order |
| `circe_conversation_engine.h` | `circe_flow_step_t` enum — every screen ID | Enum names only if adding flows (avoid in RC1) | Renumbering/reordering breaks nav string IDs |
| `circe_conversation_engine.c` | Step → default prompt key mapping | Copy key routing for new steps | Flow logic |
| `circe_terminal.c` / `circe_terminal.h` | Fixed-label terminal feed (5 lines max), row height (`CIRCE_TERMINAL_ROW_H`), feed Y offset, blinking cursor timer, encoder nav back/sysmenu | `CIRCE_TERMINAL_*` layout constants, row label styles via `circe_terminal_style_row*` | Feed destroy/create pairing; do not add scroll lists |
| `circe_home_wheel.c` / `circe_home_wheel.h` | Home slot-wheel arc, option labels, focus ring | Label positions, arc radius, option spacing | Wheel destroy on every `clear_content()` — recreate happens in HOME step only |

**Lifecycle risk:** `circe_ui_show_step()` always calls `clear_content()` first. Any browser state (memory, patterns, body map) must be initialized **after** that in the target `case`, not in the worker handler before `go_step()`.

---

## Themes / Colors / Fonts

| File | Owns |
|------|------|
| `circe_theme.c` / `circe_theme.h` | All theme palettes including **Neon Terminal** (`CIRCE_THEME_NEON_TERMINAL`), NVS persistence, preview/commit, `circe_theme_style_*` helpers |
| `circe_fonts.c` / `circe_fonts.h` | LVGL font selection for HUD and terminal |
| `circe_hud.c` / `circe_hud.h` | Viewport chrome, status line, subline, prompt area, presence orb styling |

**Neon Terminal** palette tokens (edit in `circe_theme.c` palette table):

- `bg`, `surface`, `surface_alt` — background / panels
- `text`, `muted` — primary and secondary text
- `accent_primary`, `accent_secondary`, `accent_muted` — green / magenta accents
- `border`, `focus` — viewport and focus ring
- `danger` — delete/error emphasis

Theme persistence: NVS via `circe_theme_commit_preview()`. Do not rename NVS keys without migration.

**Safe edits:** hex colors in palette struct, `btn_min_h`, `btn_radius`, HUD line colors via theme style functions.

---

## Home Screen

| File | Role |
|------|------|
| `circe_home_bg.c/h` | Static HUD background image (Home only); show/hide on navigation |
| `assets/circe_homepage_bg.c/h` | Embedded 412×412 RGB565 bitmap (generated from `docs/circe_homepage_bg.png`) |
| `circe_home_wheel.c/h` | Slot-wheel UI and encoder selection |
| `circe_daily.c/h` | Daily companion summary lines (worker-loaded) |
| `circe_ui.c` | `CIRCE_FLOW_HOME` — feed init, wheel create, daily worker post |
| `circe_copy.c/h` | Home feed copy keys (`home.*`, `daily.*`) |

Home feed uses terminal feed fixed labels, not a scroll list. Daily lines update asynchronously after worker completes.

---

## Body Check-In Flow

| File | Role |
|------|------|
| `circe_ui.c` | `CIRCE_FLOW_BODY_AREA`, `BODY_SENSATION`, `INTENSITY`, `BODY_ADD_MORE`, `EMOTION_TONE` — button rows |
| `circe_entry.c/h` | Entry struct, body area/sensation lists, load/save helpers |
| `circe_copy.c/h` | Body prompt copy (`body.*`, `tone.*`) |

Area/sensation buttons are created in `circe_ui.c` from `circe_body_areas[]` / `circe_body_sensations[]` in entry module.

---

## Mood Color Picker

| File | Role |
|------|------|
| `circe_color_picker.c/h` | Canvas gradient field, touch mapping, magnifier, hex label |
| `circe_color_intel.c/h` | Local trait derivation (temperature, brightness, saturation labels) |
| `circe_ui.c` | `CIRCE_FLOW_COLOR_PICKER`, `COLOR_PRESETS`, save confirm |

**Warning:** Do not reintroduce the old LVGL rectangle color grid — it crashed from too many objects (`docs/bugs/COLOR_PICKER_GRID_CRASH.md`).

---

## Save / Reflection / Photo Prompt

| File | Role |
|------|------|
| `circe_worker.c/h` | Async save, delete, timeline, patterns, body map, photo capture |
| `circe_save.c` | Save orchestration, index update |
| `circe_reflection.c/h` | Post-save reflection text, recent pattern reflection rules |
| `circe_photo.c/h` | Photo consent/capture scaffold, unavailable fallback |
| `circe_ui.c` | Save confirm, reflection, photo flows |

---

## Memory Timeline / Review

| File | Role |
|------|------|
| `circe_timeline.c/h` | Category load (TODAY/YESTERDAY/WEEK/ALL), global cache, line formatting |
| `circe_memory_browser.c/h` | Encoder browse of cached entries, feed refresh |
| `circe_ui.c` | Memory menu, `MEMORY_BROWSE`, review/detail, delete confirm |
| `circe_index.c/h` | Index collect/filter |
| `circe_worker.c/h` | `CIRCE_WORKER_LOAD_TIMELINE`, `LOAD_ENTRY`, `DELETE_ENTRY` |

**TODAY display bug:** Fixed RC1 — browser init moved to `show_step` after `clear_content()`. See `docs/bugs/REVIEW_TODAY_DISPLAY_BUG.md`.

---

## Patterns Screen

| File | Role |
|------|------|
| `circe_patterns.c/h` | Pattern scan rules, result struct |
| `circe_worker.c/h` | `CIRCE_WORKER_LOAD_PATTERNS` |
| `circe_ui.c` | `CIRCE_FLOW_PATTERNS*` cases, `pattern_browser_*` |
| `circe_copy.c/h` | Pattern copy keys |

---

## Body Map Screen

| File | Role |
|------|------|
| `circe_body_map.c/h` | Aggregation, `#` bar formatting, scan window |
| `circe_worker.c/h` | `CIRCE_WORKER_LOAD_BODY_MAP` |
| `circe_ui.c` | `CIRCE_FLOW_BODY_MAP*` cases, `body_map_browser_*` |
| `circe_copy.c/h` | `body_map.*` copy keys |

---

## Regulation Screens

| File | Role |
|------|------|
| `circe_regulation.c/h` | Breathing, body anchor, 5-4-3-2-1, sensory reset, bilateral tap timers and phases |
| `circe_ui.c` | Regulation menu and step routing |
| `circe_copy.c/h` | Regulation copy (`reg.*`) |
| `circe_entry.c/h` | Regulation entry fields on save |

**Warning:** Timers must be deleted on exit (`circe_regulation_destroy()`). Do not create LVGL objects every tick.

---

## Settings / Diagnostics / Time / Voice

| File | Role |
|------|------|
| `circe_ui.c` | MORE menu, appearance, diagnostics UI |
| `circe_time_picker.c/h` | Manual date/time UI |
| `circe_time.c/h` | RTC/time helpers, date folders |
| `circe_theme.c/h` | Theme picker and persistence |
| `circe_voice.c/h` | Soft tone cues, NVS mode |
| `circe_worker.c/h` | TEST SAVE, probe, rebuild, health check |

---

## Safe Manual UI Editing Guidelines

### Safe to edit

- Text size (font helpers, label styles)
- Label positions and row Y offsets (`CIRCE_TERMINAL_FEED_Y_OFS`, HUD layout)
- Theme palette colors and button radius/height
- Spacing, border width, focus ring colors
- Copy strings in `circe_copy.c`
- Selected/dim row styles in `circe_terminal_style_row_label`

### Be careful editing

- Flow transitions and `go_step()` targets
- Worker completion handlers
- Timer create/delete in regulation and terminal cursor
- `circe_terminal_feed_destroy` / `init` pairing
- SD/index reads (must stay on worker)
- Save/delete/index code paths
- CMake source registration
- NVS key names
- Camera/voice hardware init

### Avoid

- Hundreds of LVGL objects (grids, scroll lists)
- Large canvas buffers on stack
- SD reads inside UI callbacks or encoder poll
- Recreating entire screens on every knob tick
- Scrollbars on circular UI
- Full flash / partition table changes

---

## RC1 Visual Editing Quick Reference

### If you want to change the Home screen

- `firmware/circe/main/circe_home_wheel.c`
- `firmware/circe/main/circe_daily.c`
- `firmware/circe/main/circe_ui.c`
- `firmware/circe/main/circe_theme.c`

### If you want to change selectable menu behavior

- `firmware/circe/main/circe_ui.c`
- `firmware/circe/main/circe_selector.c` / `circe_selector.h`
- `firmware/circe/main/circe_home_wheel.c`
- `firmware/circe/main/circe_encoder.c` (double/triple/long press)

### If you want to change the magenta status banner

- `firmware/circe/main/circe_status_banner.c` / `circe_status_banner.h`
- `firmware/circe/main/circe_ui.c` (when banner is shown)
- `firmware/circe/main/circe_theme.c` (if tying banner to theme tokens)

### If you want to change the color picker

- `firmware/circe/main/circe_color_picker.c`
- `firmware/circe/main/circe_color_intel.c`
- `firmware/circe/main/circe_theme.c`

### If you want to change Patterns / Body Map / Review

- `firmware/circe/main/circe_patterns.c`
- `firmware/circe/main/circe_body_map.c`
- `firmware/circe/main/circe_memory_browser.c`
- `firmware/circe/main/circe_timeline.c`
- `firmware/circe/main/circe_ui.c`

### If you want to change Regulation screens

- `firmware/circe/main/circe_regulation.c`
- `firmware/circe/main/circe_ui.c`
- `firmware/circe/main/circe_theme.c`

### If you want to change Voice settings

- `firmware/circe/main/circe_voice.c`
- `firmware/circe/main/circe_ui.c`
- `firmware/circe/main/circe_copy.c`

**Warnings:** Do not add object grids. Do not move SD work into UI code. Do not create timers without deleting them on exit. Do not place critical status text at the circular bottom edge — use `circe_status_banner_show()`.

---

## Screen capture cross-reference

Use with **`docs/ui/SCREEN_CAPTURE_GUIDE.md`**. For each screen: **path to reach it**, **layout file**, **copy**, **selector**, **banner**, **navigation**.

| Screen | Path | Layout / visual | Copy text | Selector | Status banner | Nav callbacks |
|--------|------|-----------------|-----------|----------|---------------|---------------|
| HOME | Boot | `circe_home_wheel.c`, `circe_daily.c` | `circe_copy.c` (`home.*`, `daily.*`) | `circe_home_wheel.c` | — | `circe_ui.c` `home_wheel_open_selected()` |
| BODY AREA | HOME → BODY CHECK-IN | `circe_selector.c`, `circe_ui.c` | `circe_copy.c` (`body.*`) | `circe_selector.c` | — | `circe_ui.c` `build_body_area_selector()` |
| BODY SENSATION | area → press | same | `circe_copy.c` | `circe_selector.c` | — | `build_body_sensation_selector()` |
| INTENSITY | sensation → press | `circe_ui.c` slider row | `circe_copy.c` | — (slider) | — | `next_intensity` btn handler |
| EMOTIONAL TONE | intensity flow | `circe_selector.c` | `circe_copy.c` (`tone.*`) | dynamic selector | — | `circe_ui.c` tone builder |
| COLOR FIELD | tone → picker | `circe_color_picker.c` | `circe_copy.c` | — | — | `circe_ui.c` color steps |
| COLOR PRESETS | picker flow | `circe_ui.c` | `circe_copy.c` | buttons / presets | — | save confirm handlers |
| SAVING / ENTRY SAVED | save | — | `circe_copy.c` (`STATUS_*`) | — | `circe_status_banner.c` | `circe_ui.c` `enqueue_save_async`, worker done |
| REFLECTION | post-save | `circe_reflection.c`, terminal | `circe_copy.c` | buttons | — | `circe_ui.c` reflection case |
| PHOTO PROMPT | reflection → photo | `circe_photo.c` | `circe_copy.c` | buttons | optional banner | `circe_ui.c` photo handlers |
| REVIEW MENU | HOME → REVIEW | `circe_selector.c` | menu labels in `circe_ui.c` | `s_memory_menu_items` | LOADING on sub-screens | `open_memory_menu()` |
| TODAY BROWSER | REVIEW → TODAY | `circe_memory_browser.c`, `circe_terminal.c` | `circe_timeline.c` formatters | encoder browse | LOADING MEMORY | worker `LOAD_TIMELINE` → `show_step` browser init |
| ENTRY DETAIL | browse → press | `circe_ui.c`, terminal | `circe_copy.c` | buttons VIEW/DELETE | DELETING | worker load/delete |
| PATTERNS | REVIEW → PATTERNS | `circe_patterns.c`, terminal | `circe_copy.c` | pattern browser | LOADING PATTERNS | `circe_ui.c` patterns cases |
| BODY MAP | REVIEW → BODY MAP | `circe_body_map.c`, terminal | `circe_copy.c` | body map browser | LOADING BODY MAP | `circe_ui.c` body map cases |
| REGULATE MENU | HOME → REGULATE | `circe_selector.c` | `s_regulation_menu_items` | selector | — | `CIRCE_FLOW_GROUNDING` |
| BREATHING | REGULATE → BREATHING | `circe_regulation.c` | `circe_copy.c` (`reg.*`) | — | — | regulation timers; triple → `go_home_safe()` |
| 5-4-3-2-1 / SENSORY / BILATERAL | REGULATE menu | `circe_regulation.c` | `circe_copy.c` | — | — | `circe_ui.c` regulation routing |
| SETTINGS | HOME → SETTINGS | `circe_selector.c` | `s_settings_menu_items` | selector | — | `CIRCE_FLOW_MORE` |
| VOICE CUES | SETTINGS → VOICE CUES | `circe_selector.c` | `circe_copy.c` voice keys | `s_voice_menu_items` | PLAYING/TONE/AUDIO | `circe_voice.c`, voice handlers |
| APPEARANCE | SETTINGS → APPEARANCE | `circe_theme.c` | `circe_copy.c` | theme selector | APPLIED banner | theme pick handlers |
| TIME SET | SETTINGS → TIME | `circe_time_picker.c` | `circe_copy.c` | time picker | — | `circe_time.c` NVS |
| DIAGNOSTICS | HOME → DIAGNOSTICS | `circe_selector.c` | `s_diagnostics_menu_items` | selector | TEST SAVE status | worker diagnostics jobs |
| Triple-press Home | any (most screens) | — | `circe_copy.c` `NAV_TRIPLE_HOME` hint | — | — | `circe_encoder.c`, `go_home_safe()` in `circe_ui.c` |

**Navigation module:** `circe_encoder.c/h` — double=back, triple=home (550 ms window), long=settings on terminal nav.

**Banner API:** `circe_status_banner_show()` / `show_timed()` / `hide()` / `reset()` / `dismiss_indefinite()` — triggered from `circe_ui.c` via `ui_show_status_*()`. Indefinite loading banners clear on `circe_ui_show_step()` and worker completion; timed banners keep auto-hide timer.

---

## Related docs

- `docs/ui/SCREEN_CAPTURE_GUIDE.md` — photo checklist for every screen
- `docs/ui/COMPANION_INTERFACE_SPEC.md` — screen behavior spec
- `docs/memory/MEMORY_TIMELINE_MVP.md` — timeline caps and browse
- `docs/releases/CIRCE_STANDALONE_MVP_RC1.md` — RC1 feature list and validation
- `docs/releases/CIRCE_RC1_HARDWARE_SIGNOFF.md` — hardware sign-off pass/fail table
