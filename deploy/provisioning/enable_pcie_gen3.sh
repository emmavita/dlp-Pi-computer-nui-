#!/usr/bin/env bash
# enable_pcie_gen3.sh — force PCIe Gen3 for the AI HAT+ 2 (Hailo-10H).
# Without this the link negotiates Gen2 and halves PCIe bandwidth (Phase 3/9).
set -euo pipefail

CONFIG=/boot/firmware/config.txt          # Raspberry Pi OS Bookworm+ location
[ -f "$CONFIG" ] || CONFIG=/boot/config.txt  # older layout fallback
if [ ! -f "$CONFIG" ]; then
  echo "error: config.txt not found (looked in /boot/firmware and /boot)." >&2
  exit 1
fi

if grep -q '^dtparam=pciex1_gen=3' "$CONFIG"; then
  echo "PCIe Gen3 already enabled in $CONFIG."
else
  echo 'dtparam=pciex1_gen=3' | sudo tee -a "$CONFIG" >/dev/null
  echo "Added 'dtparam=pciex1_gen=3' to $CONFIG. Reboot required."
fi
