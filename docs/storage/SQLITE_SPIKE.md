# SQLite Index Spike — Phase 3

Date: 2026-06-24  
Status: **JSONL retained** — SQLite deferred

---

## Context

Phase 2 planned `index/circe.db` (SQLite) with canonical JSON at `entries/YYYY-MM-DD/<uuid>.json`.

Phase 2 shipped **JSONL index** at `index/entry_index.jsonl` because `esp32-idf-sqlite3` could not be added cleanly via the ESP-IDF component manager in that environment.

Phase 3 re-evaluated whether to swap JSONL → SQLite before hardware validation.

---

## Options compared

| Criterion | JSON + JSONL (current) | JSON + SQLite | Binary/compact index |
|-----------|------------------------|---------------|----------------------|
| **FAT32 reliability** | Good — append + atomic rewrite | Moderate — journal/WAL on SD can corrupt on power loss | Good if append-only |
| **Dependency risk** | Low (cJSON only) | High — third-party sqlite component, build size | Low — custom format |
| **RAM usage** | Low per operation | Higher — SQLite page cache | Low |
| **Flash size** | Smallest delta | +100–300 KB typical | Small |
| **Rebuild speed** | O(n) scan entries tree | O(n) scan + INSERT | O(n) scan |
| **Corruption recovery** | Delete index file, rebuild from JSON | `REINDEX` or delete DB, rebuild | Delete file, rebuild |
| **Code complexity** | Simple line I/O | SQL schema, migrations, error paths | Custom parser maintenance |
| **Standalone maintainability** | **Best** — human-readable index | Good queries, opaque file | Poor debuggability |

---

## Phase 3 investigation

1. **Build test:** CIRCE Phase 3 builds successfully with JSONL only (ESP-IDF v5.2, esp32s3).
2. **Component manager:** `esp32-idf-sqlite3` was not integrated in Phase 3 to avoid destabilizing the MVP before hardware validation.
3. **Hardware test:** Blocked — no SenseCAP Watcher connected in dev environment. SQLite FAT32 behavior on Watcher SD remains unverified.

---

## Decision

**Keep JSON canonical + JSONL index for Phase 3.**

Rationale (standalone-first):

- JSON files remain source of truth.
- JSONL index is rebuildable via `circe_rebuild_index_from_json()`.
- No new dependency or flash/RAM cost before hardware proves the core flow.
- Human-readable index aids field debugging without a host tool.

SQLite remains a **Phase 4+ candidate** if:

- Entry count grows enough that JSONL scan latency matters.
- Hardware validates stable sqlite3-on-FAT32 with journal disabled or temp store on heap.
- Component is pinned and approved in `idf_component.yml`.

---

## Migration path (future)

If SQLite is adopted later:

1. Open `index/circe.db` on SD after mount.
2. Schema: `entries(id, local_date, created_at, json_path, lifecycle_state)`.
3. `circe_rebuild_index_from_json()` populates from JSON tree (same as today).
4. Keep JSONL write path behind `#ifdef CIRCE_USE_SQLITE` during transition, or one-shot migrate on first boot.

---

## API stability

Storage public API unchanged — index backend is opaque to UI and conversation engine. Swap is localized to `circe_index.c`.
