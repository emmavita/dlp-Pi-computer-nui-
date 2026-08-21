# services/perception — status (Phase 7.2)

The perception service has two independent responsibilities:

1. **Hand landmark cascade** (palm -> crop -> landmark) on the Hailo-10H, via a
   GStreamer/TAPPAS pipeline. See `pipeline/hand_cascade.gst`.
2. **Shadow contact channel** on the CPU (OpenCV), camera-frame only.

## Delivered and verified in this phase

- `src/shadow_contact.cpp` (+ header, + `tests/shadow_contact_test.cpp`)
  Complete OpenCV implementation of the shadow-adjacency heuristic (Phase 6 §2).
  Compiled with OpenCV 4.6 (`-Wall -Wextra -Wpedantic`, clean) and passes
  synthetic contact / hover / dark-content tests.
- `src/hef_integrity.cpp` (+ header, + `tests/hef_integrity_test.cpp`)
  Self-contained SHA-256 (FIPS 180-4) to verify `.hef` integrity before load,
  no external crypto dependency. Verified against FIPS test vectors and
  cross-checked against coreutils `sha256sum`.

## Blocked — cannot be written without the target's Hailo headers/models

The three post-process shared objects referenced by the pipeline are NOT written
here, on purpose. Writing them would require inventing APIs, which is forbidden:

- `libpalm_post.so`  (palm_detection decode)  — needs the output-tensor layout
  of `palm_detection_lite` compiled for Hailo-10H, and the TAPPAS `hailofilter`
  post-process entry-point signature.
- `libhand_crop.so`  (prepare_crops)          — needs the `hailocropper`
  crop-function signature and how palm detections are exposed as ROIs.
- `libhand_post.so`  (hand_landmark decode)   — needs the output-tensor layout
  of `hand_landmark_lite` compiled for Hailo-10H.

**Je ne sais pas** the exact signatures/tensor layouts from memory. They must be
read on the target from:
- the installed TAPPAS post-process headers,
- the Hailo Model Zoo entry for each `.hef` (input size, output tensor shapes),
- `gst-inspect-1.0 hailonet|hailofilter|hailocropper` for exact property names.

## On-target bring-up order

1. `dtparam=pciex1_gen=3` in `/boot/firmware/config.txt`, reboot; confirm
   `hailortcli fw-control identify` shows `Device Architecture: HAILO10H` at
   `8GT/s` (Phase 3/9).
2. Fetch Hailo-10H `.hef` for palm + hand landmark from the Model Zoo, record
   their SHA-256 into `config/pipeline.yaml` (consumed by `hef_integrity`).
3. `gst-inspect-1.0` the three Hailo elements; fill the property names and the
   `.gst` placeholders.
4. Write the three post-process `.so` from the tensor layouts, then wire the
   `appsink` callback to emit `nui_hand_state_t` over UDS (contract already
   fixed in `proto/nui_events.h`).

The output contract of this service (`nui_hand_state_t`, `nui_contact_state_t`)
is already frozen and consumed by `engine` (Phase 7.1), so the blocked work does
not affect any interface.

## Performance instrumentation (T11)

`perf_probe.hpp` (`PerfProbe`) is the perception-side latency/FPS harness. Wire it
into the appsink callback once the pipeline exists, using CLOCK_MONOTONIC ns:

    // once, at startup:
    nui::PerfConfig pc; pc.report_interval_s = 2.0; pc.csv_path = "/tmp/perc.csv";
    nui::PerfProbe perf(pc);

    // per frame, in the appsink callback:
    uint64_t t_capture = /* buffer capture time (PTS mapped to CLOCK_MONOTONIC) */;
    // ... run hand cascade ...
    uint64_t t_infer   = mono_ns();          // after hand_landmark decode
    // ... fill nui_hand_state_t, set header.timestamp_ns = t_capture ...
    uint64_t t_publish = mono_ns();          // just before the UDS send
    perf.record(t_capture, t_infer, t_publish);

Setting `header.timestamp_ns = t_capture` also feeds the UI-side end-to-end figure
(`qa_ui_client`). PerfProbe decomposes capture→inference→publish inside perception
WITHOUT touching the frozen wire protocol.
