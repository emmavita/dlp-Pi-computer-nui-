// contact_fusion.hpp — N4: fuse pinch (from landmarks) and the shadow channel
// (from perception) into a single engage state, with hysteresis. Dwell is
// handled by the FSM (time/stationarity), not here.
//
// The threshold defaults below are INITIAL PLACEHOLDERS, not validated values.
// They MUST be replaced by calibration (config/gestures.yaml). Marked as such.
#ifndef NUI_ENGINE_CONTACT_FUSION_HPP
#define NUI_ENGINE_CONTACT_FUSION_HPP

#include "nui_events.h"
#include "engine/hand_shape.hpp"

namespace nui {

struct ContactConfig {
    float pinch_on_ratio  = 0.35f; // à calibrer — valeur initiale non validée
    float pinch_off_ratio = 0.45f; // à calibrer — hystérésis (> on)
    float shadow_conf_min = 0.60f; // à calibrer — seuil de confiance ombre
};

struct ContactOut {
    bool               engaged = false;
    nui_contact_method method  = NUI_CONTACT_NONE;
};

class ContactFusion {
public:
    explicit ContactFusion(const ContactConfig& cfg = {}) : cfg_(cfg) {}

    // Clear hysteresis state (used when the hand disappears).
    void reset() { pinch_engaged_ = false; }

    // shadow may be null (no shadow channel this frame).
    ContactOut update(const nui_hand_state_t& hand,
                      const nui_contact_state_t* shadow) {
        ContactOut out;

        // 1) Pinch with hysteresis (most robust, content-independent).
        float r = pinch_ratio(hand);
        if (!pinch_engaged_ && r < cfg_.pinch_on_ratio)  pinch_engaged_ = true;
        else if (pinch_engaged_ && r > cfg_.pinch_off_ratio) pinch_engaged_ = false;

        if (pinch_engaged_) {
            out.engaged = true;
            out.method  = NUI_CONTACT_PINCH;
            return out;
        }

        // 2) Shadow channel only if confident enough (auto-fallback otherwise).
        if (shadow && shadow->engaged &&
            shadow->confidence >= cfg_.shadow_conf_min) {
            out.engaged = true;
            out.method  = NUI_CONTACT_SHADOW;
        }
        return out;
    }

private:
    ContactConfig cfg_;
    bool          pinch_engaged_ = false;
};

} // namespace nui

#endif // NUI_ENGINE_CONTACT_FUSION_HPP
