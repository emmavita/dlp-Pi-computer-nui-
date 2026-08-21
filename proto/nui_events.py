"""nui_events.py — ctypes mirror of proto/nui_events.h.

Single source of truth is the C header; this file mirrors it so Python tools
(calibration, tests, replay) share the exact same wire layout. A drift test
(tools/tests/test_proto_sizes.py) compiles the C header and asserts every
struct size matches this mirror, so the two cannot silently diverge.

Layout matches the C header's #pragma pack(push, 1): all structs use _pack_ = 1
and little-endian (native on the ARM64 target; same-host IPC, no byte-swapping).
"""
import ctypes

NUI_PROTO_MAGIC = 0x4E554931  # 'N''U''I''1'
NUI_PROTO_VERSION = 1
NUI_NUM_LANDMARKS = 21

# enum nui_msg_type
NUI_MSG_HAND_STATE = 1
NUI_MSG_CONTACT_STATE = 2
NUI_MSG_POINTER_EVENT = 3
NUI_MSG_GESTURE_EVENT = 4

# enum nui_contact_method
NUI_CONTACT_NONE = 0
NUI_CONTACT_PINCH = 1
NUI_CONTACT_DWELL = 2
NUI_CONTACT_SHADOW = 3

# enum nui_gesture_type
NUI_GESTURE_TAP = 1
NUI_GESTURE_LONG_PRESS = 2
NUI_GESTURE_DRAG_BEGIN = 3
NUI_GESTURE_DRAG_END = 4
NUI_GESTURE_SWIPE = 5
NUI_GESTURE_SCROLL = 6
NUI_GESTURE_ZOOM = 7
NUI_GESTURE_HOME = 8
NUI_GESTURE_BACK = 9
NUI_GESTURE_DWELL_SELECT = 10


class NuiHeader(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("magic", ctypes.c_uint32),
        ("proto_version", ctypes.c_uint16),
        ("msg_type", ctypes.c_uint16),
        ("payload_len", ctypes.c_uint32),
        ("timestamp_ns", ctypes.c_uint64),
        ("sequence", ctypes.c_uint64),
    ]


class NuiLandmark(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("z", ctypes.c_float),
        ("visibility", ctypes.c_float),
    ]


class NuiHandState(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("present", ctypes.c_uint8),
        ("handedness", ctypes.c_uint8),
        ("_pad", ctypes.c_uint8 * 2),
        ("score", ctypes.c_float),
        ("lm", NuiLandmark * NUI_NUM_LANDMARKS),
    ]


class NuiContactState(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("engaged", ctypes.c_uint8),
        ("method", ctypes.c_uint8),
        ("_pad", ctypes.c_uint8 * 2),
        ("confidence", ctypes.c_float),
    ]


class NuiPointerEvent(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("ui_x", ctypes.c_float),
        ("ui_y", ctypes.c_float),
        ("confidence", ctypes.c_float),
    ]


class NuiGestureEvent(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("type", ctypes.c_uint16),
        ("_pad", ctypes.c_uint16),
        ("x", ctypes.c_float),
        ("y", ctypes.c_float),
        ("param0", ctypes.c_float),
        ("param1", ctypes.c_float),
    ]


# Expected sizes (must equal sizeof in C). Verified by test_proto_sizes.py.
EXPECTED_SIZES = {
    "nui_header_t": 28,
    "nui_hand_state_t": 344,
    "nui_contact_state_t": 8,
    "nui_pointer_event_t": 12,
    "nui_gesture_event_t": 20,
}

PY_STRUCTS = {
    "nui_header_t": NuiHeader,
    "nui_hand_state_t": NuiHandState,
    "nui_contact_state_t": NuiContactState,
    "nui_pointer_event_t": NuiPointerEvent,
    "nui_gesture_event_t": NuiGestureEvent,
}
