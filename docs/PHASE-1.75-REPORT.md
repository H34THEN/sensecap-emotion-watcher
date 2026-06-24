# Phase 1.75 Handoff Report — Memory and Data Architecture

Generated: 2026-06-24

---

## 1. Documents created

```
docs/memory/
├── README.md
├── MEMORY_ARCHITECTURE.md
├── ENTRY_LIFECYCLE.md
├── MOOD_STRAND_SPECIFICATION.md
├── PATTERN_DISCOVERY_DATA_MODEL.md
├── PHOTO_MEMORY_MODEL.md
├── VOICE_MEMORY_MODEL.md
├── GPU_PIPELINE_PLAN.md
├── MAGIC_MIRROR_VISUALIZATION_PLAN.md
├── EXPORT_AND_BACKUP_STRATEGY.md
├── STORAGE_DECISION_STUDY.md
└── SCHEMA_ADDITIONS.md
```

**12 files** — complete memory and data architecture for multi-year Circe journaling.

---

## 2. Storage recommendation

**Hybrid canonical + derived index:**

| Layer | Format |
|-------|--------|
| **Canonical entries** | One **JSON file per entry** (`entries/YYYY-MM-DD/<uuid>.json`) |
| **Query index** | **SQLite** (`index/circe.db`) — rebuildable from JSON |
| **Export to GPU** | On-demand **JSONL** bundles — not live storage |
| **Rollups** | Derived **JSON** (`rollups/daily|weekly|monthly/`) |
| **Media** | JPEG/WAV **files** + sidecar metadata |

**Reject:** monolithic JSONL as sole store; SQLite-only without JSON backup.

**Phase 2 MVP:** JSON per entry + minimal SQLite index. Rollups Phase 3.

Full analysis: [STORAGE_DECISION_STUDY.md](STORAGE_DECISION_STUDY.md).

---

## 3. Schema additions recommended

Promote to schema v **1.1.0** at firmware start:

- `entry_mode`, `interaction_mode`
- `lifecycle_state`, `revision`
- `local_date`, `timezone_at_capture`
- `sync_status` (Phase 3)
- Normative `assistant_transcript` shape
- `_extensions` object for forward compatibility

Sidecar schemas: `photos/*.meta.json`, `voice/*.meta.json`, `patterns/observations/*.json`.

Full list: [SCHEMA_ADDITIONS.md](SCHEMA_ADDITIONS.md).

---

## 4. Backup strategy recommendation

| Priority | Action |
|----------|--------|
| **P0** | Atomic JSON writes + SQLite rebuild from entries on corruption |
| **P1** | User monthly copy of `/circe/entries/` to PC |
| **P2** | Weekly **encrypted** bundle to NAS (`circe-backup-*.circe.enc`) |
| **P3** | Incremental backup tool (post-MVP) |

Phase 2: local SD only, no encryption. Document restore procedure in firmware diagnostics.

Details: [EXPORT_AND_BACKUP_STRATEGY.md](EXPORT_AND_BACKUP_STRATEGY.md).

---

## 5. GPU integration recommendation

**Push model only** — Watcher never auto-uploads.

```
Watcher → user export (JSONL + manifest) → Hades ingest → GPU jobs → PatternObservation + suggestions → LAN pull to Watcher
```

- Embeddings live on **Hades**, not Watcher SD
- Suggestions are **opt-in display** — never auto-write journal
- Phase 2: storage only; Phase 3: manual export; Phase 5: GPU embeddings

Details: [GPU_PIPELINE_PLAN.md](GPU_PIPELINE_PLAN.md).

---

## 6. Magic Mirror recommendation

**Tiered sync** — default tier 0 only:

- Public strand blocks + missing-day voids
- No private entry content, no photos, no audio
- Weekly/monthly rollup JSON cached on NAS
- Module **MMM-Circe** renders blanket + strand + pattern cards
- No animation; autism-friendly static layout

Details: [MAGIC_MIRROR_VISUALIZATION_PLAN.md](MOOD_STRAND_SPECIFICATION.md).

---

## 7. Future risks

| Risk | Mitigation |
|------|------------|
| SQLite corruption on FAT32 | JSON canonical + rebuild script |
| SD removal mid-write | temp + rename + fsync |
| Schema drift across years | `_extensions` + migration tool |
| Photo storage dominates SD | Retention settings + optional purge |
| User expects cloud backup | Clear copy: local-first |
| GPU suggestions feel diagnostic | Template validator + disclaimer keys |
| Mirror leaks private data | Tier 0 payload audit; private day indicator only |
| Voice retention legal/sensitivity | Separate consent flags; default off |
| 10k+ JSON files FAT perf | Index all queries; avoid directory scan |
| Encrypted backup passphrase loss | User education; no key escrow |

---

## 8. Remaining unknowns

1. SQLite `journal_mode` stability on Watcher hardware spike
2. Realistic fsync latency on microSD during save UX
3. Watcher LAN HTTP server vs NAS-only mirror cache
4. On-device encryption feasibility (ESP32 AES performance)
5. Hades ingest stack choice (user environment)
6. Mirror module bandwidth for month blanket at 4K
7. Incremental backup size vs full backup on slow Wi-Fi
8. Whether `merge_adjacent_identical` strand setting is wanted
9. Cross-device restore `device_id` provenance policy
10. Photo face-blur pipeline location (Hades only vs export tool)

---

## 9. Should firmware implementation begin?

**Yes — Phase 2 may begin**, with scope constrained to:

1. Hybrid storage spike (JSON + SQLite rebuild)
2. Conversation-driven body-first vertical slice (Phase 1.5)
3. Schema v1.1.0 fields required for memory (`entry_mode`, `lifecycle_state`, `revision`, `local_date`)
4. **Defer:** rollups, sync, export, GPU, Mirror, voice, encryption

Memory architecture is defined enough that firmware should **implement storage contracts now**, not invent layouts during coding.

---

## 10. Recommended Phase 2 firmware prompt

```markdown
# PHASE 2 — CIRCE Firmware MVP (Storage + Conversation)

Read mandatory:
- docs/memory/STORAGE_DECISION_STUDY.md
- docs/memory/MEMORY_ARCHITECTURE.md
- docs/memory/ENTRY_LIFECYCLE.md
- docs/memory/SCHEMA_ADDITIONS.md
- docs/conversation/CIRCE_CORE_PERSONALITY.md
- docs/conversation/CIRCE_BODY_FIRST_FLOW.md
- docs/conversation/CIRCE_ENTRY_TYPES.md
- docs/roadmap/phase-2-implementation.md

Implement:

1. **circe_storage** — hybrid layout:
   - entries/YYYY-MM-DD/<uuid>.json (atomic write)
   - index/circe.db (minimal entry_index table)
   - rebuild_index() from JSON scan

2. **Schema v1.1.0** — entry_mode, lifecycle_state, revision, local_date

3. **Conversation vertical slice** — body_only + quick modes, pattern keys

4. **Entry lifecycle** — create + hard delete + partial save; no edit/export/sync yet

5. **Diagnostics** — index rebuild, entry count, last save path

Do NOT implement: rollups, strand UI, Mirror sync, export bundles, photos, voice, GPU, encryption.

Success: reboot-surviving entries; index rebuild matches JSON; body-first save in ≤4 taps quick mode.

After hardware validation, Phase 2b adds mood strand read path from index (MOOD_STRAND_SPECIFICATION.md).
```

---

**Phase 1.75 complete. Stop — no coding in this phase.**
