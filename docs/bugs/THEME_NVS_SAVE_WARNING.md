# Theme NVS Save Warning

## Symptom

Serial monitor showed:

```
W circe_theme: failed to save theme_id
```

Theme preview/apply worked in RAM, but selection did not persist across reboot.

## Root cause

`circe_theme.c` used NVS namespace `circe_ui` / key `theme_id`, but **`nvs_flash_init()` was never called** in CIRCE `main.c`.

Without NVS init, `nvs_open()` returns `ESP_ERR_NVS_NOT_INITIALIZED` and save fails silently (previously logged only as "failed to save theme_id").

The same gap affected `circe_time` NVS backup (`circe_time` namespace) — time appeared to work when set in-session via `settimeofday()`, but manual time backup could not persist until NVS init was added.

## Fix

1. **`main.c`**: call `nvs_flash_init()` at boot (standard ESP-IDF pattern with `NO_FREE_PAGES` / `NEW_VERSION_FOUND` handling on the **user `nvs` partition only** — does not touch `nvsfactory`).
2. **`circe_theme.c`**: log `esp_err_to_name()` at each NVS step (`nvs_open`, `nvs_set_u8`, `nvs_commit`, load path).

## Safe behavior

- NVS failure does not crash or block the app.
- Theme remains active in RAM even if save fails.
- Warning includes exact ESP-IDF error name.

## Verification

1. Settings → Appearance → Fall Out of Time → Apply
2. Monitor: `theme saved to NVS: Fall Out of Time (4)`
3. Reboot → theme still Fall Out of Time

## Namespaces

| Namespace | Key(s) | Module |
|-----------|--------|--------|
| `circe_ui` | `theme_id` | `circe_theme.c` |
| `circe_time` | `manual_set`, `year`, … | `circe_time.c` |

Do **not** flash or erase `nvsfactory` (factory calibration).
