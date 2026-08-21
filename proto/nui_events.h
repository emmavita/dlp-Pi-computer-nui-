/* nui_events.h — Single source of truth for inter-service message contracts.
 *
 * Fixed-layout, little-endian (native on the ARM64 target; both endpoints run
 * on the same host, so no byte-swapping is performed). Consumed by the C++
 * services and mirrored by proto/nui_events.py (a size-equality test guards
 * against drift). Corresponds exactly to the Phase 2 interface contracts:
 *   HandState, ContactState, PointerEvent, GestureEvent.
 */
#ifndef NUI_EVENTS_H
#define NUI_EVENTS_H

#include <stdint.h>

#define NUI_PROTO_MAGIC   0x4E554931u /* 'N''U''I''1' */
#define NUI_PROTO_VERSION 1u
#define NUI_NUM_LANDMARKS 21          /* MediaPipe hand topology */

#ifdef __cplusplus
extern "C" {
#endif

enum nui_msg_type {
    NUI_MSG_HAND_STATE    = 1, /* perception -> engine (continuous) */
    NUI_MSG_CONTACT_STATE = 2, /* perception -> engine (continuous) */
    NUI_MSG_POINTER_EVENT = 3, /* engine -> ui       (continuous)   */
    NUI_MSG_GESTURE_EVENT = 4  /* engine -> ui       (discrete)     */
};

enum nui_contact_method {
    NUI_CONTACT_NONE   = 0,
    NUI_CONTACT_PINCH  = 1,
    NUI_CONTACT_DWELL  = 2,
    NUI_CONTACT_SHADOW = 3
};

enum nui_gesture_type {
    NUI_GESTURE_TAP          = 1,
    NUI_GESTURE_LONG_PRESS   = 2,
    NUI_GESTURE_DRAG_BEGIN   = 3,
    NUI_GESTURE_DRAG_END     = 4,
    NUI_GESTURE_SWIPE        = 5,
    NUI_GESTURE_SCROLL       = 6,
    NUI_GESTURE_ZOOM         = 7,  /* reserved: needs two-hand message (deferred) */
    NUI_GESTURE_HOME         = 8,
    NUI_GESTURE_BACK         = 9,
    NUI_GESTURE_DWELL_SELECT = 10
};

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;         /* NUI_PROTO_MAGIC */
    uint16_t proto_version; /* NUI_PROTO_VERSION */
    uint16_t msg_type;      /* enum nui_msg_type */
    uint32_t payload_len;   /* bytes following the header */
    uint64_t timestamp_ns;  /* capture timestamp (latency measurement) */
    uint64_t sequence;      /* monotonic per link (loss/order detection) */
} nui_header_t;             /* 28 bytes (packed) */

typedef struct {
    float x;          /* normalized [0..1] in camera frame */
    float y;
    float z;          /* RELATIVE MediaPipe depth — NON-metric, never used for contact */
    float visibility; /* [0..1] */
} nui_landmark_t;

typedef struct {
    uint8_t        present;    /* 0/1 */
    uint8_t        handedness; /* 0=left, 1=right */
    uint8_t        _pad[2];
    float          score;      /* hand presence score [0..1] */
    nui_landmark_t lm[NUI_NUM_LANDMARKS];
} nui_hand_state_t;

typedef struct {
    uint8_t engaged;    /* 0/1 */
    uint8_t method;     /* enum nui_contact_method (shadow channel only here) */
    uint8_t _pad[2];
    float   confidence; /* [0..1] */
} nui_contact_state_t;

typedef struct {
    float ui_x;       /* normalized [0..1] on projected surface */
    float ui_y;
    float confidence; /* [0..1] */
} nui_pointer_event_t;

typedef struct {
    uint16_t type;   /* enum nui_gesture_type */
    uint16_t _pad;
    float    x;      /* surface coords [0..1] where relevant */
    float    y;
    float    param0; /* swipe direction (rad) / scroll dy / zoom delta */
    float    param1;
} nui_gesture_event_t;

#pragma pack(pop)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NUI_EVENTS_H */
