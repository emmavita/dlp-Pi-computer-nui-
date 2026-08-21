// perf_probe_test.cpp — validates FPS and stage-latency statistics on synthetic
// timings (no hardware). Exits non-zero on failure.
#include <cstdio>
#include <cmath>
#include "perception/perf_probe.hpp"

static bool near(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

int main() {
    nui::PerfConfig cfg;
    cfg.report_interval_s = 0.0; // disable auto-report noise
    cfg.window = 1000;
    nui::PerfProbe probe(cfg);

    // 100 frames at 33 ms spacing (~30.3 FPS). Per frame:
    //   inference done 5 ms after capture, publish 8 ms after capture.
    const uint64_t base = 1'000'000'000ull; // arbitrary monotonic origin
    const uint64_t dt   = 33'000'000ull;    // 33 ms
    for (int i = 0; i < 100; ++i) {
        uint64_t cap = base + uint64_t(i) * dt;
        uint64_t inf = cap + 5'000'000ull;  // +5 ms
        uint64_t pub = cap + 8'000'000ull;  // +8 ms
        probe.record(cap, inf, pub);
    }

    nui::PerfStats s = probe.stats();
    std::fprintf(stderr, "%s\n", probe.report().c_str());

    int fail = 0;
    if (s.n != 100) { std::fprintf(stderr, "FAIL n=%zu (expected 100)\n", s.n); fail = 1; }
    if (!near(s.fps, 1000.0 / 33.0, 0.5)) { std::fprintf(stderr, "FAIL fps=%.3f\n", s.fps); fail = 1; }
    if (!near(s.cap2pub_p50_ms, 8.0, 1e-3)) { std::fprintf(stderr, "FAIL cap2pub_p50=%.4f\n", s.cap2pub_p50_ms); fail = 1; }
    if (!near(s.cap2pub_mean_ms, 8.0, 1e-3)) { std::fprintf(stderr, "FAIL cap2pub_mean=%.4f\n", s.cap2pub_mean_ms); fail = 1; }
    if (!near(s.cap2inf_p50_ms, 5.0, 1e-3)) { std::fprintf(stderr, "FAIL cap2inf_p50=%.4f\n", s.cap2inf_p50_ms); fail = 1; }
    if (!near(s.inf2pub_p50_ms, 3.0, 1e-3)) { std::fprintf(stderr, "FAIL inf2pub_p50=%.4f\n", s.inf2pub_p50_ms); fail = 1; }

    // Window eviction: keep only the last `window` frames.
    nui::PerfConfig cfg2; cfg2.report_interval_s = 0.0; cfg2.window = 50;
    nui::PerfProbe p2(cfg2);
    for (int i = 0; i < 200; ++i) {
        uint64_t cap = base + uint64_t(i) * dt;
        p2.record(cap, cap + 5'000'000ull, cap + 8'000'000ull);
    }
    if (p2.stats().n != 50) { std::fprintf(stderr, "FAIL window n=%zu (expected 50)\n", p2.stats().n); fail = 1; }

    if (fail) return 1;
    std::fprintf(stderr, "OK: perf probe fps + stage-latency stats verified\n");
    return 0;
}
