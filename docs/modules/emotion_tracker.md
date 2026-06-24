# Module: emotion_tracker

**Layer:** Application  
**Phase:** 2

## Purpose

Capture named emotions and maintain history for pattern reflection.

## Emotion picker

- Scrollable list + "Not sure yet" + Skip
- Custom emotions (user-added via calibration)
- Does not require selection when `flow_path == body_first`

## Data

- `emotion: string | null`
- `emotion_skipped: boolean`

## Pattern hooks

- Frequency by emotion label over time windows
- Co-occurrence with body sensations (observational stats)

## Dependencies

- local_storage
- pattern_reflection_engine (read)
