# CIRCE Module Map

All modules are **documented** in Phase 1; implementation is Phase 2+.

## Module index

| Module | Layer | Phase | Doc |
|--------|-------|-------|-----|
| circe_assistant | App | 2 | [circe_assistant.md](../modules/circe_assistant.md) |
| emotion_tracker | App | 2 | [emotion_tracker.md](../modules/emotion_tracker.md) |
| color_picker | UI | 2 | [color_picker.md](../modules/color_picker.md) |
| hex_color_input | UI | 2 | [hex_color_input.md](../modules/hex_color_input.md) |
| intensity_slider | UI | 2 | [intensity_slider.md](../modules/intensity_slider.md) |
| body_sensation_tags | UI | 2 | [body_sensation_tags.md](../modules/body_sensation_tags.md) |
| sleep_energy_stress_ratings | UI | 2 | [sleep_energy_stress_ratings.md](../modules/sleep_energy_stress_ratings.md) |
| context_tags | UI | 2 | [context_tags.md](../modules/context_tags.md) |
| feeling_summary | UI | 2 | [feeling_summary.md](../modules/feeling_summary.md) |
| camera_capture | UI/HW | 2 | [camera_capture.md](../modules/camera_capture.md) |
| retake_photo | UI | 2 | [retake_photo.md](../modules/retake_photo.md) |
| training_consent_toggle | UI | 2 | [training_consent_toggle.md](../modules/training_consent_toggle.md) |
| privacy_lock | UI | 2 | [privacy_lock.md](../modules/privacy_lock.md) |
| delete_entry | App | 2 | [delete_entry.md](../modules/delete_entry.md) |
| entry_review | UI | 2 | [entry_review.md](../modules/entry_review.md) |
| local_storage | Data | 2 | [local_storage.md](../modules/local_storage.md) |
| sync_queue | Data | 3 | [sync_queue.md](../modules/sync_queue.md) |
| mood_strand_visualizer | UI | 3 | [mood_strand_visualizer.md](../modules/mood_strand_visualizer.md) |
| pattern_reflection_engine | App | 3 | [pattern_reflection_engine.md](../modules/pattern_reflection_engine.md) |
| export_dataset | Data | 4 | [export_dataset.md](../modules/export_dataset.md) |
| calibration_mode | App | 3 | [calibration_mode.md](../modules/calibration_mode.md) |

---

## Dependency graph

```
circe_assistant
    └── orchestrates all UI modules

emotion_tracker ──► local_storage ◄── all entry fields
                         │
                         ├── delete_entry
                         ├── privacy_lock (metadata filter)
                         ├── export_dataset (consent filter)
                         └── sync_queue (non-private only)

camera_capture ──► retake_photo ──► entry_review

color_picker ──► hex_color_input ──► mood_strand_visualizer
                      │
                      └── intensity_slider

body_sensation_tags ──► pattern_reflection_engine

sleep_energy_stress_ratings ──► pattern_reflection_engine
context_tags ──► pattern_reflection_engine

calibration_mode ──► color_picker, body_sensation_tags, intensity_slider
```

---

## Firmware directory mapping (proposed Phase 2)

```
firmware/circe/main/
├── circe_app/           # orchestrator, assistant, emotion_tracker
├── circe_ui/            # SquareLine exports + ui_manager
├── circe_storage/       # local_storage, sync_queue, export
├── circe_reflect/       # pattern_reflection_engine, mood_strand
└── circe_modules/       # individual UI screen controllers
```

Integrates with Watcher `view/` and BSP components per SDK research notes.
