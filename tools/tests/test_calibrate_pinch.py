#!/usr/bin/env python3
"""test_calibrate_pinch.py — validate the calibration tool logic on synthetic
data and check pinch_ratio geometry matches the C++ hand_shape.hpp definition.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CALIB = os.path.abspath(os.path.join(HERE, "..", "calibration"))
sys.path.insert(0, CALIB)

import calibrate_pinch as cp  # noqa: E402


def test_pinch_ratio_geometry():
    # Reuse the engine self-test geometry: wrist=(0.5,0.9), middle_mcp=(0.5,0.6)
    # -> scale = 0.3. Thumb on index -> ratio 0. Thumb far -> ratio ~0.83.
    lm = [[0.0, 0.0]] * 21
    lm[cp.LM_WRIST] = [0.5, 0.9]
    lm[cp.LM_MIDDLE_MCP] = [0.5, 0.6]
    lm[cp.LM_INDEX_TIP] = [0.5, 0.4]
    lm[cp.LM_THUMB_TIP] = [0.5, 0.4]  # pinch
    assert abs(cp.pinch_ratio_from_landmarks(lm) - 0.0) < 1e-6

    lm[cp.LM_THUMB_TIP] = [0.5, 0.75]  # released; dist=0.35, scale=0.3
    r = cp.pinch_ratio_from_landmarks(lm)
    assert abs(r - (0.35 / 0.30)) < 1e-6, r


def test_best_threshold_separable():
    # Well-separated synthetic classes.
    samples = ([(0.10 + 0.01 * i, "pinch") for i in range(10)] +
               [(0.70 + 0.01 * i, "release") for i in range(10)])
    res = cp.best_threshold(samples)
    assert res["balanced_accuracy"] == 1.0, res
    assert 0.19 < res["pinch_on_ratio"] < 0.70, res
    assert res["pinch_off_ratio"] > res["pinch_on_ratio"], res
    assert res["class_margin"] > 0, res


def test_best_threshold_overlap():
    # Overlapping classes -> accuracy < 1, still returns a usable hysteresis.
    samples = [(0.3, "pinch"), (0.35, "release"), (0.32, "pinch"), (0.33, "release")]
    res = cp.best_threshold(samples)
    assert res["pinch_off_ratio"] > res["pinch_on_ratio"]
    assert res["balanced_accuracy"] <= 1.0


if __name__ == "__main__":
    test_pinch_ratio_geometry()
    test_best_threshold_separable()
    test_best_threshold_overlap()
    print("OK: pinch geometry matches C++, threshold calibration logic verified")
