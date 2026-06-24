# Hades Watch — Future Integration Plan

**Status:** Planning only (Phase 4+). No implementation in Phase 1–2.

---

## Vision

Hades Watch provides local GPU inference, embedding generation, and long-term memory — keeping ML off the Watcher and off the cloud.

---

## Potential features

| Feature | Description |
|---------|-------------|
| Local GPU inference | Emotion/body pattern classifiers trained on user exports |
| Embedding generation | Vector index of entries for similarity search |
| Pattern recognition | Detect recurring body-emotion-color clusters |
| Voice model hosting | Local TTS/STT for Circe voice |
| Personalized emotional recognition | Suggest tags (user confirms — never auto-apply) |
| Long-term memory systems | Summaries across months with user consent |

---

## Architecture (proposed)

```
Watcher (collect)          Hades Watch (compute)
     │                              │
     │  export_dataset (manual)     │
     └──────────► JSONL + photos ───► Training pipeline
                                           │
                                           ▼
                                    Models + embeddings
                                           │
     ◄──────── LAN API (suggestions) ─────┘
     (never auto-write to journal)
```

---

## Data flow principles

1. Watcher remains **source of truth** for raw entries.
2. Hades receives **explicit exports** only (`training_ok && !private_locked`).
3. Hades suggestions return to Watcher as **proposals** — user accepts or ignores.
4. No remote/cloud inference path.

---

## Training pipeline (future)

1. User triggers export on Watcher or copies SD bundle.
2. Hades ingests with schema_version validation.
3. GPU jobs: photo analysis, multimodal correlation, voice correlation (if audio added later).
4. Artifacts stored on Hades storage — not pushed back automatically.

---

## Inference API (draft)

```
POST /circe/suggest
{
  "body_sensations": ["tight", "overstimulated"],
  "context_tags": ["noise"]
}

Response:
{
  "suggested_colors": ["#4A5568"],
  "suggested_context": [],
  "confidence": "low",
  "disclaimer": "Suggestion only — not a diagnosis"
}
```

---

## Voice integration

- Watcher `record` / `vi_ctrl` paths capture audio locally
- Optional upload to Hades **only with consent**
- Hades runs local whisper-class STT + local LLM with Circe system prompt

---

## Memory system

- Long-term summaries stored on Hades
- Referenced only when user asks "what patterns do you notice?"
- Private entries never embedded

---

## Open questions

1. Hades hardware/GPU stack specifics?
2. Real-time vs batch inference expectations?
3. Embedding store technology (sqlite-vec, faiss, etc.)?
4. Model update delivery back to Watcher — likely **no** on-device model in v1

See [training-dataset-design.md](../ml/training-dataset-design.md).
