# Changelog

Format based on Keep a Changelog. This project is a prototype and is **not
complete** (see docs/STATUS.md).

## [0.1.0-prototype] — 2026

### Added
- Shared wire contracts (`proto/nui_events.h` + ctypes mirror `nui_events.py`)
  with a C/Python size-drift guard.
- UDS `SOCK_SEQPACKET` transport (`libs/nui_protocol`) with backpressure policy.
- NUI engine N1..N6 (`services/engine`): 1€ filter, homography mapping, contact
  fusion (pinch/dwell + shadow), gesture FSM, HOME/BACK shape gestures.
- Perception building blocks (`services/perception`): shadow-adjacency contact
  (OpenCV), self-contained SHA-256 `.hef` integrity, tracking gate, and the real
  GStreamer/TAPPAS hand-cascade pipeline **specification**.
- Qt6 Quick fullscreen Wayland UI (`services/ui`): reticle, dwell ring, projected
  keyboard, gesture-driven shell.
- Optional Rust process supervisor (`supervisor/`).
- Python tools (`tools/`): projector-camera calibration (OpenCV), pinch-threshold
  calibration.
- systemd units (two exclusive variants), tmpfiles, provisioning, install
  (`deploy/`).
- End-to-end integration test over the real UDS bus (`tests/integration`).
- Documentation: README, ARCHITECTURE, BUILD, SECURITY, STATUS, ADRs.

### Known limitations / blocked
- Three Hailo post-process `.so` (palm decode, crop decision, landmark decode)
  are NOT written — they require on-target tensor layouts and TAPPAS headers.
- End-to-end latency/FPS not measured (no target hardware).
- Real gesture/contact thresholds and homography require on-target calibration.
- Single-camera true-touch contact is unproven (highest technical risk).

### Fixed (inconsistencies caught during development)
- Protocol header size corrected 24 → 28 bytes.
- Removed voice path (no mic/speaker) → projected keyboard.
- UI uses `QSocketNotifier` (QLocalSocket lacks SEQPACKET).
- 1€ filter state reset on hand disappearance (found by the integration test).
- Bus moved from `/tmp` to `/run/nui` (PrivateTmp would break a `/tmp` bus).
