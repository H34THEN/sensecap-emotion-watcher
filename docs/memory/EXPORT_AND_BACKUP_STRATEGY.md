# Export and Backup Strategy

JSON exports, encrypted backups, NAS integration, restore, migration, and schema versioning.

Related: [ENTRY_LIFECYCLE.md](ENTRY_LIFECYCLE.md), [GPU_PIPELINE_PLAN.md](GPU_PIPELINE_PLAN.md)

---

## Export types

| Type | Purpose | Initiator | Contents |
|------|---------|-----------|----------|
| **Training export** | Hades GPU | User | Consented JSONL + optional media |
| **Full backup** | Disaster recovery | User | All entries + photos + voice + index snapshot |
| **Mirror sync** | Visualization | Auto LAN | Public aggregates tier 0–2 |
| **Portable read** | Human archive | User | JSON folder zip, no index required |

---

## JSON export formats

### Training export (JSONL)

```
export/circe-export-<ts>/
  manifest.json
  entries.jsonl       # one EmotionEntry per line, anonymized device_id
  photos/             # optional
  voice/              # optional
  checksums.sha256
  README.txt
```

### Full backup

```
backup/circe-backup-<ts>/
  manifest.json
  entries/            # full tree copy or tar
  photos/
  voice/
  index/circe.db
  rollups/
  patterns/
  config/
  checksums.sha256
```

### Incremental backup (future)

```
backup/incremental-<ts>/
  manifest.json
  since_backup_id: "<previous manifest hash>"
  changed_entry_ids.json
  entries/            # only changed files
```

---

## Encrypted backups

### Goals

- Protect SD theft / NAS breach
- Keys never on Watcher cloud

### Design

| Element | Choice |
|---------|--------|
| Algorithm | AES-256-GCM |
| Key derivation | Argon2id from user passphrase (on PC or Watcher numeric PIN — **Phase 3+**) |
| Encrypted artifact | `circe-backup-<ts>.circe.enc` (tar.gz encrypted) |
| Manifest cleartext | Optional `manifest.json` outside blob with entry count only |

### Workflow

1. User creates backup on Watcher → plaintext to SD `backup/` OR streams to LAN
2. User runs `circe-backup-tool encrypt` on PC before NAS upload
3. Passphrase stored in password manager — **not** on Watcher by default

### Watcher-native encryption (optional future)

- Encrypt **photo/voice files** at rest with key in NVS + user PIN
- Entries JSON remain readable for firmware simplicity OR encrypt sensitive fields only

**Phase 2:** plaintext local storage; document encryption as Phase 3 NAS path.

---

## NAS backups

### Recommended home architecture

```
NAS:/circe/
  watcher-<device_id>/
    mirror-cache/     # from sync_queue
    backups/
      circe-backup-2026-06-24.circe.enc
    exports/
      circe-export-2026-06-24/
```

### Sync methods

| Method | Direction | Use |
|--------|-----------|-----|
| Watcher → NAS SMB | Push nightly optional | Backup + mirror cache |
| User PC copy SD | Manual | Air-gapped backup |
| Hades reads NAS exports | Pull | GPU ingest |

Watcher never requires NAS online for journaling.

---

## Restore workflow

### Full restore from backup

1. Verify `checksums.sha256`
2. Decrypt if needed
3. Stop Circe on Watcher (maintenance mode)
4. Copy `entries/`, `photos/`, `voice/`, `index/`, `config/` to SD `/circe/`
5. Run **index rebuild** if SQLite missing/corrupt
6. Run **rollup rebuild** (background)
7. Validate sample entries against schema
8. Circe: "Restored from backup."

### Partial restore (single entry)

Import one JSON from backup → assign new id OR restore same id if not present.

Conflict policy: **keep newer `updated_at`** unless user chooses overwrite.

### Corruption recovery (no backup)

1. Scan `entries/**/*.json` — quarantine invalid
2. Rebuild SQLite from valid files
3. Regenerate rollups
4. Report lost ids to user

Document in settings → Diagnostics.

---

## Migration workflow

### Schema version bump

Each entry has `schema_version`. Index has `db_schema_version`.

```
migrations/
  1.0.0_to_1.1.0.json   # field mapping rules
```

### Migration job

1. Backup before migrate (mandatory prompt)
2. For each entry: read vN → transform → write vN+1 → update index
3. Atomic per entry (temp file rename)
4. Log to `logs/migration.jsonl`

### Forward compatibility

- Unknown fields in JSON: preserve in `_extensions` object (see SCHEMA_ADDITIONS)
- Older firmware reads new files: ignore unknown fields if `additionalProperties` relaxed in reader

### Cross-device migration

Export full backup from Watcher A → restore to Watcher B.

`device_id` in entries may remain source device for provenance OR remap — user choice at restore.

---

## Schema versioning policy

| Change type | Version bump | Example |
|-------------|--------------|---------|
| Add optional field | Minor 1.0 → 1.1 | `entry_mode` |
| Add required field | Major 2.0 | breaking |
| Rename field | Major + migration | — |
| Strand visual | `strand_version` separate | — |

Export manifest lists:

```json
{
  "schema_versions_present": ["1.0.0", "1.1.0"],
  "lowest_reader": "1.0.0",
  "migration_tool": "circe-migrate 1.1.0"
}
```

---

## Backup strategy recommendation

| Tier | Frequency | Method | Encryption |
|------|-----------|--------|------------|
| **Operational** | Continuous | microSD canonical JSON | No (Phase 2) |
| **Local copy** | Monthly | User copies SD `entries/` to PC | User choice |
| **NAS** | Weekly optional | Watcher or PC push encrypted bundle | **Yes** |
| **Training** | On demand | Export to Hades inbox | TLS in transit |

Minimum viable: **monthly manual SD copy** + on-device corruption rebuild.

Ideal: **weekly encrypted NAS backup** + daily mirror cache sync (non-private aggregates only).

---

## Retention

| Artifact | Retention |
|----------|-----------|
| Export bundles on SD | User deletes manually; warn if > 1 GB |
| NAS backups | User rotates; suggest keep last 12 |
| Tombstones | 30 days optional |
| Migration logs | 1 year |

---

## Circe copy

- Backup start: "Creating backup — this stays on your devices."
- Restore: "This replaces what's on your Watcher. Continue?"
- Export: "N entries included. Photos: M. Nothing uploads automatically."

---

## Related tools (future, not Phase 2 firmware)

- `circe-backup-tool` — CLI encrypt/verify/decrypt on PC
- `circe-migrate` — schema migration
- `circe-reindex` — rebuild SQLite from JSON
