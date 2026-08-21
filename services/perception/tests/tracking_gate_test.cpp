// tracking_gate_test.cpp — verifies the gate policy and measures the reduction
// in palm-detection invocations vs the palm-every-frame baseline (O2/O7).
#include <cstdio>
#include "perception/tracking_gate.hpp"

// Count palm invocations over a scripted stream. `scores[i] < 0` means the hand
// is absent / landmark invalid on frame i.
static int palm_invocations(const float* scores, int n, const nui::TrackingGateConfig& cfg) {
    nui::TrackingGate gate(cfg);
    int palm = 0;
    for (int i = 0; i < n; ++i) {
        if (gate.begin_frame()) ++palm;
        bool valid = scores[i] >= 0.0f;
        gate.observe(valid, valid ? scores[i] : 0.0f);
    }
    return palm;
}

int main() {
    const int N = 100;

    // 1) Continuous good tracking: palm runs once (acquire), then never.
    {
        float s[N];
        for (int i = 0; i < N; ++i) s[i] = 0.9f;
        int palm = palm_invocations(s, N, nui::TrackingGateConfig{});
        if (palm != 1) { std::fprintf(stderr, "FAIL continuous: palm=%d (expected 1)\n", palm); return 1; }
        double reduction = 100.0 * (N - palm) / N;
        std::fprintf(stderr, "continuous: palm=%d/%d baseline=%d  reduction=%.0f%%\n",
                     palm, N, N, reduction);
    }

    // 2) One dropout mid-stream: palm runs twice (acquire + re-acquire).
    {
        float s[N];
        for (int i = 0; i < N; ++i) s[i] = 0.9f;
        s[50] = -1.0f; // hand lost on frame 50
        int palm = palm_invocations(s, N, nui::TrackingGateConfig{});
        if (palm != 2) { std::fprintf(stderr, "FAIL dropout: palm=%d (expected 2)\n", palm); return 1; }
        std::fprintf(stderr, "dropout: palm=%d/%d\n", palm, N);
    }

    // 3) Low presence keeps re-detecting (below presence_min -> never tracks).
    {
        float s[N];
        for (int i = 0; i < N; ++i) s[i] = 0.2f; // below default presence_min 0.5
        int palm = palm_invocations(s, N, nui::TrackingGateConfig{});
        if (palm != N) { std::fprintf(stderr, "FAIL lowconf: palm=%d (expected %d)\n", palm, N); return 1; }
        std::fprintf(stderr, "low-confidence: palm=%d/%d (no stable tracking)\n", palm, N);
    }

    // 4) Periodic re-anchor every 30 frames.
    {
        float s[N];
        for (int i = 0; i < N; ++i) s[i] = 0.9f;
        nui::TrackingGateConfig cfg; cfg.redetect_interval = 30;
        int palm = palm_invocations(s, N, cfg);
        // acquire at 0, then re-anchor at 30, 60, 90 -> 4
        if (palm != 4) { std::fprintf(stderr, "FAIL periodic: palm=%d (expected 4)\n", palm); return 1; }
        std::fprintf(stderr, "periodic(30): palm=%d/%d\n", palm, N);
    }

    std::fprintf(stderr, "OK: tracking gate policy + invocation reduction verified\n");
    return 0;
}
