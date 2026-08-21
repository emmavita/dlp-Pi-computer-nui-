// engine_core.cpp — implementation of the N1..N6 orchestration.
#include "engine/engine_core.hpp"
#include "engine/hand_shape.hpp"
#include <cmath>

namespace nui {

EngineCore::EngineCore(const EngineConfig& cfg)
    : cfg_(cfg),
      fx_(cfg.one_euro_min_cutoff, cfg.one_euro_beta, 1.0f),
      fy_(cfg.one_euro_min_cutoff, cfg.one_euro_beta, 1.0f),
      fusion_(cfg.contact),
      fsm_(cfg.fsm) {}

EngineOutput EngineCore::update(const nui_hand_state_t& hand,
                                const nui_contact_state_t* shadow, double t) {
    EngineOutput out;

    if (!hand.present) {
        // Still drive the FSM so a pending drag is closed cleanly.
        fsm_.update(false, prev_u_, prev_v_, 0.0f, false, t, out.gestures);
        have_prev_ = false;
        open_active_ = fist_active_ = false;
        // Clear filter/hysteresis state so a hand reappearing elsewhere does not
        // inherit a stale filtered position or engage state.
        fx_.reset();
        fy_.reset();
        fusion_.reset();
        return out;
    }

    // N2: filter the index tip (camera-normalized), then N3: map to surface.
    const nui_landmark_t& tip = hand.lm[LM_INDEX_TIP];
    float fx = fx_.filter(tip.x, t);
    float fy = fy_.filter(tip.y, t);
    float u, v;
    cfg_.homography.map(fx, fy, u, v);

    // Speed on the surface (units/s).
    float speed = 0.0f;
    if (have_prev_) {
        float dt = static_cast<float>(t - prev_t_);
        if (dt > 0.0f) {
            float du = u - prev_u_, dv = v - prev_v_;
            speed = std::sqrt(du * du + dv * dv) / dt;
        }
    }

    // N4: engage decision (pinch + shadow).
    ContactOut c = fusion_.update(hand, shadow);

    // HOME (open palm held) / BACK (fist held) — geometry-based shape gestures.
    int ext = extended_finger_count(hand);
    if (ext >= 4) {
        if (!open_active_) { open_active_ = true; open_since_ = t; }
        else if (t - open_since_ > cfg_.t_home_s) {
            out.gestures.push_back([&]{
                nui_gesture_event_t g; g.type = NUI_GESTURE_HOME; g._pad = 0;
                g.x = u; g.y = v; g.param0 = 0; g.param1 = 0; return g; }());
            open_active_ = false; // consume
        }
    } else {
        open_active_ = false;
    }
    if (ext == 0) {
        if (!fist_active_) { fist_active_ = true; fist_since_ = t; }
        else if (t - fist_since_ > cfg_.t_back_s) {
            out.gestures.push_back([&]{
                nui_gesture_event_t g; g.type = NUI_GESTURE_BACK; g._pad = 0;
                g.x = u; g.y = v; g.param0 = 0; g.param1 = 0; return g; }());
            fist_active_ = false; // consume
        }
    } else {
        fist_active_ = false;
    }

    // N5: gesture FSM.
    fsm_.update(true, u, v, speed, c.engaged, t, out.gestures);

    // N6: pointer event (continuous).
    out.has_pointer = true;
    out.pointer.ui_x = u;
    out.pointer.ui_y = v;
    out.pointer.confidence = hand.score;

    prev_u_ = u; prev_v_ = v; prev_t_ = t; have_prev_ = true;
    return out;
}

} // namespace nui
