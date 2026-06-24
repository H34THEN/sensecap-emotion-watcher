# Module: sync_queue

**Layer:** Data  
**Phase:** 3

## Purpose

Optional LAN synchronization of non-private entries to Magic Mirror / Hades.

## Default

Disabled (`sync_enabled = false` globally).

## Behavior

- Queue jobs on save when `!private_locked && sync_enabled`
- Background worker over Wi-Fi (LAN only)
- Retry with backoff; no cloud endpoints

## Payload

Minimal entry JSON + color record; photos optional separate sync setting.

## Dependencies

- local_storage
- privacy_lock enforcement

See [magic-mirror.md](../integration/magic-mirror.md).
