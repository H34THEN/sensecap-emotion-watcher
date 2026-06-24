# Mood Strand Visualization Study — Circular Display

**Phase 4:** design study only — **do not implement**.

The Watcher is a **412×412 circle**. Mood strands (daily color blocks per entry) need a visualization that respects safe area and emotional tone.

Reference: [MOOD_STRAND_SPECIFICATION.md](../memory/MOOD_STRAND_SPECIFICATION.md), [design/color-system.md](../design/color-system.md).

---

## Evaluation criteria

| Criterion | Weight |
|-----------|--------|
| Circle-first fit | High |
| Multiple entries per day | High |
| Cognitive load | High |
| Encoder navigability | Medium |
| Implementation cost on ESP32+LVGL | Medium |
| Emotional comfort | High |
| Standalone (no Mirror) | Required |

---

## Option A — Traditional rows (horizontal)

```
  Home top:  [■][■][■][□]
```

| Strengths | Weaknesses |
|-----------|------------|
| Already partially implemented | Horizontal row clips at left/right on round display |
| Matches blanket metaphor docs | Feels "rectangular UI on round face" |
| Easy LVGL flex row | Does not use circular space |
| Low CPU | Many entries wrap awkwardly |

**Circle fit:** Poor — corners of row enter danger zones.

**Verdict:** Accept as **MVP placeholder** only; replace for Watcher-native home.

---

## Option B — Radial rings (day arc)

```
        ╭───■─■─■───╮
       ╭┤           ├╮
      │ │   prompt  │ │
       ╰┤           ├╯
        ╰───────────╯
```

Blocks placed along **upper arc** of safe circle (y ≈ 12–48).

| Strengths | Weaknesses |
|-----------|------------|
| Native round aesthetic | Arc layout math non-trivial |
| Stays in safe zone | Max ~7–9 blocks visible without overlap |
| Calm, jewel-like | Harder to show intensity via stroke |
| Unique CIRCE identity | Encoder selection of individual blocks unclear |

**Verdict:** **Recommended for Watcher home strand** (Phase 5–6).

---

## Option C — Spiral memory rings

Multi-day spiral from center outward — each day a ring segment.

| Strengths | Weaknesses |
|-----------|------------|
| Beautiful metaphor (growth, time) | High cognitive load |
| Uses full circle | Overwhelming during distress |
| Rich storytelling | CPU/GPU cost for animation |
| | Hard to read precise times |

**Verdict:** Defer to **optional gallery screen**, not home. Not overload-friendly.

---

## Option D — Constellation visualization

Blocks as stars connected by faint lines (time order).

| Strengths | Weaknesses |
|-----------|------------|
| Poetic, night-sky mood | Implies pattern/narrative ("constellation says…") |
| Fits Moonlit Obsidian theme | Risk of feeling diagnostic |
| Circular naturally | Accessibility: low contrast links |
| | May trigger "meaning finding" — against Circe ethics |

**Verdict:** **Reject** for core UI — conflicts with non-diagnostic principle.

---

## Option E — Tree-ring model

Concentric rings; each entry adds a **segment** to today's ring (like tree growth).

```
     ╭─────────────╮
    ╭┤  ▓▓░░▓▓▓░░  ├╮   ← today ring (segments = entries)
   ╭┤   (yesterday)  ├╮
   ╰──────────────────╯
```

| Strengths | Weaknesses |
|-----------|------------|
| Strong circle-first fit | Multi-day view crowded on 412 px |
| Grounding metaphor (Forest theme) | Segment boundaries need legend |
| Shows multiple entries clearly | Intensity → arc thickness needs care |
| Encoder: rotate around ring | Implementation moderate |

**Verdict:** **Recommended for Today + Week mini view** in More or Review.

---

## Comparison matrix

| Option | Circle fit | Low load | Multi-entry | Build cost | Emotional safety |
|--------|------------|----------|-------------|------------|------------------|
| A Rows | ★★ | ★★★★ | ★★★ | ★★★★★ | ★★★★ |
| B Radial arc | ★★★★★ | ★★★★ | ★★★ | ★★★ | ★★★★ |
| C Spiral | ★★★★ | ★★ | ★★★★ | ★★ | ★★ |
| D Constellation | ★★★★ | ★★ | ★★★ | ★★★ | ★★ |
| E Tree-ring | ★★★★★ | ★★★ | ★★★★ | ★★★ | ★★★★ |

---

## Phase 5 recommendation

| Surface | Visualization |
|---------|-----------------|
| **Home (today)** | **Option B** — upper arc, max 8 blocks, void gap for empty |
| **Today detail** (More) | **Option E** — single day ring with segments |
| **Week** (future) | 7 micro-rings in circle grid |
| **Mirror export** (optional plugin) | Option A rows — rectangular display |

---

## Block rendering rules (all options)

- Color from `color_hex` only
- Opacity from intensity (MOOD_STRAND spec)
- No emoji overlays
- Missing color → `strand_void` (#2D3748 neutral)
- Never animate blocks on home boot (SD load deferred)

---

## Related

- [SAFE_AREA_SPEC.md](SAFE_AREA_SPEC.md)
- [CIRCLE_FIRST_DESIGN.md](CIRCLE_FIRST_DESIGN.md)
