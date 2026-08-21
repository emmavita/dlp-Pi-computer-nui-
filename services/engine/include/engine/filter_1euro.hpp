// filter_1euro.hpp — 1€ filter (Casiez, Roussel, Vogel, CHI 2012).
// Low latency, adaptive: strong smoothing at rest, minimal lag when moving.
#ifndef NUI_ENGINE_FILTER_1EURO_HPP
#define NUI_ENGINE_FILTER_1EURO_HPP

#include <cmath>

namespace nui {

class LowPass {
public:
    void reset() { has_prev_ = false; prev_ = 0.0f; }
    float filter(float x, float alpha) {
        if (!has_prev_) { has_prev_ = true; prev_ = x; return x; }
        prev_ = alpha * x + (1.0f - alpha) * prev_;
        return prev_;
    }
private:
    bool  has_prev_ = false;
    float prev_ = 0.0f;
};

class OneEuroFilter {
public:
    OneEuroFilter(float min_cutoff = 1.0f, float beta = 0.0f, float dcutoff = 1.0f)
        : min_cutoff_(min_cutoff), beta_(beta), dcutoff_(dcutoff) {}

    // Clear all state (used when the tracked hand disappears, so a hand that
    // reappears elsewhere does not drag a stale filtered position with it).
    void reset() {
        xf_.reset();
        dxf_.reset();
        has_prev_ = false;
        prev_x_ = 0.0f;
        prev_t_ = 0.0;
    }

    // t in seconds (monotonic). Returns the filtered value.
    float filter(float x, double t) {
        if (!has_prev_) { has_prev_ = true; prev_x_ = x; prev_t_ = t; return x; }
        float dt = static_cast<float>(t - prev_t_);
        if (dt <= 0.0f) dt = 1e-3f;
        prev_t_ = t;
        float dx  = (x - prev_x_) / dt;
        float edx = dxf_.filter(dx, alpha(dcutoff_, dt));
        float cutoff = min_cutoff_ + beta_ * std::fabs(edx);
        float ex  = xf_.filter(x, alpha(cutoff, dt));
        prev_x_ = x;
        return ex;
    }

private:
    static float alpha(float cutoff, float dt) {
        const float kPi = 3.14159265358979323846f;
        float tau = 1.0f / (2.0f * kPi * cutoff);
        return 1.0f / (1.0f + tau / dt);
    }
    float   min_cutoff_, beta_, dcutoff_;
    LowPass xf_, dxf_;
    bool    has_prev_ = false;
    float   prev_x_ = 0.0f;
    double  prev_t_ = 0.0;
};

} // namespace nui

#endif // NUI_ENGINE_FILTER_1EURO_HPP
