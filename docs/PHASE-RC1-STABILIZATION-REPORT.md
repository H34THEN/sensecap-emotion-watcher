# Phase Report — RC1 Stabilization

**Date:** 2026-06-24  
**Branch:** main  
**Roadmap:** Standalone MVP RC1 (stabilization, not a new feature phase)

---

## Summary

Fixed REVIEW → TODAY browse display bug (browser lifecycle). Added UI file map and RC1 release documentation. Build and app-flash pass; serial boot validation OK.

---

## REVIEW → TODAY bug

**Root cause:** `circe_memory_browser_begin()` ran in worker completion before `go_step()`, then `clear_content()` destroyed browser state before `MEMORY_BROWSE` refresh.

**Fix:** Initialize memory/pattern/body browsers in `circe_ui_show_step()` after `clear_content()`.

**Files:** `firmware/circe/main/circe_ui.c`

**Doc:** `docs/bugs/REVIEW_TODAY_DISPLAY_BUG.md`

---

## Documentation added

| File | Purpose |
|------|---------|
| `docs/ui/UI_FILE_MAP.md` | Screen → source file map for manual visual editing |
| `docs/releases/CIRCE_STANDALONE_MVP_RC1.md` | RC1 feature list, validation, flash instructions |
| `docs/bugs/REVIEW_TODAY_DISPLAY_BUG.md` | Bug postmortem |

---

## Build / flash / validation

**Build:** PASS — `circe.bin` `0xC79A0` (817568 bytes)  
**Flash:** PASS — app-flash `/dev/ttyACM1`  
**Boot (serial):** PASS — home wheel, worker, daily/timeline load, no panic  
**Worker stack:** ~2524 words free after daily load  
**Manual UI checklist:** Not fully automated — see RC1 release doc

---

## Git

**Commits:** `7fb568a` (fix), (release doc commit pending)  
**Tag:** `circe-standalone-mvp-rc1`
