# Magic Mirror — Future Integration Plan

**Status:** Planning only (Phase 4+). No implementation in Phase 1–2.

---

## Vision

Display Circe emotional data as ambient household visualization — mood strands, color timelines, and reflection cards — without exposing private entries by default.

---

## Potential features

| Feature | Description |
|---------|-------------|
| Daily mood strands | Horizontal color band for today's entries |
| Weekly mood summaries | Aggregated strand + entry count |
| Color timelines | Month view (blanket/crochet metaphor) |
| Pattern visualizations | Charts from pattern_reflection_engine exports |
| Circe reflection cards | Short observational text cards |
| Voice interactions | Mirror mic → LAN Hades → spoken Circe response |
| Speaker output | TTS playback on mirror or linked speakers |

---

## Architecture (proposed)

```
SenseCAP Watcher                    LAN
       │                              │
       │  sync_queue (opt-in)         │
       └──────────► Magic Mirror ◄───┘
                      Module: MMM-Circe
                           │
                     reads non-private
                     entry summaries
```

---

## Magic Mirror module: `MMM-Circe` (future)

- Node.js module consuming local HTTP/MQTT API
- Config: `circeEndpoint`, `refreshInterval`, `showPrivate: false`
- Renders strand CSS/SVG from color records

---

## Data contract (draft)

```json
GET /api/strand?from=2026-06-24&to=2026-06-24

{
  "segments": [
    { "at": "2026-06-24T08:00:00Z", "color": "#68D391", "intensity": 4 },
    { "at": "2026-06-24T19:00:00Z", "color": "#4A5568", "intensity": 8 }
  ]
}
```

Private entries **never** appear in API responses unless household policy explicitly enabled (not default).

---

## Privacy requirements

- TLS on LAN preferred
- API token in Mirror config (not on Watcher UI)
- No cloud relay
- Sync is one-way push from Watcher unless delete propagation added

---

## Dependencies

- sync_queue module on Watcher
- Local bridge service OR direct Mirror polling of Watcher HTTP server
- Non-private entries only

---

## Open questions

1. Does Watcher host HTTP server or push to Mirror?
2. Photo thumbnails on Mirror — separate consent?
3. Multi-user household — single Watcher assumption for v1?

See [sync_queue.md](../modules/sync_queue.md).
