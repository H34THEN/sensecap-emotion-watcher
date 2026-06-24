# Small Screen Polish — Phase 3

Target: SenseCAP Watcher 412×412 (1.45") round display.

---

## Changes in Phase 3

### Scrolling

Long lists (15 body areas, 18 sensations, 5 quick presets) use an **LVGL scroll panel** inside the content area:

- Vertical scroll (`LV_DIR_VER`)
- Flex column layout with 4 px row padding
- Buttons at fixed 36 px height for consistent tap targets

### Encoder / dial

BSP initializes knob as **LV_ENCODER** input (`bsp_lvgl_init` → `bsp_knob_indev_init`).

Phase 3 wires a default `lv_group_t`:

- All buttons and the intensity slider are added to the group on each screen rebuild.
- Encoder indev is attached via `lv_indev_set_group()`.

**Hardware validation:** encoder focus and press-to-select not verified without device.

### Spacing

- Prompt label at y=32 (below Today strand bar)
- Content area 250 px tall, status bar at bottom
- Strand bar 24 px at top with 4 px gaps between color blocks

### Navigation

- **Back** on body area, sensation, intensity, color, quick pick, and edit screens
- **Cancel** on delete confirmation returns home without deleting
- Edit menu uses Back → Review

### Confirmations

- **Save:** dedicated `SAVE_DONE` step with `save.confirmed` + privacy notice before Home
- **Delete:** unchanged confirm step (`delete.confirm` → Yes / Cancel)

### Diagnostics

More → Storage health shows:

- SD mounted yes/no
- Index status
- Entry count
- Last storage error
- Standalone privacy copy
- Rebuild index (separate from self-test)
- Link to Today strand view

---

## Home layout (unchanged priority)

1. Body  
2. Quick  
3. Review  
4. More  

Today strand color blocks appear as a **read-only row** above the greeting (void block when no entries today).

---

## Not in Phase 3

- SquareLine export
- Full factory `pm.c` page manager
- Mood blanket multi-day view
- Encoder focus freeze / page animations
