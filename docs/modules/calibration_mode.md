# Module: calibration_mode

**Layer:** Application  
**Phase:** 3

## Purpose

User tuning of UI defaults without editing entries.

## Settings

- Favorite colors
- Custom emotions
- Custom context tags
- Hidden body sensation tags
- Default flow path (body-first vs standard)
- Intensity slider range visibility
- Global sync enable (default off)

## Storage

- NVS for small settings
- `/circe/config/*.json` on SD for larger lists

## Dependencies

- color_picker, body_sensation_tags, context_tags, circe_assistant
