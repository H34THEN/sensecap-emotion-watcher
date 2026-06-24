# Module: circe_assistant

**Layer:** Application  
**Phase:** 2

## Purpose

Conversational guide for the entry flow. Presents prompts, supports body-first routing, and optionally logs a lightweight transcript.

## Responsibilities

- Welcome and "how are you arriving today?" messaging
- Offer body-first vs emotion-first path
- Contextual hints between steps (non-directive)
- Enforce personality boundaries (see [circe-personality.md](../personality/circe-personality.md))

## Inputs

- Current flow state
- Partial `EntryDraft`
- User settings (verbosity, body-first default)

## Outputs

- Next flow state recommendation
- UI strings
- Optional `assistant_transcript` snippets

## UI

- Text panel with optional subtle animation (rlottie — optional)
- No chat LLM in Phase 2; scripted templates with variable substitution

## Dependencies

- Entry flow orchestrator
- All downstream UI modules (navigation only)

## Future

- LAN LLM via Hades with privacy-filtered context
- TTS via speaker for hands-free mode
