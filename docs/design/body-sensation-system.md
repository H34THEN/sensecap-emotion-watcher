# Body Sensation System

## Design priority

Body-first emotional exploration is a **primary design requirement**, not an optional add-on.

The system must support:

> "I don't know the emotion yet, but my body feels like this."

without requiring the user to choose an emotion first.

---

## Body areas

Fixed taxonomy (Phase 1). User selects one or more areas per entry.

| ID | Label | Notes |
|----|-------|-------|
| `head` | Head | Includes scalp tension |
| `eyes` | Eyes | Pressure, dryness, unfocused |
| `jaw` | Jaw | Clenching, soreness |
| `throat` | Throat | Lump, tightness |
| `chest` | Chest | Weight, warmth |
| `stomach` | Stomach | Nausea, butterflies |
| `shoulders` | Shoulders | Raised, tense |
| `back` | Back | Knots, ache |
| `arms` | Arms | Heavy, weak |
| `hands` | Hands | Clenched, cold, tingling |
| `hips` | Hips | Pressure, restlessness |
| `legs` | Legs | Restless, weak |
| `feet` | Feet | Grounded, numb |
| `skin` | Skin | Crawling, sensitive |
| `whole_body` | Whole body | Diffuse, everywhere |

### UI concept

- **Body map screen**: simplified front silhouette; tap regions (touch) or cycle regions (encoder).
- **List fallback**: accessible list of areas for precise encoder navigation.
- Selecting `whole_body` may dim other areas (mutually exclusive option — **Phase 2 UX decision**).

---

## Sensation tags

Multi-select per entry. Grouped for UI scrolling on small display.

### Physical quality

| Tag | Label |
|-----|-------|
| `tight` | Tight |
| `pressure` | Pressure |
| `heavy` | Heavy |
| `shaky` | Shaky |
| `buzzing` | Buzzing |
| `tingling` | Tingling |
| `hot` | Hot |
| `cold` | Cold |
| `numb` | Numb |
| `floaty` | Floaty |
| `nauseous` | Nauseous |

### Autonomic / breath

| Tag | Label |
|-----|-------|
| `racing_heart` | Racing heart |
| `shallow_breathing` | Shallow breathing |

### Sensory / overload

| Tag | Label |
|-----|-------|
| `skin_crawling` | Skin crawling |
| `overstimulated` | Overstimulated |

### Autistic-specific (user-requested)

| Tag | Label | UI note |
|-----|-------|---------|
| `shutdown_feeling` | Shutdown feeling | No alarm styling |
| `meltdown_warning` | Meltdown warning | Calm color; optional private auto-lock suggestion |
| `pain_spike` | Pain spike | — |

---

## Data model

```json
{
  "body_areas": ["chest", "throat"],
  "body_sensations": ["tight", "pressure", "shallow_breathing"]
}
```

- **Multiple areas** and **multiple sensations** per entry — required.
- Optional future field: `body_area_sensation_pairs[]` for area-specific tagging (Phase 3).

---

## Interaction flows

### Body-first entry

1. Circe: "Would it be easier to start with your body?"
2. User selects areas on map.
3. User selects sensation tags (scrollable chips).
4. Optional: add emotion later or skip.

### Emotion-first entry

1. User picks emotion (or skips).
2. Circe: "Where do you notice that in your body?"
3. Same area + sensation UI.

---

## Pattern reflection hooks

`pattern_reflection_engine` may surface:

- Frequent co-occurrence: `overstimulated` + `shutdown_feeling`
- Time-of-day clusters for `meltdown_warning`
- Correlation between `poor sleep rating` and `heavy` + `whole_body`

No diagnostic language in reflections — observational only:

> "This week, 'tight' in your shoulders appeared on 4 of 7 days."

---

## Calibration mode

User can:

- Hide infrequently used sensation tags
- Reorder tag groups
- Enable "quick tags" favorites

Stored in NVS / settings file — not per-entry.

---

## Related modules

- [body_sensation_tags.md](../modules/body_sensation_tags.md)
- [circe_assistant.md](../modules/circe_assistant.md)
