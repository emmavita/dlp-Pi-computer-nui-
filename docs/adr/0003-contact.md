# ADR-0003 — Hybrid contact detection

**Context.** A single 2D RGB camera cannot reliably tell "touching" from
"hovering". Depth/IR hardware is not allowed (fixed config).

**Decision.** Primary selection = pinch (thumb-index) or dwell — content- and
light-independent, robust. Shadow-adjacency is an optional enhancement that
auto-degrades to pinch/dwell when confidence is low (dark content, bright ambient).

**Rationale.** Shadow-based touch (Wilson 2005; Dai & Chung 2012) is the canonical
single-camera method but its robust variants add a dedicated IR illuminant
(forbidden here) or use the known projected image (adds a ui→perception coupling).
Camera-only adjacency keeps the flow unidirectional and is honest about its limits.

**Consequences.** True surface-touch reliability is the project's highest risk and
is **not guaranteed**; the interaction model does not depend on it. Predicted-image
robustness is a future enhancement needing a controlled ui→perception frame channel.
