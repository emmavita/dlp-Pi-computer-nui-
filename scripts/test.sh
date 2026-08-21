#!/usr/bin/env bash
# Run the full test suite (C++ ctest incl. integration + Python tests).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
ctest --test-dir build --output-on-failure
for t in test_proto_sizes test_calibration test_calibrate_pinch; do python3 "tools/tests/$t.py"; done
echo "all tests passed."
