// main.cpp — nui-engine service: receives HandState/ContactState from
// perception over UDS, runs EngineCore, sends PointerEvent/GestureEvent to ui.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <string>
#include <fstream>
#include <sys/select.h>

#include "nui_events.h"
#include "nui_protocol/uds.hpp"
#include "engine/engine_core.hpp"

namespace {
double now_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<double>(ts.tv_sec) + static_cast<double>(ts.tv_nsec) * 1e-9;
}
std::string env_or(const char* k, const char* d) {
    const char* v = std::getenv(k);
    return v ? std::string(v) : std::string(d);
}
bool load_homography(const std::string& path, nui::Homography& H) {
    std::ifstream f(path);
    if (!f) return false;
    for (int i = 0; i < 9; ++i)
        if (!(f >> H.h[i])) return false;
    return true;
}
} // namespace

int main() {
    const std::string perc_path = env_or("NUI_PERCEPTION_SOCK", "/tmp/nui_perception.sock");
    const std::string ui_path   = env_or("NUI_UI_SOCK",         "/tmp/nui_ui.sock");

    nui::EngineConfig cfg;
    const std::string calib = env_or("NUI_CALIB", "");
    if (!calib.empty()) {
        if (load_homography(calib, cfg.homography))
            std::fprintf(stderr, "engine: homography loaded from %s\n", calib.c_str());
        else
            std::fprintf(stderr, "engine: WARN homography load failed, using identity\n");
    }
    nui::EngineCore core(cfg);

    try {
        nui::UdsListener perc_l(perc_path);
        nui::UdsListener ui_l(ui_path);
        std::fprintf(stderr, "engine: waiting for perception on %s\n", perc_path.c_str());
        nui::UdsConn perc_c = perc_l.accept_one();
        std::fprintf(stderr, "engine: waiting for ui on %s\n", ui_path.c_str());
        nui::UdsConn ui_c = ui_l.accept_one();
        if (!perc_c.valid() || !ui_c.valid()) {
            std::fprintf(stderr, "engine: accept failed\n");
            return 1;
        }
        std::fprintf(stderr, "engine: connected\n");

        nui_contact_state_t shadow{};
        bool have_shadow = false;
        unsigned char payload[nui::kMaxPayload];
        nui_header_t hdr;
        uint64_t out_seq = 0;

        for (;;) {
            fd_set rf; FD_ZERO(&rf); FD_SET(perc_c.fd(), &rf);
            int r = ::select(perc_c.fd() + 1, &rf, nullptr, nullptr, nullptr);
            if (r < 0) { if (errno == EINTR) continue; break; }

            nui::RecvResult rr = perc_c.recv_msg_ex(hdr, payload);
            if (rr == nui::RecvResult::Closed) break;      // peer gone -> end session
            if (rr == nui::RecvResult::WouldBlock) continue;
            if (rr == nui::RecvResult::Error) {
                // Malformed frame. SEQPACKET preserves message boundaries, so a
                // bad datagram does not desync the stream: drop it and continue
                // (fail-open on the trusted local bus) rather than tearing down.
                std::fprintf(stderr, "engine: dropped malformed frame\n");
                continue;
            }

            if (hdr.msg_type == NUI_MSG_CONTACT_STATE &&
                hdr.payload_len == sizeof(nui_contact_state_t)) {
                std::memcpy(&shadow, payload, sizeof(shadow));
                have_shadow = true;
                continue;
            }
            if (hdr.msg_type == NUI_MSG_HAND_STATE &&
                hdr.payload_len == sizeof(nui_hand_state_t)) {
                nui_hand_state_t hand;
                std::memcpy(&hand, payload, sizeof(hand));
                double t = hdr.timestamp_ns ? static_cast<double>(hdr.timestamp_ns) * 1e-9
                                            : now_seconds();
                nui::EngineOutput o = core.update(hand, have_shadow ? &shadow : nullptr, t);
                if (o.has_pointer)
                    ui_c.send_msg(NUI_MSG_POINTER_EVENT, &o.pointer, sizeof(o.pointer),
                                  hdr.timestamp_ns, out_seq++, /*reliable=*/false);
                for (const auto& g : o.gestures)
                    ui_c.send_msg(NUI_MSG_GESTURE_EVENT, &g, sizeof(g),
                                  hdr.timestamp_ns, out_seq++, /*reliable=*/true);
            }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "engine: fatal: %s\n", e.what());
        return 1;
    }
    std::fprintf(stderr, "engine: exiting\n");
    return 0;
}
