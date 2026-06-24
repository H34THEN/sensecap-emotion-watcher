# Module: pattern_reflection_engine

**Layer:** Application  
**Phase:** 3

## Purpose

On-device observational statistics — not diagnosis, not ML classification.

## Examples

- "Tight + shoulders appeared 4 times this week"
- "Stress rating > 7 often follows sleep rating < 4"
- Body sensation ↔ color co-occurrence tables

## Scope

- Rule-based aggregates initially
- Optional lightweight stats library on ESP32-S3

## Privacy

- Personal owner view includes private entries by default
- Exported reflections exclude `private_locked` unless user opts in

## Dependencies

- local_storage
- circe_assistant (display copy)
