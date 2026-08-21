// engine_selftest.cpp — off-target logic test for EngineCore (no hardware).
// Simulates an index-only hand and scripts pinch->release (TAP) and a
// stationary hold (DWELL_SELECT). Exits non-zero on failure.
#include <cstdio>
#include <cstring>
#include <vector>
#include "nui_events.h"
#include "engine/engine_core.hpp"
#include "engine/hand_shape.hpp"

static nui_hand_state_t make_hand(bool pinch) {
    nui_hand_state_t h;
    std::memset(&h, 0, sizeof(h));
    h.present = 1; h.handedness = 1; h.score = 0.99f;
    auto set = [&](int i, float x, float y) {
        h.lm[i].x = x; h.lm[i].y = y; h.lm[i].z = 0.0f; h.lm[i].visibility = 1.0f;
    };
    // Only the index finger extended; others folded near the wrist.
    set(nui::LM_WRIST,      0.50f, 0.90f);
    set(nui::LM_INDEX_MCP,  0.50f, 0.58f);
    set(nui::LM_INDEX_PIP,  0.50f, 0.55f);
    set(nui::LM_INDEX_TIP,  0.50f, 0.40f); // extended
    set(nui::LM_MIDDLE_MCP, 0.50f, 0.60f);
    set(nui::LM_MIDDLE_PIP, 0.50f, 0.62f);
    set(nui::LM_MIDDLE_TIP, 0.50f, 0.85f); // folded
    set(nui::LM_RING_PIP,   0.50f, 0.62f);
    set(nui::LM_RING_TIP,   0.50f, 0.85f); // folded
    set(nui::LM_PINKY_PIP,  0.50f, 0.62f);
    set(nui::LM_PINKY_TIP,  0.50f, 0.85f); // folded
    // Thumb tip: near index tip = pinch engaged; far = released.
    if (pinch) set(nui::LM_THUMB_TIP, 0.50f, 0.40f);
    else       set(nui::LM_THUMB_TIP, 0.50f, 0.75f);
    return h;
}

static int count(const std::vector<nui_gesture_event_t>& g, uint16_t type) {
    int n = 0; for (const auto& e : g) if (e.type == type) ++n; return n;
}

int main() {
    nui::EngineConfig cfg; // default (identity homography, placeholder thresholds)
    nui::EngineCore core(cfg);

    std::vector<nui_gesture_event_t> all;
    auto feed = [&](const nui_hand_state_t& h, double t) {
        nui::EngineOutput o = core.update(h, nullptr, t);
        for (auto& e : o.gestures) all.push_back(e);
    };

    // Sanity: index-only hand must report exactly 1 extended finger.
    nui_hand_state_t rel = make_hand(false);
    if (nui::extended_finger_count(rel) != 1) {
        std::fprintf(stderr, "FAIL: extended_finger_count=%d (expected 1)\n",
                     nui::extended_finger_count(rel));
        return 1;
    }

    // TAP: released -> pinch -> released, quick, stationary.
    feed(make_hand(false), 0.00);
    feed(make_hand(true),  0.05);
    feed(make_hand(false), 0.10);

    int taps = count(all, NUI_GESTURE_TAP);
    if (taps < 1) { std::fprintf(stderr, "FAIL: no TAP emitted\n"); return 1; }

    // DWELL: stationary, released, held past t_dwell (default 0.80s).
    for (double t = 0.20; t <= 1.20; t += 0.10)
        feed(make_hand(false), t);

    int dwells = count(all, NUI_GESTURE_DWELL_SELECT);
    if (dwells < 1) { std::fprintf(stderr, "FAIL: no DWELL_SELECT emitted\n"); return 1; }

    // No spurious BACK (fist) since one finger is extended.
    int backs = count(all, NUI_GESTURE_BACK);
    if (backs != 0) { std::fprintf(stderr, "FAIL: unexpected BACK x%d\n", backs); return 1; }

    std::fprintf(stderr, "OK: taps=%d dwells=%d backs=%d\n", taps, dwells, backs);
    return 0;
}
