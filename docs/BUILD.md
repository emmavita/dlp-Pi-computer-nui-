# Build & bring-up

## Toolchain (verified versions used during development)

- C++: g++ 13.3, `-std=c++17 -Wall -Wextra -Wpedantic` (clean, zero warnings)
- CMake ≥ 3.16
- OpenCV 4.6 (dev host) / 4.x on target
- Qt6 (Quick, Gui) — 6.4 on the dev host
- Rust ≥ 1.75 (supervisor), std-only, no external crates
- Python ≥ 3.9 with numpy + opencv-python; picamera2 is target-only

## Development-host build

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

Targets built: `nui-engine`, `nui-ui`, plus tests `engine_selftest`,
`hef_integrity_test`, `tracking_gate_test`, `shadow_contact_test`, and the
`integration` end-to-end test (`qa_perception → nui-engine → qa_ui_client` over
the real UDS bus). `nui-ui` builds only if Qt6 is found (otherwise skipped).

Rust supervisor:
```
( cd supervisor && cargo build --release )
```

Python tools & tests:
```
python3 tools/tests/test_proto_sizes.py     # C vs Python contract drift
python3 tools/tests/test_calibration.py     # homography + chessboard
python3 tools/tests/test_calibrate_pinch.py # pinch calibration logic
```

## On-target bring-up (Raspberry Pi 5 + AI HAT+ 2)

1. **PCIe Gen3** (else Gen2, half bandwidth):
   ```
   deploy/provisioning/enable_pcie_gen3.sh    # adds dtparam=pciex1_gen=3, reboot
   ```
   Verify: `sudo lspci -vvv` shows the link at 8 GT/s.

2. **Hailo + camera stack**:
   ```
   deploy/provisioning/install_hailo_stack.sh
   ```
   `hailo-all` is the Raspberry-Pi-documented meta-package; **verify** it pulls
   HailoRT ≥ 5.3.0 and TAPPAS core for the Hailo-10H (this is not confirmed here —
   "Je ne sais pas" for the exact package deltas). The script runs
   `hailortcli fw-control identify`, which must report `Device Architecture: HAILO10H`.

3. **Models + integrity**:
   - Obtain the Hailo-10H `.hef` for `palm_detection_lite` and `hand_landmark_lite`
     from the Model Zoo / hailo-apps tooling (exact retrieval command depends on the
     installed tooling version — not invented here). Place them in `models/`.
   - `tools/models/fetch_models.sh models/ config/model_checksums.txt` records their
     SHA-256, which `services/perception` re-checks at load (`hef_integrity`).

4. **BLOCKED — write the three post-process `.so`** (`services/perception/README.md`):
   `libpalm_post.so`, `libhand_crop.so`, `libhand_post.so`. These require the
   output-tensor layouts of the specific Hailo-10H `.hef` and the TAPPAS
   post-process/cropper entry-point signatures, readable only on the target
   (`gst-inspect-1.0 hailonet|hailofilter|hailocropper`). They are intentionally
   **not** written in this repo to avoid inventing APIs.

5. **Calibration**:
   - `tools/calibration/capture_grid.py` (Picamera2, on target) captures the
     projected pattern; `detect_pattern.py` + `compute_homography.py` produce the
     9-float matrix consumed by the engine (`NUI_CALIB`).
   - `tools/calibration/calibrate_pinch.py` derives pinch thresholds from labelled
     recordings (requires the on-target landmark stream). Timing thresholds
     (t_dwell, t_long, swipe_speed, …) require timed interaction recordings — also
     target-side.

6. **Install & run**:
   ```
   deploy/install.sh
   # Enable EXACTLY ONE variant:
   sudo systemctl enable --now nui-engine nui-perception nui-ui   # Variant A (recommended)
   # or
   sudo systemctl enable --now nui-supervisor                     # Variant B
   ```
   The UDS bus lives in `/run/nui` (created by `deploy/tmpfiles/nui.conf`), shared
   across the services (they use `PrivateTmp=yes`, so `/tmp` is not shared).

## UI as a Wayland client

`nui-ui` is a Wayland client and needs a running compositor session (labwc). The
`nui-ui.service` sets `XDG_RUNTIME_DIR`/`WAYLAND_DISPLAY` assuming uid 1000 —
**adjust to the target session**, or run it under a dedicated kiosk compositor
(e.g. `cage`, availability to verify). Tested on the dev host only in `offscreen`
mode (QML loads and runs); real Wayland rendering is target-side.
