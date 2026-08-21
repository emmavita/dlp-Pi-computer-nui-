#!/usr/bin/env python3
"""test_calibration.py — verify the calibration tools on synthetic data.

1) Homography round-trip: build a known perspective mapping, feed corresponding
   points to compute_homography, and check it reproduces the mapping.
2) Chessboard detection: render a synthetic chessboard and confirm
   detect_chessboard finds the expected number of inner corners.
"""
import os
import sys

import numpy as np
import cv2

HERE = os.path.dirname(os.path.abspath(__file__))
CALIB = os.path.abspath(os.path.join(HERE, "..", "calibration"))
sys.path.insert(0, CALIB)

import compute_homography as ch  # noqa: E402
import detect_pattern as dp      # noqa: E402


def test_homography_roundtrip():
    # A known homography (camera-normalized -> surface-normalized).
    H_true = np.array([
        [1.05, 0.02, 0.01],
        [0.03, 0.98, 0.02],
        [0.10, 0.05, 1.00],
    ], dtype=np.float64)

    src = np.array([[0.1, 0.1], [0.9, 0.1], [0.9, 0.9], [0.1, 0.9],
                    [0.5, 0.5], [0.3, 0.7]], dtype=np.float64)
    # Apply H_true to get dst.
    dst = []
    for x, y in src:
        v = H_true @ np.array([x, y, 1.0])
        dst.append([v[0] / v[2], v[1] / v[2]])
    dst = np.array(dst)

    H = ch.compute_homography(src, dst)
    # Verify mapping agreement on a fresh point.
    p = np.array([0.42, 0.61, 1.0])
    a = H @ p
    b = H_true @ p
    a = a[:2] / a[2]
    b = b[:2] / b[2]
    err = np.linalg.norm(a - b)
    assert err < 1e-6, f"homography mismatch, err={err}"


def test_homography_save_format(tmp_path=None):
    import tempfile
    H = np.eye(3, dtype=np.float64)
    d = tempfile.mkdtemp()
    path = os.path.join(d, "m.matrix")
    ch.save_matrix(H, path)
    with open(path, encoding="utf-8") as f:
        vals = f.read().split()
    assert len(vals) == 9, f"expected 9 floats, got {len(vals)}"
    arr = np.array([float(v) for v in vals]).reshape(3, 3)
    assert np.allclose(arr, np.eye(3))


def _render_chessboard(inner_cols, inner_rows, square=40, margin=40):
    cols = inner_cols + 1
    rows = inner_rows + 1
    w = cols * square + 2 * margin
    h = rows * square + 2 * margin
    img = np.full((h, w), 255, dtype=np.uint8)
    for r in range(rows):
        for c in range(cols):
            if (r + c) % 2 == 0:
                y0 = margin + r * square
                x0 = margin + c * square
                img[y0:y0 + square, x0:x0 + square] = 0
    return img


def test_chessboard_detection():
    inner = (7, 5)  # (cols, rows) inner corners
    img = _render_chessboard(inner[0], inner[1])
    corners = dp.detect_chessboard(img, inner)
    assert corners is not None, "chessboard not detected"
    assert corners.shape == (inner[0] * inner[1], 2), f"got {corners.shape}"


if __name__ == "__main__":
    test_homography_roundtrip()
    test_homography_save_format()
    test_chessboard_detection()
    print("OK: homography round-trip, save format, chessboard detection")
