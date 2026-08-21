# ADR-0005 — Rust supervisor optional/collapsible

**Context.** The brief requires a Rust component; systemd already restarts
services and manages the cgroup lifecycle.

**Decision.** A std-only, FFI-free Rust process supervisor (spawn + wait +
exponential backoff). It is explicitly optional and collapsible; systemd is the
primary lifecycle mechanism.

**Rationale.** Rust has no first-class bindings for HailoRT/Qt6/TAPPAS, so forcing
it into the hot path adds FFI friction for no gain. Process supervision is the one
place it fits without touching any frozen interface. Heartbeat-over-UDS was
rejected because it would require modifying the validated services and the frozen
protocol.

**Consequences.** Two exclusive systemd variants (direct services vs supervisor).
No signal handling (std can't do it portably without a crate/FFI); shutdown is
delegated to systemd's cgroup kill.
