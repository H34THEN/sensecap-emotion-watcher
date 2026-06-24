# Privacy Model

## Default assumptions

| Setting | Default | Meaning |
|---------|---------|---------|
| `training_ok` | `false` | Entry not used for ML training |
| `private_locked` | `true` | Entry excluded from sync, export, household views |

**No cloud upload. No remote AI. No automatic sharing. No automatic training.**

User must explicitly opt in at save time (and can change defaults in settings).

---

## Threat model (proportionate)

CIRCE protects against:

- Accidental sharing of sensitive journal data to SenseCraft/cloud
- Household members seeing private entries on Magic Mirror
- Training pipelines ingesting data without consent
- Future LAN services accessing more than intended

CIRCE does **not** protect against:

- Physical device access (unencrypted SD card if removed)
- Network adversary on LAN if sync is enabled without TLS

Phase 2 should evaluate **optional SD encryption** — major unknown.

---

## Privacy lock (`private_locked`)

When `true`:

- Entry stored locally only
- Excluded from `sync_queue`
- Excluded from `export_dataset`
- Excluded from Magic Mirror default dashboards
- Excluded from Hades embedding batch jobs
- Included in on-device personal views for the device owner

When `false`:

- Entry may sync to configured LAN endpoints (if sync enabled globally)
- Still requires `training_ok` for training export

---

## Training consent (`training_ok`)

When `true` **and** `private_locked == false`:

- Entry eligible for `export_dataset` (user-initiated bundle)
- Eligible for future Hades GPU training pipelines

When `false`:

- Never included in exports or training corpora
- On-device pattern stats may still use entry **if** user enables "include private in personal patterns" (separate setting, default on for owner-only stats)

---

## Data residency

| Data | Location |
|------|----------|
| Entries | microSD on Watcher |
| Photos | microSD |
| Settings | NVS + optional SD config file |
| Sync copies | LAN services (opt-in) |
| Cloud | **None by default** |

---

## SenseCraft / factory firmware

Factory Watcher firmware integrates SenseCraft (MQTT, app binding). CIRCE custom firmware must:

1. Document whether SenseCraft stack is disabled or isolated.
2. Never route journal JSON through SenseCraft APIs.
3. Avoid `image analyzer` cloud LLM module for journal photos.

**Phase 2 decision:** fork factory firmware with SenseCraft compile flag vs. minimal BSP-only project.

---

## Delete semantics

`delete_entry` performs local hard delete. No tombstone sent anywhere if sync never ran.

If entry was synced: **Phase 3** — propagate delete to LAN services (requires mirror/hades delete API).

---

## Audit & transparency

Settings screen shows:

- Count of entries by privacy/training status
- Last export date
- Sync queue status
- List of LAN endpoints (if any)

---

## User-facing copy (save screen)

> **Private** — Only on this Watcher. (default ON)  
> **Help improve future models** — Allow this entry in exports you choose to create. (default OFF)

Never combine into single "share" toggle.

---

## Related

- [training_consent_toggle.md](../modules/training_consent_toggle.md)
- [privacy_lock.md](../modules/privacy_lock.md)
- [export_dataset.md](../modules/export_dataset.md)
