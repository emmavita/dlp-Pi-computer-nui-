// shadow_contact_test.cpp — synthetic tests for the shadow-adjacency heuristic.
// Builds three controlled frames (contact / hover / dark content) and checks
// the estimator behaves as designed. Exits non-zero on failure.
#include <cstdio>
#include <opencv2/imgproc.hpp>
#include "perception/shadow_contact.hpp"

// Bright background with a dark shadow blob (filled circle) centered at
// `shadow_center`. Light direction is +y (shadow appears below the finger).
static cv::Mat make_frame(int bg_luma, cv::Point shadow_center, int shadow_r) {
    cv::Mat img(200, 200, CV_8UC3, cv::Scalar(bg_luma, bg_luma, bg_luma));
    if (shadow_r > 0)
        cv::circle(img, shadow_center, shadow_r, cv::Scalar(5, 5, 5), cv::FILLED);
    return img;
}

int main() {
    const cv::Point2f tip(100.0f, 100.0f);
    const int R = 15;

    // 1) CONTACT: shadow circle centered at tip+(0,R) so its top edge touches
    //    the fingertip -> gap ~ 0.
    {
        nui::ShadowContact sc; // default config: light_dir = (0,1), gap_on 0.06
        cv::Mat f = make_frame(200, cv::Point(100, 100 + R), R);
        nui::ShadowResult res = sc.estimate(f, tip);
        if (!res.shadow_found || !res.engaged) {
            std::fprintf(stderr, "FAIL contact: found=%d engaged=%d gap=%.3f\n",
                         res.shadow_found, res.engaged, res.gap_norm);
            return 1;
        }
        if (res.confidence < 0.6f) {
            std::fprintf(stderr, "FAIL contact confidence too low: %.3f\n", res.confidence);
            return 1;
        }
    }

    // 2) HOVER: shadow circle far below the fingertip -> large gap -> released.
    {
        nui::ShadowContact sc;
        cv::Mat f = make_frame(200, cv::Point(100, 100 + 3 * R), R);
        nui::ShadowResult res = sc.estimate(f, tip);
        if (res.engaged) {
            std::fprintf(stderr, "FAIL hover: engaged unexpectedly gap=%.3f\n", res.gap_norm);
            return 1;
        }
    }

    // 3) DARK CONTENT: whole ROI dark -> shadow mask fires everywhere ->
    //    apparent contact, BUT confidence must be low so the engine ignores it.
    {
        nui::ShadowContact sc;
        cv::Mat f = make_frame(25, cv::Point(100, 100 + R), R);
        nui::ShadowResult res = sc.estimate(f, tip);
        if (res.confidence >= 0.6f) { // 0.6 = engine's default shadow_conf_min
            std::fprintf(stderr, "FAIL dark: confidence not suppressed: %.3f\n", res.confidence);
            return 1;
        }
    }

    std::fprintf(stderr, "OK: shadow contact contact/hover/dark behave as designed\n");
    return 0;
}
