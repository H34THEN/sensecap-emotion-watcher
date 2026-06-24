# Primary User Flow

## Standard flow (product specification)

```
New Entry
    ↓
Conversation with Circe
    ↓
Emotion Picker
    ↓
Color Picker
    ↓
Hex Color Input
    ↓
Intensity Slider
    ↓
Body Sensation Tags
    ↓
Sleep Rating
    ↓
Energy Rating
    ↓
Stress Rating
    ↓
Context Tags
    ↓
Short Summary
    ↓
Photo Capture
    ↓
Review
    ↓
Retake Optional
    ↓
Training Consent
    ↓
Privacy Lock
    ↓
Save
```

---

## Body-first alternate flow (design requirement)

The user is autistic; body sensations are often easier to identify than emotions. **Body-first is a first-class path**, not an edge case.

```
New Entry
    ↓
Conversation with Circe
    ↓
"Would it be easier to start with your body?" 
    ↓ [Yes]
Body Sensation Tags  ←── FIRST substantive capture
    ↓
(Optional) Emotion Picker — skippable ("Not sure yet")
    ↓
Color Picker → Hex → Intensity
    ↓
Sleep / Energy / Stress
    ↓
Context Tags → Summary → Photo → Review → Consent → Privacy → Save
```

### Body-first rules

1. **Emotion picker may be skipped** — `emotion: null`, `emotion_skipped: true`.
2. **Body step cannot be auto-skipped** — user may tap "Nothing noticeable" (empty array), but not forced through emotion first.
3. **Circe copy** reinforces that unnamed emotions are valid.
4. **Review screen** shows body tags prominently when emotion is absent.

---

## Step details

| Step | Required | Skip behavior |
|------|----------|---------------|
| Conversation | No | Tap through or single "Ready" |
| Emotion picker | No | Skip / "Not sure" |
| Color | Soft | Default gray `#808080` if skipped |
| Hex input | No | Only if user wants precise color |
| Intensity | Soft | Default mid (5/10) |
| Body sensations | Soft | Empty array allowed |
| Sleep/Energy/Stress | Soft | Null if skipped |
| Context tags | No | Empty array |
| Summary | No | Empty string |
| Photo | No | Null photo_path |
| Review | Yes | Must confirm or go back |
| Training consent | Yes | Default OFF |
| Privacy lock | Yes | Default ON |
| Save | Yes | — |

---

## Navigation

- **Back** — Previous step; preserves draft.
- **Save privately** (shortcut from any step after body/emotion) — Saves with `completion_status: partial`.
- **Cancel** — Discards draft after confirmation.

---

## Encoder + touch

Each step must support:

- Touch tap targets ≥ recommended 44×44 logical px on 412×412 display
- Encoder: focus order follows visual top-to-bottom; wheel press = activate

---

## Time expectations

Target **2–5 minutes** for full flow; **30–60 seconds** for body-only quick capture.

---

## Related

- [Body sensation system](body-sensation-system.md)
- [Color system](color-system.md)
- [Circe personality](../personality/circe-personality.md)
