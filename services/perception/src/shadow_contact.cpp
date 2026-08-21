// shadow_contact.cpp — implementation of the shadow-adjacency contact heuristic.
#include "perception/shadow_contact.hpp"

#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace nui {

ShadowResult ShadowContact::estimate(const cv::Mat& frame,
                                     const cv::Point2f& fingertip_px) {
    ShadowResult out;
    if (frame.empty() || frame.type() != CV_8UC3) return out;

    // 1) ROI around the fingertip, clamped to the image.
    int r = cfg_.roi_radius_px;
    int x0 = std::max(0, static_cast<int>(fingertip_px.x) - r);
    int y0 = std::max(0, static_cast<int>(fingertip_px.y) - r);
    int x1 = std::min(frame.cols, static_cast<int>(fingertip_px.x) + r);
    int y1 = std::min(frame.rows, static_cast<int>(fingertip_px.y) + r);
    if (x1 - x0 < 3 || y1 - y0 < 3) return out;
    cv::Rect roi(x0, y0, x1 - x0, y1 - y0);
    cv::Point2f tip_local(fingertip_px.x - x0, fingertip_px.y - y0);

    // 2) Grayscale + mean luma (content brightness under the finger).
    cv::Mat gray;
    cv::cvtColor(frame(roi), gray, cv::COLOR_BGR2GRAY);
    double mean_luma = cv::mean(gray)[0];

    // 3) Shadow mask: pixels darker than the shadow threshold.
    cv::Mat mask;
    cv::threshold(gray, mask, cfg_.shadow_luma_max, 255, cv::THRESH_BINARY_INV);

    // 4) Contours of shadow blobs.
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 5) Pick the shadow blob lying in the expected light direction and closest
    //    to the fingertip; measure the fingertip<->shadow-tip gap.
    const float lx = cfg_.light_dir_x, ly = cfg_.light_dir_y;
    float best_gap = std::numeric_limits<float>::max();
    bool  found = false;

    for (const auto& c : contours) {
        if (cv::contourArea(c) < cfg_.min_shadow_area) continue;

        cv::Moments m = cv::moments(c);
        if (m.m00 <= 0.0) continue;
        cv::Point2f centroid(static_cast<float>(m.m10 / m.m00),
                             static_cast<float>(m.m01 / m.m00));
        // Must be on the shadow side (dot with light direction > 0).
        if ((centroid.x - tip_local.x) * lx + (centroid.y - tip_local.y) * ly <= 0.0f)
            continue;

        // Nearest contour point to the fingertip = shadow "tip".
        for (const auto& p : c) {
            float dx = static_cast<float>(p.x) - tip_local.x;
            float dy = static_cast<float>(p.y) - tip_local.y;
            float d  = std::sqrt(dx * dx + dy * dy);
            if (d < best_gap) { best_gap = d; found = true; }
        }
    }

    out.shadow_found = found;
    if (!found) {
        engaged_ = false; // no shadow -> release
        out.engaged = false;
        out.confidence = 0.0f;
        out.gap_norm = 1.0f;
        return out;
    }

    out.gap_norm = best_gap / static_cast<float>(cfg_.roi_radius_px);

    // 6) Hysteresis on the normalized gap.
    if (!engaged_ && out.gap_norm < cfg_.gap_on_norm)  engaged_ = true;
    else if (engaged_ && out.gap_norm > cfg_.gap_off_norm) engaged_ = false;
    out.engaged = engaged_;

    // 7) Confidence: brighter content -> crisper shadow -> higher confidence.
    //    Dark content (mean_luma below ambient_luma_min) -> low confidence, so
    //    the engine ignores this channel and falls back to pinch/dwell.
    float conf = static_cast<float>(mean_luma) / cfg_.conf_luma_ref;
    conf = std::min(1.0f, std::max(0.0f, conf));
    if (mean_luma < cfg_.ambient_luma_min) conf *= 0.3f;
    out.confidence = conf;

    return out;
}

} // namespace nui
