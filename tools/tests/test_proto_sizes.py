#!/usr/bin/env python3
"""test_proto_sizes.py — guard against C/Python protocol drift.

Compiles a tiny C probe that includes proto/nui_events.h and prints sizeof for
each wire struct, then asserts those sizes equal both the ctypes mirror
(proto/nui_events.py) and the documented EXPECTED_SIZES. If anyone edits one
side of the contract without the other, this test fails.

Runnable with pytest or directly (python3 test_proto_sizes.py).
"""
import ctypes
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
PROTO_DIR = os.path.join(REPO, "proto")

sys.path.insert(0, PROTO_DIR)
import nui_events as pe  # noqa: E402

C_PROBE = r"""
#include <stdio.h>
#include "nui_events.h"
int main(void) {
    printf("nui_header_t %zu\n", sizeof(nui_header_t));
    printf("nui_hand_state_t %zu\n", sizeof(nui_hand_state_t));
    printf("nui_contact_state_t %zu\n", sizeof(nui_contact_state_t));
    printf("nui_pointer_event_t %zu\n", sizeof(nui_pointer_event_t));
    printf("nui_gesture_event_t %zu\n", sizeof(nui_gesture_event_t));
    return 0;
}
"""


def _c_sizes():
    with tempfile.TemporaryDirectory() as td:
        src = os.path.join(td, "probe.c")
        exe = os.path.join(td, "probe")
        with open(src, "w", encoding="utf-8") as f:
            f.write(C_PROBE)
        cc = os.environ.get("CC", "gcc")
        subprocess.run([cc, "-I", PROTO_DIR, src, "-o", exe], check=True)
        out = subprocess.run([exe], check=True, capture_output=True, text=True).stdout
    sizes = {}
    for line in out.strip().splitlines():
        name, val = line.split()
        sizes[name] = int(val)
    return sizes


def test_sizes_match():
    c_sizes = _c_sizes()
    for name, py_struct in pe.PY_STRUCTS.items():
        py = ctypes.sizeof(py_struct)
        c = c_sizes[name]
        exp = pe.EXPECTED_SIZES[name]
        assert py == c == exp, (
            f"{name}: python={py} c={c} expected={exp} (drift detected)"
        )


def test_version_file_matches_header():
    # proto/VERSION must mirror NUI_PROTO_VERSION so the handshake constant and
    # the metadata file cannot drift.
    with open(os.path.join(PROTO_DIR, "VERSION"), encoding="utf-8") as f:
        file_version = int(f.read().strip())
    assert file_version == pe.NUI_PROTO_VERSION, (
        f"proto/VERSION={file_version} != NUI_PROTO_VERSION={pe.NUI_PROTO_VERSION}"
    )


if __name__ == "__main__":
    try:
        test_sizes_match()
        test_version_file_matches_header()
    except AssertionError as e:
        print("FAIL:", e, file=sys.stderr)
        sys.exit(1)
    c_sizes = _c_sizes()
    for n in pe.PY_STRUCTS:
        print(f"OK {n}: {c_sizes[n]} bytes (C == Python == expected)")
