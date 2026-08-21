// homography_map.hpp — apply a 3x3 homography (camera -> projected surface).
// H is produced offline by tools/calibration and loaded at startup.
#ifndef NUI_ENGINE_HOMOGRAPHY_MAP_HPP
#define NUI_ENGINE_HOMOGRAPHY_MAP_HPP

#include <array>
#include <algorithm>

namespace nui {

struct Homography {
    // Row-major 3x3. Identity by default (no-op until calibration is loaded).
    std::array<float, 9> h{{1, 0, 0, 0, 1, 0, 0, 0, 1}};

    // Map camera-normalized (x,y) to surface-normalized (u,v), clamped [0..1].
    void map(float x, float y, float& u, float& v) const {
        float dz = h[6] * x + h[7] * y + h[8];
        if (dz == 0.0f) dz = 1e-9f;
        u = (h[0] * x + h[1] * y + h[2]) / dz;
        v = (h[3] * x + h[4] * y + h[5]) / dz;
        u = std::min(1.0f, std::max(0.0f, u));
        v = std::min(1.0f, std::max(0.0f, v));
    }
};

} // namespace nui

#endif // NUI_ENGINE_HOMOGRAPHY_MAP_HPP
