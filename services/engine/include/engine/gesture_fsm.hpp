// gesture_fsm.hpp — N5: gesture recognizer, explicit finite state machine.
// Matches the Phase 2 / Phase 5 state diagram (IDLE, TRACKING, ENGAGED,
// DRAGGING, SWIPE). Two-hand ZOOM is deferred: it requires a two-hand message
// variant not present in nui_hand_state_t (documented limit).
//
// Threshold defaults are INITIAL PLACEHOLDERS (à calibrer), not validated.
#ifndef NUI_ENGINE_GESTURE_FSM_HPP
#define NUI_ENGINE_GESTURE_FSM_HPP

#include <vector>
#include "nui_events.h"

namespace nui {

struct FsmConfig {
    float t_dwell_s     = 0.80f; // à calibrer
    float t_long_s      = 0.60f; // à calibrer
    float tap_max_s     = 0.30f; // à calibrer
    float tap_move      = 0.03f; // surface units (0..1) — à calibrer
    float drag_eps      = 0.04f; // à calibrer
    float dwell_move    = 0.02f; // stationarity tolerance — à calibrer
    float swipe_speed   = 1.50f; // surface units / s — à calibrer
};

enum class FsmState { Idle, Tracking, Engaged, Dragging, Swipe };

class GestureFsm {
public:
    explicit GestureFsm(const FsmConfig& cfg = {}) : cfg_(cfg) {}

    // present: hand tracked; (u,v): surface coords; speed: surface units/s;
    // engaged: contact/pinch/dwell engage from N4; t: seconds.
    // Appends any emitted gesture events to `out`.
    void update(bool present, float u, float v, float speed, bool engaged,
                double t, std::vector<nui_gesture_event_t>& out);

    FsmState state() const { return state_; }

private:
    FsmConfig cfg_;
    FsmState  state_ = FsmState::Idle;

    float  engage_u_ = 0, engage_v_ = 0;
    double engage_t_ = 0;
    float  dwell_u_ = 0, dwell_v_ = 0;
    double dwell_t_ = 0;
    float  swipe_u_ = 0, swipe_v_ = 0;
};

} // namespace nui

#endif // NUI_ENGINE_GESTURE_FSM_HPP
