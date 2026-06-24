# Module: training_consent_toggle

**Layer:** UI  
**Phase:** 2

## Purpose

Per-entry opt-in for future ML dataset inclusion.

## Default

`training_ok = false`

## UI

- Clear toggle with explanation (not pre-checked)
- Link to privacy settings

## Enforcement

- export_dataset rejects if false
- Hades pipeline rejects if false

See [privacy-model.md](../design/privacy-model.md).
