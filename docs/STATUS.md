# Status — honest, authoritative

This file is the single source of truth for what is and is not done. It does not
overclaim. Evidence = actually executed in the development environment.

## Completion criteria (project brief §17)

| Criterion | State | Evidence / reason |
|-----------|-------|-------------------|
| Architecture validated | ✅ | Phases 1–6 validated incrementally; ADRs recorded |
| Coherent tree | ✅ | 57 source & config files; layout matches architecture |
| Compilable code | ✅ | All C++ targets build clean (`-Wall -Wextra -Wpedantic`, 0 warnings); Rust builds; QML loads |
| Complete documentation | ✅ (software) | README + ARCHITECTURE + BUILD + SECURITY + ADRs; on-target procedures documented |
| Dependencies verified | ✅ | All real and justified; versions in BUILD.md |
| **Pipelines validated** | ❌ | The Hailo hand-cascade is **not** validated on hardware — 3 post-process `.so` blocked + no target |
| Coherent diagrams | ✅ | Mermaid diagrams consistent with code |
| **Performance meets targets** | ❌ (partial) | Engine stage measured (median 0.062 ms, Phase 8); **end-to-end latency/FPS NOT measured** (no target) |
| **Security validated** | ⚠️ partial | Design + systemd hardening verified (`systemd-analyze verify`); no runtime pen-test; device ACLs pending |
| Tests pass | ✅ (delivered scope) | `ctest` 6/6 incl. end-to-end integration + malformed-frame injection; Python 3/3 |
| No inconsistency | ✅ | Every detected inconsistency was corrected in-phase (see below) |

**Verdict: the project is NOT complete.** Three criteria (pipelines validated,
performance conforms, security fully validated) cannot be satisfied without the
physical target and without writing the Hailo post-process `.so` (which require
on-target tensor layouts). Per §17, the project must not be declared finished.

## What is proven here (executed)

- C++ full build + `ctest` 5/5:
  `engine_selftest`, `hef_integrity_test` (FIPS vectors + coreutils cross-check),
  `tracking_gate_test` (~99 % palm-invocation reduction in continuous tracking),
  `shadow_contact_test` (contact/hover/dark), `malformed` (injection: engine drops bad frames, keeps serving) and `integration` (perception→engine→ui
  over the real UDS bus; TAP/DRAG/DWELL/SWIPE/HOME/BACK; engine-stage latency).
- Rust supervisor builds and runs (spawn/backoff verified).
- Python: C↔Python protocol drift guard passes (28/344/8/12/20 bytes); calibration
  and pinch-calibration logic verified.
- systemd units validate; QML loads and runs headless (offscreen).

## Blocked (require the target; cannot be written without inventing APIs)

1. **Three Hailo post-process `.so`** (`libpalm_post`, `libhand_crop`, `libhand_post`)
   — need the Hailo-10H `.hef` output-tensor layouts and TAPPAS entry-point
   signatures. Blocks the real perception hand pipeline.
2. **`appsink` → UDS wiring** in perception (trivial once the `.so` exist; the
   output contract `nui_hand_state_t` is already frozen).

## Open — require the target to measure/validate

- End-to-end latency and FPS (camera + Hailo + render).
- Hand-landmark robustness under top-down view with the projected image on the
  hand; possible DFC fine-tuning.
- Two-stage crop rotation: `hailocropper` does axis-aligned crops; whether the
  compiled `hand_landmark_lite` needs a rotation-normalized crop is **unknown** —
  measure and mitigate (pad / custom affine crop / fine-tune).
- Zero-copy DMABUF path capture→`hailonet` (unknown on the 10H stack).
- Real gesture thresholds (pinch via `calibrate_pinch.py`; timing thresholds need
  timed recordings).
- Real projector-camera homography (via `tools/calibration`).
- Shadow-contact viability under real projector illumination and ambient light —
  the project's highest technical risk (single 2D camera, no depth).
- GStreamer element property names confirmed via `gst-inspect-1.0`.
- Wayland rendering under labwc; device ACLs for perception/ui.

## Inconsistencies detected and corrected during the project

- Header size 24 → **28 bytes** (arithmetic error, Phase 6 → corrected 7.1).
- Voice/GenAI path removed (no mic/speaker in hardware) → projected keyboard.
- `QLocalSocket` cannot do SEQPACKET → `QSocketNotifier` + additive `recv_msg_ex`.
- Predicted-image contact would create a `ui→perception` dependency → switched to
  camera-only shadow-adjacency (unidirectional flow preserved).
- Supervisor "heartbeat over UDS" would require changing frozen services →
  process supervision instead.
- 1€ filter state persisted across hand disappearance (found by the integration
  test) → reset on absence.
- `PrivateTmp=yes` would break a `/tmp` bus → moved the bus to `/run/nui`.

## "Je ne sais pas" (explicitly unknown)

Exact `.hef` input dims and tensor layouts; end-to-end latency/FPS; exact Hailo
device node names; whether `hailo-all` fully covers HailoRT ≥ 5.3.0 for the 10H;
exact Model-Zoo `.hef` download command; convertibility of MediaPipe palm to the
IMX500; current (post-Jan-2026) maturity of the hand pipeline on the 10H.
