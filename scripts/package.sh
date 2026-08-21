#!/usr/bin/env bash
# Create a clean source ZIP (excludes build artifacts and caches).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
OUT="${1:-/tmp/nui-computer.zip}"
rm -f "$OUT"
zip -r "$OUT" . \
  -x '*/build/*' -x 'build/*' \
  -x '*/target/*' -x 'supervisor/target/*' \
  -x '*/__pycache__/*' -x '*.pyc' \
  -x '*/.git/*' >/dev/null
echo "created $OUT"
