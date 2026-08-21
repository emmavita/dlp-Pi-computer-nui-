#!/usr/bin/env bash
# Build all C++ targets and the Rust supervisor.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
cmake -S . -B build -DCMAKE_BUILD_TYPE="${1:-Release}"
cmake --build build -j"$(nproc)"
( cd supervisor && cargo build --release )
echo "build complete."
