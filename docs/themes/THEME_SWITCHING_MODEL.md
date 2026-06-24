# CIRCE Theme Switching Model

How users select, preview, apply, and persist themes — **local only**.

---

## User flow

```
Home → More → Appearance
  → scroll / encoder through theme list
  → tap or press to preview (optional inline swatch)
  → Apply (or instant apply on select — design choice)
  → Back → Home (theme active)
```

**Maximum primary actions on Appearance:** 4 visible + scroll list  
(List items are not primary actions — selection, not navigation tree)

---

## Apply semantics

| Mode | Behavior | Recommendation |
|------|----------|----------------|
| **Instant apply** | Selecting theme immediately updates UI | Simpler; 2 taps |
| **Preview + Apply** | Preview temporary until Apply | Safer for photosensitive users |

**Phase 5 recommendation:** **Instant apply** with 150 ms fade — user can switch back quickly.

No "Are you sure?" dialog.

---

## Persistence

| Store | Key | Type | Default |
|-------|-----|------|---------|
| NVS `circe_ui` | `theme_id` | uint8 | `0` (Classic) |
| NVS `circe_ui` | `theme_ver` | uint8 | `1` |

Load order on boot:

```
1. nvs_open("circe_ui")
2. read theme_id — if invalid or missing → Classic
3. circe_theme_apply(theme_id)
4. before first LVGL screen draw
```

Theme survives reboot. Theme does **not** sync to entries JSON.

---

## Migration

When new themes added in firmware update:

```c
if (saved_id >= CIRCE_THEME_COUNT) {
    saved_id = CIRCE_THEME_CLASSIC;
}
```

No migration of entry data required.

---

## Encoder behavior on Appearance

```
Focus order:
  Theme list (scrollable, 10 items)
  Back

Rotate → move selection highlight
Press  → apply theme + brief haptic-free confirmation label "Applied."
Long press → no action
```

Focus style must be visible in **all** themes including High Visibility.

---

## Privacy & standalone

| Property | Value |
|----------|-------|
| Network | None |
| Entry data | Unchanged |
| Export | Theme id not included in entry JSON (UI preference only) |
| training_ok | Unaffected |

Optional `_extensions.ui_theme` in entry export **not recommended** — theme is device preference, not emotional record.

---

## Failure modes

| Failure | Fallback |
|---------|----------|
| NVS read fail | Classic |
| Unknown theme id | Classic |
| Apply crash | Classic hardcoded in ROM |

---

## Copy (pattern keys — Phase 5)

| Key | Text |
|-----|------|
| `appearance.title` | Appearance |
| `appearance.theme_prompt` | Choose a calm space. |
| `appearance.applied` | Theme updated. |

---

## Related

- [THEME_ARCHITECTURE.md](THEME_ARCHITECTURE.md)
- [THEME_COLOR_PALETTES.md](THEME_COLOR_PALETTES.md)
