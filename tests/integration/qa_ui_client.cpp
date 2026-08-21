// qa_ui_client.cpp — QA stand-in for nui-ui. Connects to the engine's ui socket
// with the SAME nui_protocol as the real UI (validating the same contract),
// counts pointer/gesture events, and measures latency + throughput.
//
// LATENCY SEMANTICS (T11): latency = (ui receive time) - (header.timestamp_ns).
// Both are CLOCK_MONOTONIC on the same host, so the delta is valid. What the
// number MEANS depends on what perception stamps:
//   - if perception stamps the CAMERA CAPTURE time  -> true END-TO-END latency
//     (capture -> Hailo landmark -> engine -> ui receive);
//   - otherwise (e.g. the synthetic harness stamps at send) it is the downstream
//     (engine) stage latency only.
// The engine forwards the incoming timestamp unchanged, so the chain is already
// wired end-to-end; only perception's capture-time stamping (blocked, target
// side) is required to make this the full end-to-end figure.
//
// Env knobs:
//   NUI_UI_SOCK        socket path (default /tmp/nui_ui.sock)
//   NUI_QA_DURATION_S  measurement window seconds (default 7)
//   NUI_QA_WARMUP_S    ignore pointer samples in the first N seconds (default 0)
//   NUI_LAT_CSV        if set, write per-pointer CSV: recv_mono_ns,capture_ts_ns,latency_ms
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/select.h>

#include "nui_events.h"
#include "nui_protocol/uds.hpp"

static uint64_t mono_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
}
static std::string env_or(const char* k, const char* d) {
    const char* v = std::getenv(k);
    return v ? std::string(v) : std::string(d);
}
static double env_num(const char* k, double d) {
    const char* v = std::getenv(k);
    return v ? std::atof(v) : d;
}

// Linear-interpolation percentile on a sorted vector (p in [0,1]).
static double percentile(const std::vector<double>& s, double p) {
    if (s.empty()) return 0.0;
    if (s.size() == 1) return s[0];
    double rank = p * (double(s.size()) - 1.0);
    size_t lo = size_t(std::floor(rank));
    double frac = rank - double(lo);
    if (lo + 1 < s.size()) return s[lo] + frac * (s[lo + 1] - s[lo]);
    return s[lo];
}

int main() {
    const std::string ui = env_or("NUI_UI_SOCK", "/tmp/nui_ui.sock");
    const double duration_s = env_num("NUI_QA_DURATION_S", 7.0);
    const double warmup_s   = env_num("NUI_QA_WARMUP_S", 0.0);
    const std::string csv_path = env_or("NUI_LAT_CSV", "");

    nui::UdsConn c;
    for (int i = 0; i < 200 && !c.valid(); ++i) {
        c = nui::UdsConn::connect(ui);
        if (!c.valid()) { struct timespec t{0, 20000000}; nanosleep(&t, nullptr); }
    }
    if (!c.valid()) { std::fprintf(stderr, "qa_ui_client: cannot connect to %s\n", ui.c_str()); return 1; }
    std::fprintf(stderr, "qa_ui_client: connected (duration=%.1fs warmup=%.1fs)\n", duration_s, warmup_s);

    long counts[16] = {0};
    long pointers = 0;
    std::vector<double> lat_ms;
    uint64_t first_ptr_ns = 0, last_ptr_ns = 0;

    FILE* csv = nullptr;
    if (!csv_path.empty()) {
        csv = std::fopen(csv_path.c_str(), "w");
        if (csv) std::fprintf(csv, "recv_mono_ns,capture_ts_ns,latency_ms\n");
    }

    const uint64_t start_ns   = mono_ns();
    const uint64_t warmup_end = start_ns + uint64_t(warmup_s * 1e9);
    const uint64_t deadline   = start_ns + uint64_t(duration_s * 1e9);

    nui_header_t hdr;
    unsigned char payload[nui::kMaxPayload];

    for (;;) {
        uint64_t now = mono_ns();
        if (now >= deadline) break;
        uint64_t rem_ns = deadline - now;
        struct timeval tv;
        tv.tv_sec  = rem_ns / 1000000000ull;
        tv.tv_usec = (rem_ns % 1000000000ull) / 1000;
        fd_set rf; FD_ZERO(&rf); FD_SET(c.fd(), &rf);
        int r = ::select(c.fd() + 1, &rf, nullptr, nullptr, &tv);
        if (r <= 0) continue;

        for (;;) {
            nui::RecvResult rr = c.recv_msg_ex(hdr, payload);
            if (rr == nui::RecvResult::Ok) {
                uint64_t recv_ns = mono_ns();
                if (hdr.msg_type == NUI_MSG_POINTER_EVENT) {
                    ++pointers;
                    if (first_ptr_ns == 0) first_ptr_ns = recv_ns;
                    last_ptr_ns = recv_ns;
                    if (hdr.timestamp_ns && recv_ns >= warmup_end) {
                        double d = double(recv_ns - hdr.timestamp_ns) / 1e6;
                        if (d >= 0 && d < 10000) {
                            lat_ms.push_back(d);
                            if (csv) std::fprintf(csv, "%llu,%llu,%.6f\n",
                                (unsigned long long)recv_ns,
                                (unsigned long long)hdr.timestamp_ns, d);
                        }
                    }
                } else if (hdr.msg_type == NUI_MSG_GESTURE_EVENT &&
                           hdr.payload_len == sizeof(nui_gesture_event_t)) {
                    nui_gesture_event_t g; std::memcpy(&g, payload, sizeof(g));
                    if (g.type < 16) counts[g.type]++;
                }
                continue;
            }
            if (rr == nui::RecvResult::WouldBlock) break;
            std::fprintf(stderr, "qa_ui_client: peer closed/error\n");
            goto done;
        }
    }
done:
    if (csv) std::fclose(csv);

    // Latency statistics.
    double lmin = 0, lmed = 0, lmax = 0, lp90 = 0, lp95 = 0, lp99 = 0, lmean = 0, lstd = 0;
    if (!lat_ms.empty()) {
        std::sort(lat_ms.begin(), lat_ms.end());
        lmin = lat_ms.front();
        lmax = lat_ms.back();
        lmed = percentile(lat_ms, 0.50);
        lp90 = percentile(lat_ms, 0.90);
        lp95 = percentile(lat_ms, 0.95);
        lp99 = percentile(lat_ms, 0.99);
        double sum = 0; for (double v : lat_ms) sum += v;
        lmean = sum / double(lat_ms.size());
        double acc = 0; for (double v : lat_ms) acc += (v - lmean) * (v - lmean);
        lstd = std::sqrt(acc / double(lat_ms.size()));
    }

    // Throughput (effective UI update rate) over the observed pointer window.
    double fps = 0.0;
    if (pointers >= 2 && last_ptr_ns > first_ptr_ns)
        fps = double(pointers - 1) / (double(last_ptr_ns - first_ptr_ns) / 1e9);

    // SUMMARY line. Existing keys preserved (integration/malformed parsers rely on
    // pointers, TAP.., lat_ms_min/med/max); new keys appended.
    std::printf("SUMMARY pointers=%ld TAP=%ld LONG_PRESS=%ld DRAG_BEGIN=%ld DRAG_END=%ld "
                "SWIPE=%ld SCROLL=%ld ZOOM=%ld HOME=%ld BACK=%ld DWELL_SELECT=%ld "
                "lat_ms_min=%.3f lat_ms_med=%.3f lat_ms_max=%.3f "
                "lat_ms_mean=%.3f lat_ms_p90=%.3f lat_ms_p95=%.3f lat_ms_p99=%.3f lat_ms_std=%.3f "
                "fps=%.2f nlat=%zu\n",
                pointers,
                counts[NUI_GESTURE_TAP], counts[NUI_GESTURE_LONG_PRESS],
                counts[NUI_GESTURE_DRAG_BEGIN], counts[NUI_GESTURE_DRAG_END],
                counts[NUI_GESTURE_SWIPE], counts[NUI_GESTURE_SCROLL],
                counts[NUI_GESTURE_ZOOM], counts[NUI_GESTURE_HOME],
                counts[NUI_GESTURE_BACK], counts[NUI_GESTURE_DWELL_SELECT],
                lmin, lmed, lmax, lmean, lp90, lp95, lp99, lstd, fps, lat_ms.size());
    return 0;
}
