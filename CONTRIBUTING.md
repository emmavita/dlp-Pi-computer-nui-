# Contributing

## Build & test before every change

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure     # must stay green
python3 tools/tests/test_proto_sizes.py        # C/Python contract must not drift
python3 tools/tests/test_calibration.py
python3 tools/tests/test_calibrate_pinch.py
( cd supervisor && cargo build --release )
```

## Coding standards

- C++17, compiled with `-Wall -Wextra -Wpedantic`, zero warnings.
- No invented APIs/libraries. If an external interface is unknown, stop and mark
  it — do not guess. See the "Je ne sais pas" entries in docs/STATUS.md.
- Keep modules single-responsibility with minimal dependencies (see docs/ARCHITECTURE.md).
- The wire contract in `proto/` is the single source of truth. Any change must
  update both `nui_events.h` and `nui_events.py`; `test_proto_sizes.py` enforces this.

## Interfaces are frozen

`nui_hand_state_t`, `nui_contact_state_t`, `nui_pointer_event_t`,
`nui_gesture_event_t` are consumed across process boundaries. Changing them
requires bumping `NUI_PROTO_VERSION` and updating all three services.

## Architecture decisions

Significant decisions are recorded in `docs/adr/`. Add a new ADR (same format)
for any load-bearing change rather than editing history.

## Target-only work

Hailo post-process `.so`, real calibration, and performance measurement happen on
the Raspberry Pi target. See docs/BUILD.md §On-target and services/perception/README.md.
