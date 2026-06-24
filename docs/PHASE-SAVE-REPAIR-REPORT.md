# Phase Save-Repair Report

**Date:** 2026-06-24  
**Hardware:** `/dev/ttyACM1`  
**Scope:** Fix save (JSON-first), center subscreens, Test Save diagnostic  
**Not done:** Voice, camera, ML, cloud, HUD polish, new features

---

## 1. Root cause of continued save failure

Index append failure after successful JSON write caused the UI to report failure. Review depended on index, so users could not see entries that **were** on SD.

## 2. Was JSON write failing?

**Uncertain on user hardware without captured failure log.** Code path analysis: JSON write likely succeeded in many reported failures; index step failed afterward. New logging prints `json=OK/FAIL` and target path per attempt.

## 3. Was index failing?

**Most likely yes** for user-visible "can't save" cases. Index append can fail (SD busy, rewrite error, missing dir) while JSON exists. Now non-fatal.

## 4. JSON-first change

- `circe_save_entry()` returns `CIRCE_SAVE_OK_INDEX_WARN` when JSON OK, index fail  
- UI treats that as **success** with subline *Saved. Index will rebuild.*  
- `circe_index_mark_dirty()` persists `.dirty` marker on SD  
- Boot calls `circe_storage_rebuild_index_if_dirty()`

## 5. Test Save

Added **More → Storage → Test Save**. Reports `JSON OK  INDEX OK/WARN  LOAD OK  DEL OK`.  
**Hardware confirmation pending.**

## 6–10. Flow validation (hardware)

| # | Flow | Status |
|---|------|--------|
| 1 | Test Save diagnostic | **Pending user** |
| 2 | Quick save | **Pending user** |
| 3 | Body save without color | **Pending user** |
| 4 | Body save with Green | **Pending user** |
| 5 | Body save with Purple | **Pending user** |
| 6 | Review after save | **Pending user** (fallback scan implemented) |
| 7 | Delete latest | **Pending user** |
| 8 | Rebuild index | Code path preserved |
| 9 | Review after rebuild | **Pending user** |
| 10 | Reboot → Review | **Pending user** |

Boot verified: SD mounted, storage ready (serial).

## 11. Pages centered

All functional subscreens — see [docs/ui/CENTERED_SUBSCREEN_PASS.md](ui/CENTERED_SUBSCREEN_PASS.md).

## 12. Build

**PASS** — 684,288 B (`0xa7100`)

## 13. Flash

**PASS** — `idf.py --port /dev/ttyACM1 -b 2000000 app-flash`  
Partitions / Himax / NVS untouched.

## 14. Hardware validation

**Incomplete** — agent cannot run touch flows. User must confirm save + centering on Watcher.

## 15. Remaining bugs

1. Touch flow confirmation required before calling save fixed  
2. Index append root cause on SD not fully diagnosed (may still warn often)  
3. Subscreen density still high (functional, not polished)  
4. Strand view still basic  

## 16. Commit recommendation

**Do not commit** until user confirms Test Save + body+color save + Review on hardware.

Suggested message when confirmed:

```
Treat JSON as save source of truth; index best-effort with rebuild.

Center all subscreen UI in 240px column. Add Test Save diagnostic.
```

---

## Next recommended prompt

> On Watcher: More → Storage → Test Save — confirm JSON OK LOAD OK DEL OK. Then Home → Ready → body → sensation → intensity → Green → save. Confirm "Saved" (not error). Review must show entry. Confirm subscreens look centered. If all pass, commit Save-Repair phase.

**Phase stopped.**
