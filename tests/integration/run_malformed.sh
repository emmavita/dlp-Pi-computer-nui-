#!/usr/bin/env bash
# run_malformed.sh — inject malformed frames, assert the engine survives and
# still processes a valid gesture afterward (fail-open drop of bad datagrams).
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

export NUI_PERCEPTION_SOCK=/tmp/qa_mf_perc.sock
export NUI_UI_SOCK=/tmp/qa_mf_ui.sock
rm -f "$NUI_PERCEPTION_SOCK" "$NUI_UI_SOCK"

BIN=build
ENG="$BIN/nui-engine"; QA_MF="$BIN/qa_malformed"; QA_UI="$BIN/qa_ui_client"
for b in "$ENG" "$QA_MF" "$QA_UI"; do [ -x "$b" ] || { echo "missing $b"; exit 2; }; done

"$ENG" 2>/tmp/mf_engine.err &
ENG_PID=$!
sleep 0.3
"$QA_UI" >/tmp/mf_ui.out 2>/tmp/mf_ui.err &
UI_PID=$!
sleep 0.2
"$QA_MF" 2>/tmp/mf_send.err

wait "$UI_PID"
kill "$ENG_PID" 2>/dev/null; wait "$ENG_PID" 2>/dev/null

SUMMARY=$(cat /tmp/mf_ui.out)
DROPPED=$(grep -c "dropped malformed frame" /tmp/mf_engine.err || true)
get() { echo "$SUMMARY" | tr ' ' '\n' | grep "^$1=" | cut -d= -f2; }
TAP=$(get TAP); PTR=$(get pointers)

echo "dropped_frames=$DROPPED TAP=$TAP pointers=$PTR"
echo "summary: $SUMMARY"

# Key assertion: the engine must process the VALID TAP sent AFTER the 5 malformed
# frames, on the SAME connection. If any malformed frame had torn down the
# session, those later frames would never be read -> TAP=0. So TAP>=1 proves the
# engine dropped the bad frames and kept going. (The engine exiting afterwards is
# normal: qa_malformed closes the connection, which is a clean peer-close.)
fail=0
[ "${DROPPED:-0}" -ge 5 ] || { echo "FAIL: expected >=5 dropped frames, got ${DROPPED:-0}"; fail=1; }
[ "$(printf '%.0f' "${TAP:-0}")" -ge 1 ] || { echo "FAIL: no TAP after malformed barrage (engine did not survive)"; fail=1; }
[ "$(printf '%.0f' "${PTR:-0}")" -ge 1 ] || { echo "FAIL: no pointer events after barrage"; fail=1; }

if [ "$fail" -ne 0 ]; then echo "MALFORMED: FAIL"; exit 1; fi
echo "MALFORMED: PASS"
