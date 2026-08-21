// qa_hands.hpp — shared synthetic hand-state builders for the QA harnesses.
// Single source so qa_perception and qa_malformed cannot drift apart.
#ifndef NUI_QA_HANDS_HPP
#define NUI_QA_HANDS_HPP

#include <cstring>
#include "nui_events.h"

namespace qa {

// Index-only extended hand (ext count == 1). pinch=true -> thumb on index tip.
inline nui_hand_state_t hand_pointer(bool pinch, float ux, float uy) {
    nui_hand_state_t h; std::memset(&h, 0, sizeof(h));
    h.present = 1; h.handedness = 1; h.score = 0.99f;
    auto set = [&](int i, float x, float y) { h.lm[i].x = x; h.lm[i].y = y; h.lm[i].visibility = 1.0f; };
    set(0, 0.50f, 0.90f);
    set(5, 0.50f, 0.58f); set(6, 0.50f, 0.55f); set(8, ux, uy);   // index extended
    set(9, 0.50f, 0.60f);
    set(10, 0.50f, 0.62f); set(12, 0.50f, 0.86f);
    set(14, 0.50f, 0.62f); set(16, 0.50f, 0.86f);
    set(18, 0.50f, 0.62f); set(20, 0.50f, 0.86f);
    if (pinch) set(4, ux, uy);
    else       set(4, 0.50f, 0.75f);
    return h;
}

// Open palm (ext >= 4, no pinch).
inline nui_hand_state_t hand_open() {
    nui_hand_state_t h; std::memset(&h, 0, sizeof(h));
    h.present = 1; h.handedness = 1; h.score = 0.99f;
    auto set = [&](int i, float x, float y) { h.lm[i].x = x; h.lm[i].y = y; h.lm[i].visibility = 1.0f; };
    set(0, 0.50f, 0.95f); set(9, 0.50f, 0.65f);
    set(5, 0.42f, 0.66f); set(6, 0.42f, 0.55f); set(8, 0.42f, 0.35f);
    set(10, 0.50f, 0.60f); set(12, 0.50f, 0.32f);
    set(14, 0.58f, 0.60f); set(16, 0.58f, 0.35f);
    set(18, 0.66f, 0.62f); set(20, 0.66f, 0.40f);
    set(4, 0.30f, 0.55f);
    return h;
}

// Fist (ext == 0, thumb far from index to avoid pinch).
inline nui_hand_state_t hand_fist() {
    nui_hand_state_t h; std::memset(&h, 0, sizeof(h));
    h.present = 1; h.handedness = 1; h.score = 0.99f;
    auto set = [&](int i, float x, float y) { h.lm[i].x = x; h.lm[i].y = y; h.lm[i].visibility = 1.0f; };
    set(0, 0.50f, 0.90f); set(9, 0.50f, 0.60f);
    set(5, 0.50f, 0.58f); set(6, 0.50f, 0.55f); set(8, 0.40f, 0.85f);
    set(10, 0.50f, 0.62f); set(12, 0.50f, 0.86f);
    set(14, 0.50f, 0.62f); set(16, 0.50f, 0.86f);
    set(18, 0.50f, 0.62f); set(20, 0.50f, 0.86f);
    set(4, 0.60f, 0.85f);
    return h;
}

inline nui_hand_state_t hand_absent() {
    nui_hand_state_t h; std::memset(&h, 0, sizeof(h));
    h.present = 0; return h;
}

} // namespace qa

#endif // NUI_QA_HANDS_HPP
