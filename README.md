# CIRCE — SenseCAP Watcher Emotional Reflection Companion

CIRCE is a personal emotional reflection companion for the [Seeed Studio SenseCAP Watcher](https://wiki.seeedstudio.com/watcher_hardware_overview/). It helps the user explore emotions through body sensations, color association, journaling, and pattern reflection over time.

**CIRCE is not a therapist, diagnosis system, or mental health professional.**

## Project status

**Phase 1 (current):** Research, architecture, and documentation. No firmware implementation yet.

## Design principles

- **Body-first exploration** — The user can start with body sensations without naming an emotion first.
- **Privacy by default** — `training_ok = false`, `private_locked = true`. No cloud upload, no remote AI, no automatic sharing.
- **Local-first data collection** — The Watcher is primarily a structured data capture device; ML and visualization happen locally or on LAN services later.
- **Reflective, not prescriptive** — Circe asks gentle questions; she never diagnoses or claims certainty.

## Repository layout

```
docs/
  architecture/     System design and module map
  hardware/         SenseCAP Watcher hardware facts (official sources)
  sdk/              Firmware SDK and ESP-IDF research notes
  personality/      Circe assistant tone and boundaries
  design/           User flow, body sensations, colors, privacy
  schema/           Entry data model planning
  modules/          Per-module specifications
  integration/      Magic Mirror and Hades Watch future plans
  ml/               Training dataset design (not implemented)
  roadmap/          Phase 2+ implementation plan
firmware/           Future Watcher firmware (placeholder)
schemas/            JSON schema drafts
```

## Official Watcher resources

| Resource | URL |
|----------|-----|
| Hardware overview | https://wiki.seeedstudio.com/watcher_hardware_overview/ |
| Development environment | https://wiki.seeedstudio.com/build_watcher_development_environment/ |
| Software framework | https://wiki.seeedstudio.com/watcher_software_framework/ |
| UI integration | https://wiki.seeedstudio.com/watcher_ui_integration_guide/ |
| Function module guide | https://wiki.seeedstudio.com/watcher_function_module_development_guide/ |
| Firmware SDK | https://github.com/Seeed-Studio/SenseCAP-Watcher-Firmware |
| Open hardware | https://github.com/Seeed-Studio/OSHW-SenseCAP-Watcher |

## Primary user flow (summary)

New Entry → Conversation → Emotion Picker → Color → Hex → Intensity → Body Sensations → Sleep/Energy/Stress → Context Tags → Summary → Photo → Review → Training Consent → Privacy Lock → Save

See [docs/design/user-flow.md](docs/design/user-flow.md) for the full flow including body-first alternate paths.

## License

Documentation and planning artifacts: TBD. Firmware will inherit SenseCAP SDK licensing when integrated.
