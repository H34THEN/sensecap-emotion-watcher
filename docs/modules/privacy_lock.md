# Module: privacy_lock

**Layer:** UI  
**Phase:** 2

## Purpose

Per-entry control excluding data from sync and household views.

## Default

`private_locked = true`

## UI

- Toggle labeled "Keep private on this Watcher"
- When ON: entry never leaves device unless user exports manually with override (future settings)

## Enforcement

- sync_queue skips
- Magic Mirror hides by default
- export_dataset skips unless user global override (not planned Phase 2)
