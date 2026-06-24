# Storage Decision Study

Comparison of **JSON**, **JSONL**, and **SQLite** for Circe on SenseCAP Watcher microSD (FAT32).

**Recommendation summary:** **Hybrid — canonical JSON per entry + SQLite derived index + JSONL for export only.**

---

## Requirements

| Requirement | Weight |
|-------------|--------|
| Years of retention | Critical |
| Corruption recovery on SD removal / power loss | Critical |
| Query by date, privacy flags, strand | High |
| Export simplicity for GPU | High |
| ESP32-S3 RAM/CPU constraints | High |
| No schema rewrites later | Critical |
| Pattern analytics | Medium (on-device light; heavy on Hades) |
| Edit/delete integrity | High |

---

## Option A — JSON (one file per entry)

### Layout

```
entries/YYYY-MM-DD/<uuid>.json
```

### Pros

- **Best corruption isolation** — one bad file ≠ whole database lost
- Human-readable on SD card in any PC
- Atomic write: temp file + rename per entry
- Matches export record shape exactly
- Easy versioning per file (`schema_version` field)
- Edit = new file revision or in-place with backup `.bak`

### Cons

- Many files (10k+ over years) — FAT32 directory size OK but slower listing
- Cross-entry queries require index or full scan
- No built-in transactions across entry + photo metadata

### microSD reliability

- FAT32 cluster allocation; power-loss during rename mitigated by write-to-temp-then-rename
- fsync after entry commit recommended

### Export

- Tar/zip folder or concatenate to JSONL at export time

### Corruption recovery

- Validate JSON on read; quarantine corrupt file to `entries/_corrupt/`
- Rebuild index skipping corrupt IDs

**Score:** Recovery ★★★★★ | Query ★★☆☆☆ | Watcher CPU ★★★☆☆ | Export ★★★★☆

---

## Option B — JSONL (one file per day or monolithic)

### Layout variants

```
entries/2026-06-24.jsonl   # one line per entry
# OR
journal.jsonl              # entire history (avoid)
```

### Pros

- **Export-native** — copy file = dataset
- Append-only daily file is fast (one fsync per day batch)
- Streaming read for Hades ingestion
- Simple line-by-line recovery (skip bad lines)

### Cons

- **Day file corruption** loses all entries that day unless backup
- Edit/delete requires rewrite of whole day file (or tombstone lines + compaction job)
- Random access by UUID requires index anyway
- Concurrent entry save + compaction risky on embedded

### microSD reliability

- Append reduces rewrite wear vs SQLite pages
- Single large JSONL over years = high corruption blast radius — **reject monolithic**

### Export

- Best-in-class for GPU pipeline

### Corruption recovery

- Daily JSONL: recover prior lines, lose tail after corruption byte
- Needs line checksum optional (`// meta` per line — non-standard)

**Score:** Recovery ★★★☆☆ (daily) | Query ★★☆☆☆ | Watcher CPU ★★★★☆ | Export ★★★★★

---

## Option C — SQLite (single or few database files)

### Layout

```
index/circe.db
# optional: entries BLOB as JSON text in table
```

### Pros

- **Best queries** — date range, tags, privacy filters, strand order
- Transactions: entry + index + sync queue atomic
- Mature pattern for mobile embedded (with care)
- Pattern discovery aggregates via SQL

### Cons

- **Database corruption** — classic SD pain point; entire index may need rebuild
- FAT32 + sudden power loss during journal write
- Schema migrations required (mitigated by versioning table)
- Heavier flash/RAM on ESP32 (esp32-sqlite3 feasible but monitor stack)
- Binary file not user-inspectable on SD without tools

### microSD reliability

- Enable SQLite PRAGMAs: `journal_mode=DELETE` or `TRUNCATE` ( safer on FAT than WAL on some setups — **validate on hardware**)
- Frequent `PRAGMA synchronous=FULL` on commit (slower, safer)

### Export

- `SELECT ...` → JSONL export job

### Corruption recovery

- Rebuild entire DB from canonical JSON files if hybrid
- Without JSON canonical, recovery is hard — **unacceptable alone**

**Score:** Recovery ★★☆☆☆ alone | Query ★★★★★ | Watcher CPU ★★★☆☆ | Export ★★★★☆

---

## Comparison matrix

| Criterion | JSON / entry | JSONL / day | SQLite only | **Hybrid (JSON + SQLite)** |
|-----------|--------------|-------------|-------------|----------------------------|
| Corruption isolation | Excellent | Moderate | Poor alone | Excellent (canonical JSON) |
| Query performance | Poor | Poor | Excellent | Excellent (index) |
| Edit/delete | Simple file | Rewrite day | SQL UPDATE | JSON revision + index update |
| Export to GPU | Concat job | Copy daily files | SQL dump | JSONL export from index scan |
| Schema evolution | Per-file version | Per-line version | Migrations | Both |
| User inspect SD | Yes | Yes | No | Yes (entries folder) |
| ESP32 complexity | Low | Medium | Medium-high | Medium |

---

## Recommendation

### Primary: Hybrid architecture

1. **Canonical store:** one **JSON file per entry** at `entries/YYYY-MM-DD/<uuid>.json`
2. **Derived index:** **SQLite** at `index/circe.db` — pointers, dates, flags, strand order, sync state
3. **Export format:** **JSONL** generated on demand into `export/` — never the live store
4. **Rollups:** JSON files in `rollups/` — derived, rebuildable

### Rationale

- **Years of data:** per-entry JSON prevents catastrophic single-file loss
- **Pattern/strand queries:** SQLite serves Watcher UI without scanning SD
- **GPU pipeline:** export job emits JSONL + manifest (already in ml design)
- **Corruption recovery:** rebuild SQLite from `entries/**/*.json` — documented procedure in [EXPORT_AND_BACKUP_STRATEGY.md](EXPORT_AND_BACKUP_STRATEGY.md)
- **Schema evolution:** entry JSON carries `schema_version`; index has `schema_migrations` table; export includes both

### What not to do

- **Do not** use monolithic JSONL as only store
- **Do not** use SQLite alone without JSON canonical backup path
- **Do not** store photos in SQLite BLOBs — files in `photos/` with metadata in entry + index

### Phase 2 MVP scope

- JSON per entry + minimal SQLite (id, created_at, date_bucket, private_locked, training_ok, color_hex, intensity)
- Defer rollups to Phase 3; strand query from index + JSON load as needed

---

## Implementation notes (future firmware)

```c
// Commit sequence
1. Write entries/YYYY-MM-DD/<uuid>.json.tmp
2. fsync
3. rename → .json
4. BEGIN SQLite transaction
5. INSERT entry_index ...
6. COMMIT
7. Queue rollup job (async, low priority)
```

If step 4–6 fails: index rebuild marks entry orphaned until reindexed.

---

## Alternative if SQLite proves unstable on hardware

**Fallback:** JSON per entry + **JSONL daily index manifest** (`index/YYYY-MM-DD.ids.jsonl`) rebuilt on boot scan.

Lose SQL aggregates; keep recovery story. Spike in Phase 2 M3.
