# Module: retake_photo

**Layer:** UI  
**Phase:** 2

## Purpose

Return from review to replace photo without losing other draft fields.

## Behavior

- Discards in-progress photo file on retake
- Re-invokes camera_capture
- Returns to entry_review

## Dependencies

- camera_capture
- entry_review
