# hand_cascade.gst — two-stage MediaPipe-style hand pipeline on Hailo-10H.
#
# This is the REAL element graph (Phase 6 §1), expressed as a gst-launch-style
# description. Every element here exists in the Hailo TAPPAS stack:
#   libcamerasrc   — libcamera GStreamer source (IMX500 as a plain RGB camera)
#   hailonet       — runs a .hef on the Hailo-10H NPU
#   hailofilter    — runs a post-process .so on the network output metadata
#   hailocropper   — cascade cropper: 1 sink, 2 srcs (orig frame + crops),
#                    crop decision provided by a .so (prepare_crops)
#   hailoaggregator— cascade aggregator: 2 sinks, 1 src (merges crop metadata
#                    back into the original frame)
#   appsink        — hand-off to the C++ worker thread
#
# Cascade topology: cropper.src_0 -> aggregator.sink_0 (original frame),
#                   cropper.src_1 -> landmark branch -> aggregator.sink_1.
#
# WARNING (verify on target, do NOT assume):
#   - Exact ELEMENT PROPERTY NAMES (so-path / function-name / hef-path /
#     internal-offset ...) must be confirmed with `gst-inspect-1.0 hailonet`,
#     `gst-inspect-1.0 hailofilter`, `gst-inspect-1.0 hailocropper` on the
#     installed TAPPAS version. They are indicative here.
#   - The three post-process .so files (palm decode, crop decision, landmark
#     decode) CANNOT be written without the output-tensor layout of the
#     specific Hailo-10H .hef models and the TAPPAS post-process headers.
#     Those are the blocked items of Phase 7.2 (see README.md). Not stubbed.
#
# W/H = camera capture resolution over the projected-surface ROI (set from
# config/pipeline.yaml after calibration). Values intentionally left as
# placeholders to be filled by calibration — not invented here.

libcamerasrc
  ! video/x-raw,format=RGB,width=W,height=H
  ! queue leaky=downstream max-size-buffers=2
  ! hailonet hef-path=<palm_detection_lite_h10.hef>
  ! hailofilter so-path=<libpalm_post.so> function-name=<palm_decode>
  ! hailocropper so-path=<libhand_crop.so> function-name=<prepare_crops> internal-offset=true name=cropper
  hailoaggregator name=agg
  cropper.
    ! agg.
  cropper.
    ! queue
    ! hailonet hef-path=<hand_landmark_lite_h10.hef>
    ! hailofilter so-path=<libhand_post.so> function-name=<landmark_decode>
    ! agg.
  agg.
    ! queue
    ! appsink name=out emit-signals=true sync=false
