#!/usr/bin/env bash
# install_hailo_stack.sh — install the runtime dependencies on the target.
# Raspberry Pi documents the 'hailo-all' meta-package for AI HAT/Kit setup.
# For the AI HAT+ 2 (Hailo-10H), VERIFY it pulls HailoRT >= 5.3.0 and the TAPPAS
# core; the exact package deltas for the 10H may differ. Je ne sais pas the
# precise package set beyond 'hailo-all' without checking the target repo.
set -euo pipefail

sudo apt update
# Camera + libcamera stack and Picamera2 (used by tools/calibration on target).
sudo apt install -y rpicam-apps python3-picamera2
# Hailo runtime + TAPPAS (RPi-documented meta-package).
sudo apt install -y hailo-all

# Verify the accelerator is present and identified as HAILO10H.
if ! hailortcli fw-control identify; then
  echo "error: Hailo device not detected (check the HAT seating and PCIe)." >&2
  exit 1
fi
echo "Confirm the output above shows: Device Architecture: HAILO10H"
