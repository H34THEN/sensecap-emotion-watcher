# CIRCE Daily Trial Guide

Written for everyday use of **CIRCE Standalone MVP RC1** on the SenseCAP Watcher.

**Firmware tags:** `circe-standalone-mvp-rc1`, `circe-standalone-mvp-rc1-ui-polish`  
**Flash (app only):** from `firmware/circe`:

```bash
idf.py --port /dev/ttyACM1 -b 2000000 app-flash
```

---

## What this build is

CIRCE is a **local-first emotional regulation companion** on your Watcher. It helps you:

- Check in with your body and mood
- Save short reflections to the SD card (`/sdcard/CIRCE`)
- Review patterns and body-area summaries over time
- Use simple regulation tools (breathing, grounding, bilateral tap, etc.)

**What it is not:**

- Not medical care or diagnosis
- Not cloud-connected
- Not speech recognition
- Not automatic camera capture (photo memory is scaffolded; capture is unavailable)
- Not microphone recording (voice cues are optional **output-only** soft tones)

---

## How to navigate

| Gesture | Action |
|---------|--------|
| Rotate encoder | Change the selected menu item |
| Single press | Enter / select (short delay ~550 ms for double/triple detection) |
| Double press | Back |
| Triple press | Home |
| Long press | Settings (on some terminal screens) |

**Major paths:**

```text
HOME → BODY CHECK-IN
HOME → REVIEW → TODAY / PATTERNS / BODY MAP
HOME → REGULATE → BREATHING / BILATERAL TAP / …
HOME → SETTINGS → VOICE CUES / APPEARANCE / TIME
HOME → DIAGNOSTICS → TEST SAVE
```

See `docs/ui/SCREEN_CAPTURE_GUIDE.md` for a full screen list.

---

## Safe daily trial routine

**Morning**

- Open **HOME**
- Read the **Daily Companion** line when it appears
- Do one **BODY CHECK-IN** if it feels useful — not as a chore

**During the day**

- Use **REGULATE** when overwhelmed (Breathing, Sensory Reset, 5-4-3-2-1)
- Add entries when they help; skip when they do not

**Evening**

- **REVIEW → TODAY** — see what you logged
- **REVIEW → BODY MAP** — where sensation showed up recently
- **REVIEW → PATTERNS** — when you have enough entries for observations

---

## Status banners

Magenta centered banners show **SAVING**, **LOADING**, **DELETING**, and short success messages.

They should **disappear** when the operation finishes or after a brief timed message. If a banner stays stuck:

1. Triple-press **Home**
2. If still stuck, note the screen and path (see bug template below)
3. Run **DIAGNOSTICS → TEST SAVE**

---

## What to watch for

| Issue | What to do |
|-------|------------|
| Banner stuck on screen | Triple-press Home; report path + photo |
| Menu item hard to find | Rotate slowly; only one item is active at a time |
| Crash or reboot | Note what you were doing; capture serial log if possible |
| Save failed | Check SD card; run TEST SAVE |
| Entry missing in TODAY | Try ALL ENTRIES; run TEST SAVE |
| Voice too loud / silent | SETTINGS → VOICE CUES → OFF or TEST TONE |
| Press feels slow | ~550 ms delay is normal (allows double/triple press) |

---

## How to report bugs

```text
Screen:
Path: (e.g. HOME → REVIEW → PATTERNS)
What happened:
What I expected:
Did it reboot: yes / no
Any serial log:
Photo/video:
```

Save reports in your notes or project issues as you prefer.

---

## Daily trial safety notes

- CIRCE is **not** medical care and does **not** diagnose conditions.
- Reflections are stored **locally** on your SD card as personal data.
- You can **delete entries** from review/detail screens.
- You can **stop any flow** anytime (triple-press Home exits without saving partial entries).
- If the device behaves oddly, run **DIAGNOSTICS → TEST SAVE** before heavy use.

---

## Related docs

- `docs/releases/CIRCE_RC1_HARDWARE_SIGNOFF.md` — validation status
- `docs/ui/UI_FILE_MAP.md` — which files control each screen (for visual edits)
- `docs/ui/MANUAL_UI_EDITING_WORKFLOW.md` — token tuning + app-flash workflow
- `docs/ui/HOME_STATIC_BACKGROUND_MVP.md` — Home static HUD background
- `docs/ui/SCREEN_CAPTURE_GUIDE.md` — photo checklist for every screen
