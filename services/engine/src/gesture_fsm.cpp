// gesture_fsm.cpp — implementation of the N5 gesture state machine.
#include "engine/gesture_fsm.hpp"
#include <cmath>

namespace nui {

namespace {
float dist2d(float ax, float ay, float bx, float by) {
    float dx = ax - bx, dy = ay - by;
    return std::sqrt(dx * dx + dy * dy);
}
nui_gesture_event_t mk(uint16_t type, float x, float y,
                       float p0 = 0.0f, float p1 = 0.0f) {
    nui_gesture_event_t g;
    g.type = type; g._pad = 0; g.x = x; g.y = y; g.param0 = p0; g.param1 = p1;
    return g;
}
} // namespace

void GestureFsm::update(bool present, float u, float v, float speed,
                        bool engaged, double t,
                        std::vector<nui_gesture_event_t>& out) {
    if (!present) {
        if (state_ == FsmState::Dragging)
            out.push_back(mk(NUI_GESTURE_DRAG_END, u, v));
        state_ = FsmState::Idle;
        return;
    }

    switch (state_) {
    case FsmState::Idle:
        state_ = FsmState::Tracking;
        dwell_u_ = u; dwell_v_ = v; dwell_t_ = t;
        break;

    case FsmState::Tracking:
        if (engaged) {
            state_ = FsmState::Engaged;
            engage_u_ = u; engage_v_ = v; engage_t_ = t;
        } else if (speed > cfg_.swipe_speed) {
            state_ = FsmState::Swipe;
            swipe_u_ = u; swipe_v_ = v;
        } else {
            if (dist2d(u, v, dwell_u_, dwell_v_) > cfg_.dwell_move) {
                dwell_u_ = u; dwell_v_ = v; dwell_t_ = t; // moved: reset anchor
            } else if (t - dwell_t_ > cfg_.t_dwell_s) {
                out.push_back(mk(NUI_GESTURE_DWELL_SELECT, u, v));
                dwell_u_ = u; dwell_v_ = v; dwell_t_ = t; // consume
            }
        }
        break;

    case FsmState::Engaged: {
        float moved = dist2d(u, v, engage_u_, engage_v_);
        if (!engaged) {
            double dt = t - engage_t_;
            if (dt <= cfg_.tap_max_s && moved <= cfg_.tap_move)
                out.push_back(mk(NUI_GESTURE_TAP, engage_u_, engage_v_));
            state_ = FsmState::Tracking;
            dwell_u_ = u; dwell_v_ = v; dwell_t_ = t;
        } else if (moved > cfg_.drag_eps) {
            out.push_back(mk(NUI_GESTURE_DRAG_BEGIN, engage_u_, engage_v_));
            state_ = FsmState::Dragging;
        } else if (t - engage_t_ > cfg_.t_long_s) {
            out.push_back(mk(NUI_GESTURE_LONG_PRESS, engage_u_, engage_v_));
            state_ = FsmState::Tracking; // consume
            dwell_u_ = u; dwell_v_ = v; dwell_t_ = t;
        }
        break;
    }

    case FsmState::Dragging:
        if (!engaged) {
            out.push_back(mk(NUI_GESTURE_DRAG_END, u, v));
            state_ = FsmState::Tracking;
            dwell_u_ = u; dwell_v_ = v; dwell_t_ = t;
        }
        // Continuous drag position is conveyed by pointer events (EngineCore).
        break;

    case FsmState::Swipe:
        if (speed < cfg_.swipe_speed * 0.5f) {
            float dir = std::atan2(v - swipe_v_, u - swipe_u_);
            out.push_back(mk(NUI_GESTURE_SWIPE, u, v, dir));
            state_ = FsmState::Tracking;
            dwell_u_ = u; dwell_v_ = v; dwell_t_ = t;
        }
        break;
    }
}

} // namespace nui
