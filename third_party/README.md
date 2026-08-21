# third_party/

No vendored third-party source. All dependencies are consumed from the system
or language package managers (justified in docs/BUILD.md):
- C++: OpenCV, Qt6, GStreamer/TAPPAS, HailoRT, libcamera (system)
- Rust: std only (no crates)
- Python: numpy, opencv-python (pip); picamera2 (system, target-only)

Vendor code here only if a dependency must be pinned/patched.
