// perf_probe.cpp — implementation of the perception latency/FPS probe.
#include "perception/perf_probe.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace nui {

namespace {
double percentile(const std::vector<double>& s, double p) {
    if (s.empty()) return 0.0;
    if (s.size() == 1) return s[0];
    double rank = p * (double(s.size()) - 1.0);
    std::size_t lo = std::size_t(std::floor(rank));
    double frac = rank - double(lo);
    if (lo + 1 < s.size()) return s[lo] + frac * (s[lo + 1] - s[lo]);
    return s[lo];
}
} // namespace

PerfProbe::PerfProbe(const PerfConfig& cfg) : cfg_(cfg) {
    interval_ns_ = uint64_t(cfg_.report_interval_s * 1e9);
    if (!cfg_.csv_path.empty()) {
        csv_ = std::fopen(cfg_.csv_path.c_str(), "w");
        if (csv_) std::fprintf(csv_, "capture_ns,inference_ns,publish_ns,cap2pub_ms\n");
    }
}

PerfProbe::~PerfProbe() {
    if (csv_) std::fclose(csv_);
}

void PerfProbe::record(uint64_t t_capture_ns, uint64_t t_inference_ns, uint64_t t_publish_ns) {
    recs_.push_back(Rec{t_capture_ns, t_inference_ns, t_publish_ns});
    if (recs_.size() > cfg_.window) recs_.pop_front();

    if (csv_) {
        double c2p = (t_publish_ns >= t_capture_ns)
                         ? double(t_publish_ns - t_capture_ns) / 1e6 : -1.0;
        std::fprintf(csv_, "%llu,%llu,%llu,%.6f\n",
                     (unsigned long long)t_capture_ns,
                     (unsigned long long)t_inference_ns,
                     (unsigned long long)t_publish_ns, c2p);
    }

    if (interval_ns_ > 0) {
        if (last_report_ == 0) last_report_ = t_publish_ns;
        if (t_publish_ns - last_report_ >= interval_ns_) {
            last_report_ = t_publish_ns;
            std::fprintf(stderr, "%s\n", report().c_str());
        }
    }
}

PerfStats PerfProbe::stats() const {
    PerfStats out;
    out.n = recs_.size();
    if (recs_.empty()) return out;

    std::vector<double> c2p, c2i, i2p;
    c2p.reserve(recs_.size()); c2i.reserve(recs_.size()); i2p.reserve(recs_.size());
    for (const auto& r : recs_) {
        if (r.pub >= r.cap) c2p.push_back(double(r.pub - r.cap) / 1e6);
        if (r.inf >= r.cap) c2i.push_back(double(r.inf - r.cap) / 1e6);
        if (r.pub >= r.inf) i2p.push_back(double(r.pub - r.inf) / 1e6);
    }

    if (recs_.size() >= 2) {
        uint64_t span = recs_.back().pub - recs_.front().pub;
        if (span > 0) out.fps = double(recs_.size() - 1) / (double(span) / 1e9);
    }

    std::sort(c2p.begin(), c2p.end());
    std::sort(c2i.begin(), c2i.end());
    std::sort(i2p.begin(), i2p.end());

    out.cap2pub_p50_ms = percentile(c2p, 0.50);
    out.cap2pub_p95_ms = percentile(c2p, 0.95);
    out.cap2pub_p99_ms = percentile(c2p, 0.99);
    if (!c2p.empty()) {
        double sum = 0; for (double v : c2p) sum += v;
        out.cap2pub_mean_ms = sum / double(c2p.size());
    }
    out.cap2inf_p50_ms = percentile(c2i, 0.50);
    out.inf2pub_p50_ms = percentile(i2p, 0.50);
    return out;
}

std::string PerfProbe::report() const {
    PerfStats s = stats();
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "PERF fps=%.2f cap2pub_ms_p50=%.3f p95=%.3f p99=%.3f mean=%.3f "
        "cap2inf_ms_p50=%.3f inf2pub_ms_p50=%.3f n=%zu",
        s.fps, s.cap2pub_p50_ms, s.cap2pub_p95_ms, s.cap2pub_p99_ms, s.cap2pub_mean_ms,
        s.cap2inf_p50_ms, s.inf2pub_p50_ms, s.n);
    return std::string(buf);
}

} // namespace nui
