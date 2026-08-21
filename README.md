# NUI Computer — projected-surface gesture interface on Raspberry Pi 5 + Hailo-10H

A consumer-computer prototype driven **entirely by hand gestures** on a small
projected surface (20.5 × 12.4 cm), running **100 % locally** (no cloud) on fixed
hardware. No physical keyboard/mouse; text input is a projected on-surface
keyboard. No microphone/speaker in the target hardware, so there is **no voice
path** (see ADR-0004).

> **Project status is NOT "complete".** The hardware-independent software is
> built, compiled and tested (see `docs/STATUS.md`), but parts of the perception
> pipeline are **blocked** pending Hailo model/tensor details that can only be
> obtained on the target, and end-to-end performance is **not measured** without
> the hardware. `docs/STATUS.md` is the authoritative, honest status.

## Fixed hardware (do not change)

- Raspberry Pi 5 (8 GB)
- Raspberry Pi AI Camera (Sony IMX500, on-sensor inference capable)
- Raspberry Pi AI HAT+ 2 — Hailo-10H (INT4, 8 GB dedicated LPDDR4X, PCIe Gen3 x1)
- Raspberry Pi OS 64-bit
- HDMI projector → surface 20.5 × 12.4 cm (≈ 16:10)

Key hardware facts and their sources are listed in `docs/ARCHITECTURE.md` §Sources.
Two facts that shape the design: the Hailo-10H's 8 GB is **dedicated to the NPU
and invisible to the Pi**; and its vision throughput is comparable to the older
26-TOPS Hailo-8 (it is a GenAI-oriented part).

## Repository layout

```
proto/            Shared wire contracts: nui_events.h (C, source of truth) + nui_events.py (ctypes mirror)
libs/nui_protocol AF_UNIX SOCK_SEQPACKET transport (UDS event bus)
services/engine   NUI core N1..N6 (C++, hardware-independent): 1€ filter, homography, contact fusion, gesture FSM
services/perception Capture + Hailo hand cascade (pipeline spec) + shadow contact (OpenCV) + .hef integrity + tracking gate
services/ui       Qt6 Quick fullscreen Wayland client (reticle, dwell ring, projected keyboard)
supervisor/       Optional Rust process supervisor (collapsible; systemd is primary)
tools/            Python: calibration (OpenCV), proto mirror, drift/calibration tests
tests/integration End-to-end QA over the real UDS bus (synthetic perception → engine → ui client)
deploy/           systemd units (two exclusive variants), tmpfiles, provisioning, install
docs/             ARCHITECTURE, BUILD, SECURITY, STATUS, and ADRs
```

## Build & test (development host)

See `docs/BUILD.md`. Quick version:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure       # 5 tests incl. end-to-end integration
( cd supervisor && cargo build --release )
python3 tools/tests/test_proto_sizes.py          # C/Python contract drift guard
```

## On-target bring-up

See `docs/BUILD.md` §On-target and `services/perception/README.md`. This includes
forcing PCIe Gen3, installing the Hailo stack, and the **blocked** step of writing
the three Hailo post-process `.so` files from the target's tensor layouts.

## License

See `LICENSE` (not included in this prototype snapshot).
