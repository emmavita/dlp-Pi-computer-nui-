# On-target bring-up plan (Raspberry Pi 5 + AI HAT+ 2 / Hailo-10H)

Purpose: unblock the three open §17 criteria (pipelines validated, performance
measured, security validated) by turning them into a mechanical, verifiable
procedure to run **on the hardware**.

Conventions per step: **Goal**, **Commands**, **Acceptance**, **Unblocks**.
Where a command's exact form is not certain, it is marked `⚠️ verify` and you must
confirm it (usually via `--help`) before relying on it — do not assume.

Legend of §17 targets: [PIPE] pipelines · [PERF] performance · [SEC] security.

---

## T0 — OS baseline

**Goal.** Confirm a 64-bit Raspberry Pi OS on a Pi 5.
**Commands.**
```
cat /etc/os-release
uname -m            # expect: aarch64
cat /proc/device-tree/model   # expect: Raspberry Pi 5
```
**Acceptance.** `aarch64`, Pi 5, Raspberry Pi OS (Bookworm/Trixie).
**Unblocks.** foundation.

---

## T1 — PCIe Gen3

**Goal.** Force the AI HAT+ link to Gen3 (else Gen2, half bandwidth).
**Commands.**
```
deploy/provisioning/enable_pcie_gen3.sh   # adds dtparam=pciex1_gen=3
sudo reboot
# after reboot:
sudo lspci                                 # locate the Hailo endpoint  ⚠️ verify vendor string
sudo lspci -vvv                            # find LnkSta: Speed
```
**Acceptance.** The accelerator's `LnkSta:` shows **8GT/s**. If it shows 5GT/s,
Gen3 is not active — recheck config.txt path (`/boot/firmware/config.txt`).
Source: Raspberry Pi docs (`dtparam=pciex1_gen=3`).
**Unblocks.** [PERF].

---

## T2 — Hailo stack + device identity

**Goal.** Install HailoRT/TAPPAS and confirm the device is a Hailo-10H.
**Commands.**
```
deploy/provisioning/install_hailo_stack.sh
hailortcli fw-control identify
hailortcli --version          # ⚠️ verify flag via: hailortcli --help
dpkg -l | grep -i hailo       # record installed package versions
```
**Acceptance.** `fw-control identify` reports **Device Architecture: HAILO10H**;
HailoRT reported version is **≥ 5.3.0** (the version TAPPAS documents for the 10H;
source: `hailo-ai/tappas`). **Je ne sais pas** the exact minimum firmware version
required — record what is present and cross-check against Hailo release notes.
**Unblocks.** [PIPE], [PERF].

---

## T3 — Camera as plain sensor

**Goal.** Confirm the AI Camera (IMX500) works as a normal RGB camera.
**Commands.**
```
rpicam-hello --list-cameras     # expect an imx500 entry
rpicam-hello -t 3000            # brief preview
python3 -c "from picamera2 import Picamera2; print(Picamera2.global_camera_info())"  # ⚠️ verify method name
```
**Acceptance.** IMX500 is listed and previews. (No on-sensor network is loaded —
ADR-0002.)
**Unblocks.** [PIPE].

---

## T4 — Models + integrity

**Goal.** Place the Hailo-10H `.hef` and record checksums.
**Commands.**
```
# Obtain palm_detection_lite + hand_landmark_lite compiled for HAILO10H from the
# Hailo Model Zoo / hailo-apps tooling. EXACT retrieval command depends on the
# installed tooling — Je ne sais pas. Confirm via: hailomz --help  (⚠️ verify tool name)
ls models/*.hef
tools/models/fetch_models.sh models/ config/model_checksums.txt
```
**Acceptance.** Two `.hef` present; `config/model_checksums.txt` populated (the
perception service re-verifies these at load via `hef_integrity`).
**Unblocks.** [PIPE], [SEC].

---

## T5 — Extract tensor layouts + element properties (the key blocker input)

**Goal.** Get the exact input/output tensor shapes/formats of each `.hef` and the
real GStreamer element property names. This is the data needed to write the three
post-process `.so`.
**Commands.**
```
hailortcli parse-hef models/palm_detection_lite_h10.hef    # ⚠️ verify subcommand via hailortcli --help
hailortcli parse-hef models/hand_landmark_lite_h10.hef
gst-inspect-1.0 hailonet
gst-inspect-1.0 hailofilter
gst-inspect-1.0 hailocropper
gst-inspect-1.0 hailoaggregator
dpkg -L hailo-tappas-core | grep -Ei 'include|\.hpp'       # locate post-process headers
```
**Acceptance.** You have, written down: for each `.hef` the input size/format and
the output tensor name(s), shape(s) and quantization; and the confirmed property
names (`hef-path`, `so-path`, `function-name`, `internal-offset`, …). Update
`services/perception/pipeline/hand_cascade.gst` placeholders (W/H, hef paths,
property names) accordingly.
**Unblocks.** [PIPE].

---

## T6 — Write the three post-process `.so` (BLOCKED until T5)

**Goal.** Implement `libpalm_post.so`, `libhand_crop.so`, `libhand_post.so`
against the TAPPAS post-process API (headers located in T5) and the tensor layouts.
**Notes.**
- `libpalm_post`: decode palm-detection output tensors → palm boxes (+ keypoints
  for orientation) into HailoROI metadata.
- `libhand_crop`: `prepare_crops` selecting top-1 (or top-2) palm ROI(s).
- `libhand_post`: decode `hand_landmark_lite` output → 21 landmarks + score.
- **Rotation risk (ADR/Phase 6):** `hailocropper` produces axis-aligned crops; if
  `hand_landmark_lite` expects a rotation-normalized crop, accuracy drops.
  Mitigate: pad the crop, or do the affine rotated crop in `prepare_crops`, or
  fine-tune the model (Hailo Dataflow Compiler). Decide by measuring in T7.
**Acceptance.** The three `.so` build against the installed TAPPAS headers, no
warnings. (These cannot be written off-target — they need T5 outputs.)
**Unblocks.** [PIPE].

---

## T7 — Validate the hand pipeline in isolation

**Goal.** Prove the two-stage cascade produces stable 21-landmark output on a live
hand.
**Commands.**
```
# Build the pipeline from hand_cascade.gst with real element props (T5) and the
# .so from T6. Visual check via an overlay + display, e.g.:
gst-launch-1.0 <hand_cascade pipeline> ! hailooverlay ! autovideosink   # ⚠️ verify hailooverlay availability
# Headless landmark sanity: dump metadata from the appsink branch.
```
**Acceptance.** Landmarks track a moving hand smoothly under the projector light;
palm detection re-acquires after the hand leaves/re-enters; measured FPS ≥ the NUI
target (aim ≥ 30). If landmarks are unstable on tilted hands, apply the T6 rotation
mitigation. Record the FPS here.
**Unblocks.** [PIPE], [PERF].

---

## T8 — Wire perception → UDS

**Goal.** Emit `nui_hand_state_t` (and `nui_contact_state_t` from `shadow_contact`)
over the UDS bus, so `nui-engine` consumes the real stream.
**Notes.** The output contract is already frozen (`proto/nui_events.h`); the
`appsink` callback fills `nui_hand_state_t` from the landmark metadata and sends via
`nui_protocol`. Keep heavy work off the blocking GStreamer callback (worker thread +
SPSC, per Phase 6).
**Acceptance.** Running `nui-engine` + this perception, `qa_ui_client` (or `nui-ui`)
receives coherent pointer/gesture events from real hand motion.
**Unblocks.** [PIPE].

---

## T9 — Calibration

**Goal.** Real homography, pinch thresholds, and shadow parameters.
**Commands.**
```
python3 tools/calibration/capture_grid.py -n 10 -o calib/     # projected chessboard
python3 tools/calibration/detect_pattern.py calib/calib_000.png --cols 7 --rows 5
python3 tools/calibration/compute_homography.py corr.json -o /etc/nui/calibration.matrix
# Pinch thresholds from labelled recordings of the real landmark stream:
python3 tools/calibration/calibrate_pinch.py pinch_dataset.json
```
**Acceptance.** Reticle aligns with the fingertip on the projected surface
(homography valid); `calibrate_pinch` reports balanced accuracy ≥ 0.9 on real data.
Timing thresholds (t_dwell, t_long, swipe_speed) tuned from timed recordings and
written to `config/gestures.yaml`. **Je ne sais pas** the final values — they are
data-derived on target.
**Unblocks.** [PIPE], [PERF] (usability).

---

## T10 — End-to-end on hardware

**Goal.** Full chain camera → Hailo → engine → ui on the projected surface.
**Commands.**
```
deploy/install.sh
sudo systemctl enable --now nui-engine nui-perception nui-ui   # Variant A
systemctl status nui-engine nui-perception nui-ui
journalctl -u nui-perception -f
```
**Acceptance.** All gestures from the vocabulary (TAP, DWELL, DRAG, SWIPE, HOME,
BACK) work by hand on the surface; UI responds; no service crash-loops.
**Unblocks.** [PIPE].

---

## T11 — Performance measurement

**Goal.** Produce the real end-to-end latency and FPS figures.
**Commands.**
```
hailortcli run models/hand_landmark_lite_h10.hef      # per-model FPS/latency  ⚠️ verify subcommand
hailortcli benchmark models/hand_landmark_lite_h10.hef # ⚠️ verify subcommand
# Pipeline FPS + stage latency (perception side): PerfProbe (perf_probe.hpp),
# wired into the appsink callback (see services/perception/README.md), reports
# fps + capture->inference->publish percentiles and a CSV. UI-side end-to-end:
# End-to-end latency + effective FPS: qa_ui_client already reports p50/p90/p95/
# p99/mean/std and fps, and can dump a per-sample CSV. Enable with:
#   NUI_QA_DURATION_S=60 NUI_QA_WARMUP_S=3 NUI_LAT_CSV=/tmp/lat.csv build/qa_ui_client
# For this to be TRUE end-to-end, perception MUST stamp header.timestamp_ns
# with the camera CAPTURE time (CLOCK_MONOTONIC); the engine forwards it
# unchanged. Until then the figure is the downstream (engine) stage only.
```
**Acceptance.** Documented median/95p end-to-end latency and sustained FPS, with
CPU load (`top`/`htop`) and thermal (`vcgencmd measure_temp`). Compare against the
"low latency / ≥30 FPS" goal. **Je ne sais pas** the numbers without running this.
**Unblocks.** [PERF] — this is the criterion that cannot be checked any other way.

---

## T12 — Security validation

**Goal.** Confirm hardening and lock down device access.
**Commands.**
```
systemd-analyze security nui-engine.service
systemd-analyze security nui-perception.service
systemd-analyze security nui-ui.service
ls -l /dev/hailo* /dev/video* /dev/media* /dev/dri /dev/dma_heap/*   # confirm real node names
ss -x -a | grep nui        # verify only AF_UNIX sockets, no TCP
```
**Acceptance.** `systemd-analyze security` exposure acceptable for engine (aim a
low score); `nui-perception`/`nui-ui` tightened with `DeviceAllow=` for the exact
nodes found (replace the placeholder comment in the units); no network sockets
opened by any service. A malformed-frame injection test added to CI (fail-closed).
**Unblocks.** [SEC].

---

## T13 — §17 re-evaluation

Re-run `docs/STATUS.md` table with the T7/T10/T11/T12 evidence. Only when
**pipelines validated** (T7–T10), **performance conforms** (T11) and **security
validated** (T12) are demonstrated on hardware — in addition to the already-met
software criteria — is the project complete per §17. Until then it is not.

---

## Sources / uncertainty

- PCIe Gen3, camera stack, OS: Raspberry Pi documentation.
- HailoRT/TAPPAS, `hailortcli`, `parse-hef`, Model Zoo: Hailo documentation and
  `hailo-ai` GitHub. Exact subcommand/flag spellings marked `⚠️ verify` must be
  confirmed with `--help` on the installed version — I do not assert them.
- Items marked **"Je ne sais pas"** are genuinely unknown without the target and
  must not be guessed.
