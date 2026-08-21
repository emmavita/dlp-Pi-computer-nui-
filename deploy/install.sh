#!/usr/bin/env bash
# install.sh — build and install the NUI computer on the target.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# 1) Build C++ services (engine, ui, and perception when its Hailo .so are ready).
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

# 2) Build the optional Rust supervisor.
( cd supervisor && cargo build --release )

# 3) Service user (system account, no login).
id nui &>/dev/null || sudo useradd --system --no-create-home --shell /usr/sbin/nologin nui

# 4) Install binaries.
sudo install -d /opt/nui/bin /etc/nui
sudo install -m0755 build/nui-engine build/nui-ui /opt/nui/bin/
if [ -f build/nui-perception ]; then
  sudo install -m0755 build/nui-perception /opt/nui/bin/
else
  echo "note: nui-perception not built yet (Hailo post-process .so pending on target)."
fi
sudo install -m0755 supervisor/target/release/nui-supervisor /opt/nui/bin/

# 5) systemd units + shared runtime dir.
sudo install -m0644 deploy/systemd/*.service deploy/systemd/*.target /etc/systemd/system/
sudo install -m0644 deploy/tmpfiles/nui.conf /etc/tmpfiles.d/nui.conf
sudo systemd-tmpfiles --create /etc/tmpfiles.d/nui.conf
sudo systemctl daemon-reload

cat <<'MSG'
Installed. Enable EXACTLY ONE variant:
  Variant A (recommended, direct services):
    sudo systemctl enable --now nui-engine nui-perception nui-ui
  Variant B (single Rust supervisor):
    sudo systemctl enable --now nui-supervisor
MSG
