# Phase Worker-Stack-Repair Report

**Date:** 2026-06-24  
**Hardware:** SenseCAP Watcher `/dev/ttyACM1`  
**Scope:** Move large storage buffers off worker stack; minimal self-test  
**Feature freeze:** maintained

---

## 1. Root cause

Worker task (12288 B) overflowed because save self-test nested calls still used **4096 B JSON buffers on the stack** in `save_json_atomic`, `entry_load`, scan, and index rewrite — often **two or more simultaneously** in one call chain.

## 2. Large stack allocations found

| File | Buffer | Size |
|------|--------|------|
| `circe_storage.c` | `json` in save/load/scan | 4096 |
| `circe_storage.c` | `json` in index_one_json_file | 2048 |
| `circe_index.c` | `line` in rewrite/scan/count/list | 640 |
| Self-test (old) | 2× `circe_entry_t` + nested paths | ~2 KB + nested JSON |

## 3. Buffers moved to heap

- All JSON read/write buffers → `circe_json_buf_alloc()` / `circe_json_buf_free()` (4096 B)
- All index line buffers → `circe_buf_alloc(CIRCE_INDEX_LINE_SIZE)` / `circe_buf_free()`
- New module: `circe_buf.c`, `circe_buf.h`

## 4. Worker stack high-water

Logged before/after every worker command:

```
worker stack before: N words free (~N*4 bytes)
heap free before: H
worker stack after: M words free
heap free after: H'
```

Values appear in serial when user runs Test Save or any worker command.

## 5–10. Hardware validation

| Test | Status |
|------|--------|
| Test Save (no reboot) | **Pending user tap** |
| Quick Save | **Pending user** |
| Body + Green Save | **Pending user** |
| Review / Delete | **Pending user** |
| Reboot persistence | **Pending user** |

Build flashed; agent cannot tap Watcher UI.

## 11. Build result

**PASS** — 695,472 B (`0xa9cb0`)

## 12. Flash result

**PASS** — `idf.py --port /dev/ttyACM1 -b 2000000 app-flash` @ `0x110000`

## 13. Remaining bugs

1. UI flow validation pending  
2. RTC unset → `19700101` date folders  
3. Strand screen still reads SD in `circe_ui_show_step()` (lower risk)  

## 14. Commit recommendation

**Do not commit** until Test Save shows `JSON OK LOAD OK DEL OK` without reboot.

Suggested message:

> `fix(storage): heap JSON/index buffers; minimal save self-test; worker stack logging`
