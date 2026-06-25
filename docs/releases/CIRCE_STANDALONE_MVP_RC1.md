# CIRCE Standalone MVP — Release Candidate 1

**Tag:** `circe-standalone-mvp-rc1`  
**UI polish tag:** `circe-standalone-mvp-rc1-ui-polish` (commit `63b794c`)  
**Hardware-signed tag:** `circe-standalone-mvp-rc1-hardware-signed` (firmware `ff2c0c8`)  
**Daily trial guide:** `docs/releases/CIRCE_DAILY_TRIAL_GUIDE.md`  
**Firmware path:** `firmware/circe`  
**Reference:** `docs/CIRCE_future_functionality_roadmap_reference.md`

---

## Feature set (standalone MVP)

- Body-first check-in (area → sensation → intensity)
- Emotional tone + mood color picker (memory-safe canvas field)
- Local color trait derivation
- Save / review / delete with `.JSN` storage under `/sdcard/CIRCE`
- Manual time and date-based folders
- Neon Terminal theme + theme persistence (NVS)
- Home slot-wheel navigation
- Reflection engine + recent pattern reflection after save
- Memory timeline (TODAY / YESTERDAY / THIS WEEK / ALL ENTRIES)
- Pattern Recognition + Body Heat Map (text/bar summary)
- Regulation library (breathing, body anchor, 5-4-3-2-1, sensory reset, bilateral tap)
- Optional local voice cues (soft tones)
- Camera memory scaffold (capture blocked — safe unavailable fallback)
- Daily companion home feed
- Diagnostics (probe, TEST SAVE, index rebuild)

---

## Known bugs (RC1)

| Issue | Status |
|-------|--------|
| REVIEW → TODAY browse empty while logs show items | **Fixed** — see `docs/bugs/REVIEW_TODAY_DISPLAY_BUG.md` |
| Status banner stuck after load/save | **Fixed** — dismiss on screen transition + worker completion (2026-06-26) |
| Camera capture | **Blocked** — SSCMA/Himax integration deferred |
| Voice cues full manual validation | **Gap** — implemented; user testing may be pending |

---

## Hardware limitations

- SenseCAP Watcher circular display — fixed terminal feed, no scroll lists
- SD card required for memory; FAT-safe `.JSN` extension only
- Camera not functional in firmware yet
- No cloud, STT, ML, or Strand visualization

---

## Safe flash instructions

App-flash only from `firmware/circe`:

```bash
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

**Do not:** erase flash, full flash, modify partitions, flash Himax, touch `nvsfactory`.

Preserves `/sdcard/CIRCE` entries and factory partitions.

---

## How to reach every major screen (post UI polish)

```text
HOME
  rotate to BODY CHECK-IN → press
  rotate to REVIEW → press
  rotate to REGULATE → press
  rotate to SETTINGS → press
  rotate to DIAGNOSTICS → press

REVIEW
  rotate to TODAY → press
  rotate to PATTERNS → press
  rotate to BODY MAP → press

REGULATE
  rotate to BREATHING → press
  rotate to BILATERAL TAP → press

SETTINGS
  rotate to VOICE CUES → press
```

See `docs/ui/RC1_VISUAL_POLISH_PASS.md` for selector behavior, status banner, and triple-press Home.

**UI polish tag:** `circe-standalone-mvp-rc1-ui-polish` (commit `63b794c`).

**Hardware sign-off:** `docs/releases/CIRCE_RC1_HARDWARE_SIGNOFF.md` · **Screen captures:** `docs/ui/SCREEN_CAPTURE_GUIDE.md` · **Daily trial:** `docs/releases/CIRCE_DAILY_TRIAL_GUIDE.md`

**Approved for daily trial use:** YES — see `docs/releases/CIRCE_RC1_HARDWARE_SIGNOFF.md`

---

## Validation checklist (RC1)

Legend: ✅ serial-verified · ⚠️ manual required · ❌ known issue

### Boot
| Item | Status |
|------|--------|
| Device boots | ✅ (2026-06-26 sign-off flash) |
| Home appears | ⚠️ serial: home wheel 6 options |
| Neon Terminal readable | ⚠️ user visual |
| SD mounts | ✅ (storage probe logs) |
| Worker starts | ✅ |
| No panic / boot loop / LVGL overflow | ✅ (serial) |

### Home
| Item | Status |
|------|--------|
| Slot-wheel / rotate / press | ⚠️ |
| Long press → Settings | ⚠️ |
| Daily companion feed | ✅ (worker log: daily summary) |

### Body check-in / save / review / delete
| Item | Status |
|------|--------|
| Full flow | ⚠️ manual |

### Memory / Review
| Item | Status |
|------|--------|
| TODAY shows entries | ⚠️ serial: 2 today items — confirm on display |
| Other categories / PATTERNS / BODY MAP | ⚠️ |
| Entry detail / delete | ⚠️ |

### Regulation / Voice / Camera / Diagnostics
| Item | Status |
|------|--------|
| Regulation flows | ⚠️ |
| Voice settings | ⚠️ |
| Photo scaffold fallback | ⚠️ |
| TEST SAVE (JSON/INDEX/LOAD/DEL) | ⚠️ |

---

## Intentionally deferred (post-RC1)

- SSCMA camera capture
- Body silhouette heat map
- Magic Mirror / Hades Watch / cloud / ML / STT
- Strand visualization (disabled)
- Full timeline architecture rewrite

---

## UI editing

See **`docs/ui/UI_FILE_MAP.md`** for which source files control each screen.

**Post-RC1 visual infrastructure (not a new signed release):**

- **`firmware/circe/main/circe_ui_tokens.h`** — centralized layout tuning tokens
- **`docs/circe_bg_home.png`** → embedded Home HUD background (`assets/circe_homepage_bg.c`)
- **`docs/ui/MANUAL_UI_EDITING_WORKFLOW.md`** — safe edit / build / app-flash workflow
- **`docs/ui/HOME_STATIC_BACKGROUND_MVP.md`** — background asset details

---

## Post-RC1 roadmap (recommended)

1. Camera capture integration (when SPI/SD coordination ready)
2. Body heat map silhouette (optional)
3. REVIEW browse polish (truncation hints, week filters)
4. Conversation engine deepening
5. Magic Mirror (LAN) — separate phase per roadmap
