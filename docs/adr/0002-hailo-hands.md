# ADR-0002 — Hand cascade on Hailo-10H; IMX500 as plain camera

**Context.** Two AI engines exist: the IMX500 (on-sensor) and the Hailo-10H.

**Decision.** Run both stages (palm detection + hand landmark) on the Hailo-10H,
using the Model-Zoo MediaPipe-derived `.hef`. Use the IMX500 as a plain RGB camera.

**Rationale.** Palm detection is infrequent (tracking reuse), so offloading it to
the IMX500 saves little while adding a second runtime, cross-chip sync, and an
unverified model conversion; the IMX500 runs one network at a time. Single runtime
= robustness + maintainability, and it maximizes Hailo use (project goal §12).

**Alternatives.** Palm on IMX500 + landmark on Hailo (rejected, above). Everything
on IMX500 (rejected: model/pipeline limits).

**Consequences.** The two-stage cascade uses TAPPAS `hailocropper`/`hailoaggregator`;
the three post-process `.so` must be written on target (blocked). IMX500 remains
available for an orthogonal on-sensor task later without stealing Hailo cycles.
