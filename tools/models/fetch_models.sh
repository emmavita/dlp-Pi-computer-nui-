#!/usr/bin/env bash
# fetch_models.sh — record SHA-256 of the Hailo-10H .hef models for the
# integrity check performed at load time by services/perception (hef_integrity).
#
# This script does NOT invent a download command. The compiled Hailo-10H .hef
# files (palm_detection_lite, hand_landmark_lite) must be obtained on the target
# from the Hailo Model Zoo / hailo-apps tooling for the HAILO10H architecture
# and placed in the models/ directory. Their exact retrieval command depends on
# the installed Hailo tooling version and is intentionally left to the operator
# (see services/perception/README.md).
#
# Usage: tools/models/fetch_models.sh <models_dir> [output_checksums_file]
set -euo pipefail

MODELS_DIR="${1:-models}"
OUT="${2:-config/model_checksums.txt}"

if [ ! -d "$MODELS_DIR" ]; then
  echo "error: models directory '$MODELS_DIR' not found." >&2
  echo "Place the Hailo-10H .hef files there first (see perception/README.md)." >&2
  exit 1
fi

shopt -s nullglob
HEFS=("$MODELS_DIR"/*.hef)
if [ ${#HEFS[@]} -eq 0 ]; then
  echo "error: no .hef files in '$MODELS_DIR'." >&2
  echo "Expected e.g. palm_detection_lite_h10.hef, hand_landmark_lite_h10.hef" >&2
  exit 1
fi

mkdir -p "$(dirname "$OUT")"
: > "$OUT"
for f in "${HEFS[@]}"; do
  # sha256sum is coreutils; the perception service recomputes and compares this.
  sha256sum "$f" | tee -a "$OUT"
done
echo "wrote checksums to $OUT"
