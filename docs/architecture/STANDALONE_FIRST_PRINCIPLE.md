# CIRCE Standalone-First Principle

CIRCE is a **standalone emotional regulation and reflection device**. It must remain fully useful without any network service, mirror, GPU cluster, or cloud platform.

## CIRCE Core (required)

These features define the product:

- Body-first emotional logging
- Quick entry for overload moments
- Scripted Circe conversation (warm, non-diagnostic)
- Local private storage on the Watcher microSD
- Entry review and hard delete
- Storage health and index rebuild
- Future on-device pattern reflection (local only)

A user with only a SenseCAP Watcher and microSD card should be able to check in, save privately, and review history **forever** without installing anything else.

## Optional future plugins

These are **not** dependencies of the core experience:

| Plugin | Purpose |
|--------|---------|
| Magic Mirror | Household visualization of **non-private** aggregates |
| Hades Watch | LAN GPU inference and embeddings (opt-in export) |
| GPU training | User-initiated dataset export only |
| NAS backup | Encrypted backup copies |
| Voice services | TTS/STT when user enables |
| Mobile apps | Remote admin (if ever built) |

Plugins may enhance CIRCE. They must not be required to create, save, or review entries.

## Engineering rules

1. Core firmware builds and runs with Wi-Fi disabled.
2. Default privacy: `private_locked=true`, `training_ok=false`.
3. No SenseCraft journaling path in Circe firmware.
4. Sync/export code is gated behind explicit user actions (future phases).
5. Architecture docs describe integrations as **optional**, never as primary flows.

## Phase 2 scope alignment

Phase 2 implements **CIRCE Core** only: body-only entry, quick entry, hybrid storage, review, delete, diagnostics.

See [firmware/circe/README.md](../../firmware/circe/README.md).
