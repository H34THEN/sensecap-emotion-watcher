# Voice Memory Model

**Future modality** — schema and policy designed now to avoid entry schema rewrites later.

Phase 2–3: no voice capture in MVP. Data structures reserved.

Related: [CIRCE_VOICE_DESIGN.md](../conversation/CIRCE_VOICE_DESIGN.md)

---

## Storage layout

```
/sdcard/circe/
  voice/
    <entry_id>.wav          # PCM or WAV, mono
    <entry_id>.meta.json
  voice/transcripts/
    <entry_id>.json
  voice/_trash/
```

Transcripts may live inside `EmotionEntry.assistant_transcript` for small clips; sidecar for long audio.

---

## Voice recording metadata

### `voice/<entry_id>.meta.json`

```json
{
  "entry_id": "uuid",
  "schema_version": "1.0.0",
  "format": "wav",
  "sample_rate": 16000,
  "channels": 1,
  "duration_ms": 4200,
  "size_bytes": 134400,
  "sha256": "...",
  "captured_at": "2026-06-24T14:32:00Z",
  "voice_consent": {
    "capture_ok": true,
    "transcription_ok": true,
    "export_ok": false,
    "embedding_ok": false,
    "sync_ok": false
  },
  "transcription_ref": "voice/transcripts/<entry_id>.json",
  "embedding_ref": null,
  "stt_engine": null,
  "language": "en"
}
```

---

## Transcriptions

### `voice/transcripts/<entry_id>.json`

```json
{
  "entry_id": "uuid",
  "schema_version": "1.0.0",
  "source": "hades_whisper | on_device",
  "text": "chest feels tight and buzzing",
  "segments": [
    { "start_ms": 0, "end_ms": 2100, "text": "chest feels tight" },
    { "start_ms": 2100, "end_ms": 4200, "text": "and buzzing" }
  ],
  "confidence": "medium",
  "user_verified": false,
  "mapped_tags": {
    "body_areas": ["chest"],
    "body_sensations": ["tight", "buzzing"]
  }
}
```

**User verification required** before tags auto-apply to entry (assistive only).

### In EmotionEntry

```json
"assistant_transcript": {
  "turns": [
    { "role": "circe", "pattern_key": "body.invite", "at": "..." },
    { "role": "user", "action": "selected_sensation", "value": "tight" }
  ],
  "voice": {
    "audio_path": "voice/uuid.wav",
    "transcript_path": "voice/transcripts/uuid.json",
    "verified": true
  }
}
```

---

## Voice embeddings (Hades)

```json
"embedding_ref": {
  "modality": "voice",
  "store": "hades",
  "id": "emb-voice-uuid",
  "model": "personal-speech-v1",
  "derived_from": "sha256:..."
}
```

Use cases (future):

- Similar tone days clustering
- Voice ↔ sensation correlation (consented export only)

**Never** stored on Watcher by default.

---

## Consent requirements

| Action | Required flags |
|--------|----------------|
| Record audio | User initiates hold-to-talk |
| Transcribe locally/LAN | `transcription_ok` |
| Include in export | `training_ok`, `!private_locked`, `export_ok` |
| Compute embedding | `embedding_ok` + export pipeline |
| Sync to Mirror | `sync_ok` (default false; audio rarely synced) |

Separate toggles at save — mirror photo model.

Default: **all voice consent false** except `capture_ok` when user completes recording.

---

## Retention rules

| Policy | Default |
|--------|---------|
| Audio tied to entry lifecycle | Yes |
| Delete entry | Delete audio + transcript |
| Transcript without audio | User may delete audio keep text |
| Max duration per clip | 60 s (configurable) |
| Max total voice storage | User warn at cap |
| Age purge | Off |

Low-energy / shutdown modes: **no voice capture**.

---

## Privacy

- Voice never cloud STT by default — Hades LAN only
- Transcripts are journal content — `private_locked` applies
- Export manifest lists audio file count separately
- Mirror: no audio playback default

---

## Deletion

Same as photo: trash → purge; embedding orphan cleanup on Hades when entry deleted and user requests Hades purge.

---

## Entry mode linkage

`entry_mode: voice` requires `voice/*.meta.json` OR transcript-only if user denies retention (transcript-only mode — user setting).

---

## Schema readiness

Fields added to entry schema as optional null objects — see [SCHEMA_ADDITIONS.md](SCHEMA_ADDITIONS.md).

No firmware until STT path validated on Hades.
