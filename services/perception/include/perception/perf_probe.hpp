// perf_probe.hpp — perception-side latency/FPS instrumentation for T11.
//
// Wire this into the (target-side) appsink callback of the hand pipeline: record
// three CLOCK_MONOTONIC timestamps per frame — capture, inference-done, publish
// (just before the UDS send) — and it maintains rolling FPS + stage-latency
// statistics, emits a periodic report, and can dump a per-frame CSV.
//
// Design (O(1) per frame): record() only pushes to a bounded ring; percentiles
// are computed lazily in stats()/report() (every report_interval_s), so the hot
// path stays cheap. Single-threaded use (call from one thread, e.g. the worker)
// — no locking, no added latency. Timestamps must be monotonic and ordered
// (capture <= inference <= publish).
//
// It does NOT modify the frozen wire protocol: stage decomposition is internal to
// perception; the header still carries only timestamp_ns = capture time, which is
// what qa_ui_client uses for the end-to-end (UI-side) figure.
#ifndef NUI_PERCEPTION_PERF_PROBE_HPP
#define NUI_PERCEPTION_PERF_PROBE_HPP

#include <cstdint>
#include <cstdio>
#include <deque>
#include <string>

namespace nui {

struct PerfConfig {
    double      report_interval_s = 2.0; // auto-emit a report every N seconds (0 = never)
    std::size_t window            = 300; // rolling number of frames kept for stats
    std::string csv_path;                // if non-empty, append per-frame CSV
};

struct PerfStats {
    double      fps            = 0.0; // effective publish rate over the window
    double      cap2pub_p50_ms = 0.0; // capture -> publish (end-to-end within perception)
    double      cap2pub_p95_ms = 0.0;
    double      cap2pub_p99_ms = 0.0;
    double      cap2pub_mean_ms = 0.0;
    double      cap2inf_p50_ms = 0.0; // capture -> inference done (Hailo path)
    double      inf2pub_p50_ms = 0.0; // inference done -> publish (post/UDS)
    std::size_t n              = 0;
};

class PerfProbe {
public:
    explicit PerfProbe(const PerfConfig& cfg = {});
    ~PerfProbe();

    PerfProbe(const PerfProbe&) = delete;
    PerfProbe& operator=(const PerfProbe&) = delete;

    // Per-frame. Timestamps in CLOCK_MONOTONIC ns, ordered cap <= inf <= pub.
    // If a stage is not separately measured, pass inf == cap (or inf == pub).
    void record(uint64_t t_capture_ns, uint64_t t_inference_ns, uint64_t t_publish_ns);

    PerfStats   stats() const;   // compute over the current window
    std::string report() const;  // one-line formatted stats()

private:
    struct Rec { uint64_t cap, inf, pub; };
    PerfConfig       cfg_;
    std::deque<Rec>  recs_;
    uint64_t         interval_ns_ = 0;
    uint64_t         last_report_ = 0;
    std::FILE*       csv_ = nullptr;
};

} // namespace nui

#endif // NUI_PERCEPTION_PERF_PROBE_HPP
