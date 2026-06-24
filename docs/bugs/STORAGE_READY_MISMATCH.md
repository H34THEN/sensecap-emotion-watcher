# Bug: STORAGE_NOT_READY Despite Boot “Storage Ready”

**Date:** 2026-06-24  
**Phase:** Storage-Repair  
**Symptom:** User sees `Save failed: STORAGE_NOT_READY` while serial boot shows `storage ready at /sdcard/circe`

---

## Root cause

**State mismatch between boot init and save-time readiness check.**

| Layer | Old behavior |
|-------|----------------|
| Boot `circe_storage_init()` | Set `s_ready=true` after `mkdir` succeeded |
| Save `circe_storage_is_ready()` | Called `access("/sdcard", W_OK)` via health check |

On ESP-IDF FAT/VFS, **`access("/sdcard", W_OK)` often returns failure** even when the SD card is mounted and file writes succeed. Boot never used this check; save did — so init logged “storage ready” while every save reported `STORAGE_NOT_READY`.

This was **not** a missing SD card. It was a **false-negative writable probe**.

---

## Secondary issue: path case

User PC shows `CIRCE/INDEX`, `CIRCE/ENTRIES`. Firmware used `/sdcard/circe/entries`. On FAT these are typically the same directory (case-insensitive), but inconsistent paths caused confusion during diagnosis.

---

## Fix

1. **Removed `access(W_OK)` mount detection** — use `stat("/sdcard")` for mount presence  
2. **Added real write probe** — `LOGS/PROBE.TMP` write/read/delete at init (was `{base}/storage_probe.tmp`; see [STORAGE_PROBE_ERRNO_22.md](STORAGE_PROBE_ERRNO_22.md))  
3. **`circe_storage_is_ready()`** = `s_ready && s_probe_passed && sd_mount_exists()`  
4. **Path resolver** (`circe_storage_paths.c`) — detects existing `/sdcard/CIRCE` or `/sdcard/circe`, prefers existing tree; canonical subdirs `ENTRIES`, `INDEX`, `CACHE`, `LOGS`  
5. **Save uses `circe_storage_ensure_ready()`** — reinit + reprobe once before failing  
6. **Storage screen** shows Mounted / Path / Writable / Probe / dirs explicitly  

---

## Verification

After flash, Storage screen should show:

```
Mounted: YES
Storage Ready: YES
Probe: PASS
Writable: YES
Path: /sdcard/CIRCE  (or resolved variant)
```

Save serial log should show `storage_ready=yes probe=yes` — not `STORAGE_NOT_READY`.

---

## Follow-on: probe EINVAL (errno=22)

After Storage-Repair, hardware still failed init with `probe write open fail errno=22` on the old probe path `/sdcard/CIRCE/storage_probe.tmp`. Fixed in **Storage-Probe-EINVAL** phase: probe moved to `/sdcard/CIRCE/LOGS/PROBE.TMP` with staged logging. Serial boot now shows **probe PASS**. See [STORAGE_PROBE_ERRNO_22.md](STORAGE_PROBE_ERRNO_22.md).

After probe fix, **Test Save** rebooted due to storage I/O on the LVGL task (not a readiness false negative). Fixed in **LVGL-Worker-Task** phase — see [LVGL_STACK_OVERFLOW_STORAGE_IO.md](LVGL_STACK_OVERFLOW_STORAGE_IO.md).

---

## Files changed

- `circe_storage.c` — probe, ready semantics, health struct  
- `circe_storage_paths.c` — path normalization  
- `circe_save.c` — `ensure_ready()` instead of broken `access()` check  
- `circe_index.c`, `circe_strand_cache.c` — dynamic paths  
