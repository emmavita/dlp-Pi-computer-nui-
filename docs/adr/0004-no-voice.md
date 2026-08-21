# ADR-0004 — No voice path; projected keyboard for text

**Context.** The fixed hardware has no microphone or speaker.

**Decision.** No voice command / STT / TTS path. Text input is an on-surface
projected keyboard selected by dwell/pinch. All feedback is visual.

**Rationale.** Voice is not implementable without added hardware, which is
forbidden. This corrects the Phase-1 sketch that included a voice/GenAI path.

**Consequences.** Text entry is slower (acceptable for short commands). The
Hailo-10H's GenAI capability is unused in the committed core (a hook remains for
future added hardware). Visual feedback (reticle, dwell ring) is mandatory, not
cosmetic, since it is the only feedback channel.
