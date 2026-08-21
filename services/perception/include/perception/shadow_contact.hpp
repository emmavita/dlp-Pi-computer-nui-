// shadow_contact.hpp — N4 shadow channel (perception side): estimate finger
// contact on the projected surface from the camera frame alone, using the
// shadow-adjacency heuristic (Phase 6 §2): under projector light the gap
// between the fingertip and the tip of its shadow tends to zero at contact.
//
// Camera-frame-only by design: it does NOT require the UI's rendered frame,
// so the perception -> engine -> ui dataflow stays unidirectional (Phase 6 §0c).
// Known limit (Phase 2): unreliable on dark projected content -> reported via a
// low `confidence`, on which the engine (N4) auto-falls-back to pinch/dwell.
//
// Threshold defaults are INITIAL PLACEHOLDERS (à calibrer), not validated.
#ifndef NUI_PERCEPTION_SHADOW_CONTACT_HPP
#define NUI_PERCEPTION_SHADOW_CONTACT_HPP

#include <opencv2/core.hpp>

namespace nui {

struct ShadowConfig {
    int    roi_radius_px   = 60;    // à calibrer
    int    shadow_luma_max = 60;    // [0..255] dark threshold — à calibrer
    double min_shadow_area = 20.0;  // px^2 — à calibrer
    float  gap_on_norm     = 0.06f; // engage when gap/roi < on — à calibrer
    float  gap_off_norm    = 0.12f; // release hysteresis (> on) — à calibrer
    float  light_dir_x     = 0.0f;  // expected shadow direction (unit vector)
    float  light_dir_y     = 1.0f;  // — à calibrer (projector vs camera)
    int    ambient_luma_min = 40;   // ROI mean luma below -> low confidence — à calibrer
    float  conf_luma_ref   = 160.0f;// luma giving full confidence — à calibrer
};

struct ShadowResult {
    bool  engaged     = false;
    float confidence  = 0.0f; // [0..1]
    float gap_norm    = 1.0f; // normalized fingertip<->shadow-tip gap
    bool  shadow_found = false;
};

class ShadowContact {
public:
    explicit ShadowContact(const ShadowConfig& cfg = {}) : cfg_(cfg) {}

    // frame: 8UC3 BGR camera image. fingertip: pixel coordinates in `frame`.
    ShadowResult estimate(const cv::Mat& frame, const cv::Point2f& fingertip_px);

private:
    ShadowConfig cfg_;
    bool         engaged_ = false; // hysteresis state
};

} // namespace nui

#endif // NUI_PERCEPTION_SHADOW_CONTACT_HPP
