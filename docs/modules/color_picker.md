# Module: color_picker

**Layer:** UI  
**Phase:** 2

## Purpose

Visual color selection for emotional association.

## UI

- Palette grid or HSV wheel optimized for round 412×412 display
- Favorites row (from calibration)
- Recent colors strip
- Live strand preview dot

## Outputs

- `color_hex`
- `color_source: "picker" | "favorite"`

## Dependencies

- calibration_mode (favorites)
- hex_color_input (bidirectional sync)

See [color-system.md](../design/color-system.md).
