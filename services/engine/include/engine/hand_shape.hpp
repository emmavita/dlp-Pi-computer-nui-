// hand_shape.hpp — coarse hand-shape features from 21 MediaPipe landmarks.
// Used for HOME (open palm) / BACK (fist) and pinch distance. Geometry only,
// no learned model. Indices follow the MediaPipe hand topology.
#ifndef NUI_ENGINE_HAND_SHAPE_HPP
#define NUI_ENGINE_HAND_SHAPE_HPP

#include <cmath>
#include "nui_events.h"

namespace nui {

// MediaPipe landmark indices used here.
enum HandLm {
    LM_WRIST = 0,
    LM_THUMB_TIP = 4,
    LM_INDEX_MCP = 5,
    LM_INDEX_PIP = 6,
    LM_INDEX_TIP = 8,
    LM_MIDDLE_MCP = 9,
    LM_MIDDLE_PIP = 10,
    LM_MIDDLE_TIP = 12,
    LM_RING_PIP = 14,
    LM_RING_TIP = 16,
    LM_PINKY_PIP = 18,
    LM_PINKY_TIP = 20
};

inline float dist(const nui_landmark_t& a, const nui_landmark_t& b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Hand scale = wrist -> middle MCP, used to normalize distances (scale/height
// invariance). Guarded against zero.
inline float hand_scale(const nui_hand_state_t& h) {
    float s = dist(h.lm[LM_WRIST], h.lm[LM_MIDDLE_MCP]);
    return s > 1e-4f ? s : 1e-4f;
}

// Pinch distance (thumb tip <-> index tip), normalized by hand scale.
inline float pinch_ratio(const nui_hand_state_t& h) {
    return dist(h.lm[LM_THUMB_TIP], h.lm[LM_INDEX_TIP]) / hand_scale(h);
}

// A finger is "extended" if its tip is farther from the wrist than its PIP.
inline bool finger_extended(const nui_hand_state_t& h, int tip, int pip) {
    return dist(h.lm[tip], h.lm[LM_WRIST]) > dist(h.lm[pip], h.lm[LM_WRIST]);
}

// Count of extended fingers among index, middle, ring, pinky (0..4).
inline int extended_finger_count(const nui_hand_state_t& h) {
    int c = 0;
    if (finger_extended(h, LM_INDEX_TIP,  LM_INDEX_PIP))  ++c;
    if (finger_extended(h, LM_MIDDLE_TIP, LM_MIDDLE_PIP)) ++c;
    if (finger_extended(h, LM_RING_TIP,   LM_RING_PIP))   ++c;
    if (finger_extended(h, LM_PINKY_TIP,  LM_PINKY_PIP))  ++c;
    return c;
}

} // namespace nui

#endif // NUI_ENGINE_HAND_SHAPE_HPP
