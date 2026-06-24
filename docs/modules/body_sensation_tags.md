# Module: body_sensation_tags

**Layer:** UI  
**Phase:** 2 — **Critical path**

## Purpose

Primary capture for body-first emotional exploration.

## UI

- Body area map (touch) + list fallback (encoder)
- Multi-select sensation chips grouped by category
- "Nothing noticeable" clears selections

## Outputs

- `body_areas[]`
- `body_sensations[]`

## Taxonomy

See [body-sensation-system.md](../design/body-sensation-system.md).

## Accessibility

- Large tap targets on body map
- High contrast selected state
- Calm styling for `meltdown_warning`, `shutdown_feeling`

## Dependencies

- pattern_reflection_engine
- calibration_mode (hidden tags, favorites)
