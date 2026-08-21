// engine_core.hpp — orchestrates the NUI pipeline N1..N6 for one hand stream.
// Pure logic: no camera, no Hailo, no Qt (testable off-target, per Phase 4).
#ifndef NUI_ENGINE_ENGINE_CORE_HPP
#define NUI_ENGINE_ENGINE_CORE_HPP

#include <vector>
#include "nui_events.h"
#include "engine/filter_1euro.hpp"
#include "engine/homography_map.hpp"
#include "engine/contact_fusion.hpp"
#include "engine/gesture_fsm.hpp"

namespace nui {

struct EngineConfig {
    Homography    homography;                 // from tools/calibration
    ContactConfig contact;
    FsmConfig     fsm;
    float         one_euro_min_cutoff = 1.0f; // à calibrer
    float         one_euro_beta       = 0.007f; // à calibrer
    float         t_home_s            = 0.80f;  // open palm hold (HOME)
    float         t_back_s            = 0.25f;  // fist (BACK) min hold
};

struct EngineOutput {
    bool                             has_pointer = false;
    nui_pointer_event_t              pointer{};
    std::vector<nui_gesture_event_t> gestures;
};

class EngineCore {
public:
    explicit EngineCore(const EngineConfig& cfg = {});

    // shadow may be null. t in seconds (monotonic).
    EngineOutput update(const nui_hand_state_t& hand,
                        const nui_contact_state_t* shadow, double t);

private:
    EngineConfig  cfg_;
    OneEuroFilter fx_, fy_;
    ContactFusion fusion_;
    GestureFsm    fsm_;

    // Continuous motion / gesture bookkeeping.
    bool   have_prev_ = false;
    float  prev_u_ = 0, prev_v_ = 0;
    double prev_t_ = 0;

    // HOME/BACK shape timers.
    bool   open_active_ = false; double open_since_ = 0;
    bool   fist_active_ = false; double fist_since_ = 0;
};

} // namespace nui

#endif // NUI_ENGINE_ENGINE_CORE_HPP
