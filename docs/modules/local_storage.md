# Module: local_storage

**Layer:** Data  
**Phase:** 2

## Purpose

Persist entries, photos, and indexes on microSD (FAT32, up to 32 GB).

## API (proposed)

```
init() → bool
save_entry(EmotionEntry) → bool
load_entry(id) → EmotionEntry
delete_entry(id) → bool
query_by_date(start, end) → EmotionEntry[]
query_for_strand(start, end) → ColorRecord[]
```

## Storage

See [entry-schema.md](../schema/entry-schema.md).

## Decisions needed

- SQLite vs JSONL index
- Atomic write strategy (temp file + rename)
- Free space monitoring

## Constraints

- FAT32 filename limits
- SD removal handling (graceful errors)
