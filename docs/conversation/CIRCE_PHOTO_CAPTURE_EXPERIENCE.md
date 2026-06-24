# Circe — Photo Capture Experience

When Circe offers a photo, how she asks, and how privacy/consent are communicated.

Related: [privacy-model.md](../design/privacy-model.md), [camera_capture.md](../modules/camera_capture.md)

---

## Design stance

**Photos are always optional. Never required.**

Photos are **memory anchors** — not evidence, not ML training input by default, not social sharing.

---

## When Circe asks for a photo

| Context | Ask? | Circe line |
|---------|------|------------|
| Full reflection entry | Soft offer | "Want a photo for this moment, or skip?" |
| Body-only quick entry | **No** (default) | — |
| Color-only quick entry | **No** (default) | — |
| Low-energy / shutdown mode | **Never** | — |
| Overstim tags selected | **Never** | — |
| User setting `photo_prompts = on` | Soft offer | Standard soft offer |
| User setting `photo_prompts = off` | Never auto | Photo in menu only |
| Photo entry mode | Yes (purposeful) | "This entry is for a photo. Ready?" |
| Re-open saved entry | No | Edit photo from review only |

---

## When Circe does not ask

- First launch before user completes one save (avoid pressure).
- Any **fast-entry** or **quiet** mode.
- When `private_locked` discussion would feel intrusive after distress tags — photo step hidden entirely.
- When camera hardware unavailable — silent skip, no apology loop.

---

## Optional vs required

| Rule | Detail |
|------|--------|
| Required | Never |
| Default action | **Skip** (equal visual weight to Take photo) |
| Skip outcome | `photo_path: null` — valid complete entry |
| Retake | Only from review; Circe: "Take a new one, or keep this?" |

---

## Privacy language

### Before camera opens

**Circe:** "Photos stay on your Watcher's memory card. Private by default."

Subtext: "Not uploaded. Not analyzed automatically."

### After capture

**Circe:** "Saved with your entry — only here unless you change privacy."

### If user turns privacy off ( rare )

**Circe:** "Privacy is off for this entry — photo could sync if you enable LAN sync later."

Do not block capture — inform only.

---

## Consent language

Photos inherit entry-level consent:

| Toggle | Photo implication |
|--------|-------------------|
| `private_locked = true` (default) | Photo never syncs, never exports |
| `training_ok = false` (default) | Photo excluded from export bundles |
| Both must be false/off for export | Explicit export dialog lists photo count |

**Circe at save (if photo present):**

**Circe:** "Photo included. Still private, still not for training — unless you change those toggles."

Separate from training toggle — user sees both.

---

## Circe lines — photo flow

### Soft offer

**Circe:** "Some people attach a photo. Want to, or skip?"

Buttons: **Take photo** | **Skip**

### Camera active

**Circe:** "Hold still. Tap when ready." *(or hardware shutter UX)*

Minimal text — reduce on-screen clutter over live preview.

### After capture — preview

**Circe:** "Keep this one?"

Buttons: **Keep** | **Retake** | **Remove photo**

### Retake from review

**Circe:** "Replace the photo? Your other fields stay."

### Skip guilt prevention

**Circe:** "No photo — that's fine."

---

## Distress and body-sensitive entries

If `meltdown_warning`, `shutdown_feeling`, or `pain_spike`:

- **Do not** suggest photo ("capture this moment").
- Hide photo step unless user opens **Add photo** from review.

**Circe (if user adds manually):**

**Circe:** "Only if you want a reminder for yourself."

---

## Future ML / analysis

**Circe never says:**

- "I'll analyze your face."
- "This photo helps the AI understand you."

If user exports for training (explicit):

**Circe:** "Exports are your choice — only entries you allow."

---

## Voice mode (future)

**Circe:** "Photo optional. Say 'skip' or 'capture'."

---

## Settings (calibration)

| Setting | Default |
|---------|---------|
| `photo_prompts` | `soft` (soft \| off \| photo_mode_only) |
| `photo_in_quick_entry` | false |
| `photo_sync_allowed` | false (global gate even if privacy off) |

---

## Firmware implications

- Step graph: `photo_capture` node conditional on mode + tags + settings
- Camera permission = implicit (local device); no cloud consent dialog
- Preview screen: Circe copy max 1 line
- `photo_hash` optional integrity; no auto-upload hook
