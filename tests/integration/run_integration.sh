#!/usr/bin/env bash
# run_integration.sh — end-to-end QA over the real UDS bus:
#   qa_perception --> nui-engine --> qa_ui_client
# Asserts the deterministic core gestures and reports the rest + latency.
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

export NUI_PERCEPTION_SOCK=/tmp/qa_perc.sock
export NUI_UI_SOCK=/tmp/qa_ui.sock
rm -f "$NUI_PERCEPTION_SOCK" "$NUI_UI_SOCK"

BIN=build
ENG="$BIN/nui-engine"
QA_PERC="$BIN/qa_perception"
QA_UI="$BIN/qa_ui_client"

[ -x "$ENG" ] || { echo "missing $ENG"; exit 2; }
[ -x "$QA_PERC" ] || { echo "missing $QA_PERC"; exit 2; }
[ -x "$QA_UI" ] || { echo "missing $QA_UI"; exit 2; }

# Start engine (server on both sockets).
"$ENG" 2>/tmp/qa_engine.err &
ENG_PID=$!
sleep 0.3

# Start UI client (connects to ui socket, queued until engine accepts).
"$QA_UI" >/tmp/qa_ui.out 2>/tmp/qa_ui.err &
UI_PID=$!
sleep 0.2

# Run the producer script (blocks until done).
"$QA_PERC" 2>/tmp/qa_perc.err

# Wait for the UI client to reach its deadline and print the summary.
wait "$UI_PID"
kill "$ENG_PID" 2>/dev/null
wait "$ENG_PID" 2>/dev/null

echo "---- engine log ----"; tail -n 3 /tmp/qa_engine.err
echo "---- summary ----"
SUMMARY=$(cat /tmp/qa_ui.out)
echo "$SUMMARY"

# --- Assertions on the deterministic core set ---
get() { echo "$SUMMARY" | tr ' ' '\n' | grep "^$1=" | cut -d= -f2; }
fail=0
check_ge() { # name value min
  if [ "$(printf '%.0f' "$2")" -lt "$3" ]; then echo "FAIL: $1=$2 (< $3)"; fail=1; else echo "ok: $1=$2"; fi
}

check_ge pointers      "$(get pointers)"      1
check_ge TAP           "$(get TAP)"           1
check_ge DRAG_BEGIN    "$(get DRAG_BEGIN)"    1
check_ge DRAG_END      "$(get DRAG_END)"      1
check_ge DWELL_SELECT  "$(get DWELL_SELECT)"  1

echo "---- informational (non-blocking) ----"
echo "SWIPE=$(get SWIPE) HOME=$(get HOME) BACK=$(get BACK)"
echo "latency (engine stage, excl. camera+Hailo): med=$(get lat_ms_med) p95=$(get lat_ms_p95) max=$(get lat_ms_max) ms mean=$(get lat_ms_mean) std=$(get lat_ms_std) (n=$(get nlat))"
echo "throughput (synthetic producer cadence, NOT hardware FPS): fps=$(get fps)"

if [ "$fail" -ne 0 ]; then echo "INTEGRATION: FAIL"; exit 1; fi
echo "INTEGRATION: PASS"
