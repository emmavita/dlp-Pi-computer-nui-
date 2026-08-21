# models/

Hailo-10H `.hef` model files live here at runtime. They are **not** committed
(`.gitignore` excludes `*.hef`) because they are large, target-specific binaries.

Place on the target (see `services/perception/README.md` and `docs/BUILD.md`):
- `palm_detection_lite_h10.hef`
- `hand_landmark_lite_h10.hef`

Then record their integrity hashes with `tools/models/fetch_models.sh`.
