# Phase 4 Recommendations

Following Phase 3 standalone polish and blocked hardware validation.

---

## Priority 1 — Hardware validation

Flash `firmware/circe` on SenseCAP Watcher with microSD:

- Confirm SD mount, self-test, body/quick flows
- Reboot persistence
- Encoder usability
- Scroll on sensation list at arm's length

Update `docs/PHASE-3-HARDWARE-VALIDATION.md` with real results.

---

## Priority 2 — Storage

- Re-spike **SQLite** on device if entry count or query needs grow
- Optional: store `color_hex` in index row for faster Today strand (avoid N JSON reads)
- RTC timezone instead of hardcoded UTC

---

## Priority 3 — UI / UX

- Factory-style encoder page manager (`pm.c` pattern) if dial navigation feels awkward
- Mood blanket **multi-day** read-only view (Phase 3 only did Today strand)
- Optional NVS quick-preset favorites (user-chosen body/color shortcuts)

---

## Priority 4 — Data / schema

- Update `schemas/emotion-entry.schema.json` to v1.1.0
- Derived daily rollup JSON (not live storage) per MEMORY_ARCHITECTURE

---

## Still out of scope (standalone-first)

No Magic Mirror, Hades Watch, GPU export, cloud, voice, photos, ML, or pattern reflection in core firmware.

Optional plugins remain documented only.

---

## Suggested Phase 4 prompt

```markdown
# PHASE 4 — CIRCE On-Device Proof + Strand Expansion

Hardware required. Complete PHASE-3-HARDWARE-VALIDATION checklist.

1. Flash and record all validation results.
2. SQLite spike on device OR document JSONL permanent for MVP scale.
3. Multi-day mood strand read-only view (local only).
4. NVS favorite quick presets.
5. schema v1.1.0 JSON schema file update.

Standalone only. No integrations.
```
