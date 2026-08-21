// tracking_gate.hpp — O2: MediaPipe-style tracking gate (pure logic).
//
// Policy: run palm detection only when the hand is NOT currently tracked (or on
// a periodic re-anchor). While a hand is tracked, the landmark ROI from the
// previous frame is reused and palm detection is skipped, cutting Hailo load.
//
// This module is the POLICY only (when to run palm). Wiring it into the
// GStreamer/TAPPAS graph so the palm hailonet actually runs on demand is a
// target-side task (depends on the TAPPAS graph); the decision logic is here
// and fully testable off-target.
#ifndef NUI_PERCEPTION_TRACKING_GATE_HPP
#define NUI_PERCEPTION_TRACKING_GATE_HPP

namespace nui {

struct TrackingGateConfig {
    float presence_min     = 0.5f; // landmark presence to keep tracking — à calibrer
    int   redetect_interval = 0;   // 0 = never force; >0 = re-run palm every N frames
};

class TrackingGate {
public:
    explicit TrackingGate(const TrackingGateConfig& cfg = {}) : cfg_(cfg) {}

    // Call at the start of each frame: true => palm detection must run this frame.
    bool begin_frame() {
        bool run = !tracking_ ||
                   (cfg_.redetect_interval > 0 && frames_since_detect_ >= cfg_.redetect_interval);
        if (run) frames_since_detect_ = 0;
        else     ++frames_since_detect_;
        return run;
    }

    // Feed this frame's landmark result to update the tracking state.
    void observe(bool landmark_valid, float presence_score) {
        tracking_ = landmark_valid && presence_score >= cfg_.presence_min;
    }

    bool tracking() const { return tracking_; }

private:
    TrackingGateConfig cfg_;
    bool tracking_ = false;
    int  frames_since_detect_ = 0;
};

} // namespace nui

#endif // NUI_PERCEPTION_TRACKING_GATE_HPP
