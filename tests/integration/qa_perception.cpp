// qa_perception.cpp — synthetic perception producer for end-to-end QA.
//
// Stands in for the real perception service (whose Hailo pipeline is blocked)
// by sending scripted HandState/ContactState messages over the real UDS bus to
// nui-engine, using the real nui_protocol. Real CLOCK_MONOTONIC timestamps and
// real sleeps drive the engine's time-based FSM, and let the UI client measure
// end-to-end engine-stage latency.
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <chrono>

#include "nui_events.h"
#include "nui_protocol/uds.hpp"
#include "qa_hands.hpp"

using namespace std::chrono;

static uint64_t mono_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return uint64_t(ts.tv_sec) * 1000000000ull + uint64_t(ts.tv_nsec);
}

static std::string env_or(const char* k, const char* d) {
    const char* v = std::getenv(k);
    return v ? std::string(v) : std::string(d);
}

// Synthetic hand builders are shared with qa_malformed via qa_hands.hpp.
using qa::hand_pointer;
using qa::hand_open;
using qa::hand_fist;
using qa::hand_absent;

int main() {
    const std::string perc = env_or("NUI_PERCEPTION_SOCK", "/tmp/nui_perception.sock");

    // Retry connect until the engine is listening.
    nui::UdsConn c;
    for (int i = 0; i < 100 && !c.valid(); ++i) {
        c = nui::UdsConn::connect(perc);
        if (!c.valid()) std::this_thread::sleep_for(milliseconds(20));
    }
    if (!c.valid()) { std::fprintf(stderr, "qa_perception: cannot connect to %s\n", perc.c_str()); return 1; }
    std::fprintf(stderr, "qa_perception: connected\n");

    uint64_t seq = 0;
    auto send_hand = [&](const nui_hand_state_t& h) {
        c.send_msg(NUI_MSG_HAND_STATE, &h, sizeof(h), mono_ns(), seq++, /*reliable=*/true);
    };
    auto send_contact_lowconf = [&]() {
        nui_contact_state_t s{}; s.engaged = 0; s.method = NUI_CONTACT_SHADOW; s.confidence = 0.1f;
        c.send_msg(NUI_MSG_CONTACT_STATE, &s, sizeof(s), mono_ns(), seq++, true);
    };
    auto ms = [](int m){ std::this_thread::sleep_for(milliseconds(m)); };

    // Exercise the CONTACT_STATE path once (low confidence -> engine ignores it).
    send_contact_lowconf();

    // --- TAP: released -> pinch -> released, quick, stationary ---
    send_hand(hand_absent());                 ms(40);
    send_hand(hand_pointer(false, 0.5f, 0.4f)); ms(40);
    send_hand(hand_pointer(true,  0.5f, 0.4f)); ms(40);
    send_hand(hand_pointer(false, 0.5f, 0.4f)); ms(40);

    // --- DRAG: pinch, move > eps, release ---
    send_hand(hand_absent());                 ms(40);
    send_hand(hand_pointer(false, 0.5f, 0.4f)); ms(40);
    send_hand(hand_pointer(true,  0.5f, 0.4f)); ms(40);
    send_hand(hand_pointer(true,  0.7f, 0.4f)); ms(40);   // DRAG_BEGIN
    send_hand(hand_pointer(false, 0.7f, 0.4f)); ms(40);   // DRAG_END

    // --- SWIPE: released, fast directional move, then settle ---
    send_hand(hand_absent());                 ms(40);
    send_hand(hand_pointer(false, 0.2f, 0.5f)); ms(30);
    send_hand(hand_pointer(false, 0.8f, 0.5f)); ms(30);   // high speed -> Swipe
    // Trailing stationary frames: the low-beta 1e filter needs a few frames for
    // the filtered speed to decay below the emit threshold (swipe_speed*0.5).
    for (int i = 0; i < 6; ++i) { send_hand(hand_pointer(false, 0.82f, 0.5f)); ms(40); }

    // --- DWELL: stationary released, held past t_dwell (0.8s) ---
    send_hand(hand_absent());                 ms(40);
    send_hand(hand_pointer(false, 0.5f, 0.5f)); ms(50);
    for (int i = 0; i < 10; ++i) { send_hand(hand_pointer(false, 0.5f, 0.5f)); ms(95); } // ~0.95s

    // --- HOME: open palm held past t_home (0.8s) ---
    send_hand(hand_absent());                 ms(40);
    for (int i = 0; i < 11; ++i) { send_hand(hand_open()); ms(90); } // ~1.0s

    // --- BACK: fist held past t_back (0.25s) ---
    send_hand(hand_absent());                 ms(40);
    for (int i = 0; i < 6; ++i) { send_hand(hand_fist()); ms(90); } // ~0.5s

    send_hand(hand_absent());                 ms(40);
    std::fprintf(stderr, "qa_perception: script done (%llu msgs)\n", (unsigned long long)seq);
    return 0;
}
