# Module: delete_entry

**Layer:** Application  
**Phase:** 2

## Purpose

Permanent local removal of entry and associated photo.

## Behavior

- Confirmation dialog
- Delete JSON + JPEG + index row
- Remove pending sync_queue jobs

## Future

- Propagate delete to LAN services (Phase 3)

## Dependencies

- local_storage
- sync_queue
