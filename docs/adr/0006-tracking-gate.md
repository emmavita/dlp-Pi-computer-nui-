# ADR-0006 — Tracking gate to cut palm-detection load

**Context.** The Phase-6 baseline ran palm detection every frame (simple, robust).

**Decision.** A pure-logic tracking gate runs palm detection only on acquisition
or loss (and optional periodic re-anchor), reusing the tracked ROI otherwise.

**Rationale.** Mirrors MediaPipe's tracking reuse. Measured ~99 % fewer palm
invocations in continuous tracking (logic-level proxy). The landmark network still
runs every frame; the real FPS/latency gain must be measured on target.

**Consequences.** The decision logic is delivered and tested off-target; wiring it
so the palm `hailonet` actually runs on demand is a target-side GStreamer task.
