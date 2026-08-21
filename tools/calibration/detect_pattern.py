#!/usr/bin/env python3
"""detect_pattern.py — find a projected chessboard's inner corners.

Given a camera image of a projected chessboard calibration pattern, returns the
detected inner-corner pixel coordinates. These, paired with the known pattern
geometry on the projected surface, feed compute_homography.py.

Uses cv2.findChessboardCorners + cv2.cornerSubPix (real OpenCV APIs).
"""
import argparse
import sys

import numpy as np
import cv2


def detect_chessboard(gray, pattern_size):
    """gray: 8-bit single-channel image. pattern_size: (cols, rows) inner corners.
    Returns (N,2) float32 corners refined to sub-pixel, or None if not found."""
    if gray.ndim != 2:
        raise ValueError("expected a single-channel (grayscale) image")
    flags = cv2.CALIB_CB_ADAPTIVE_THRESH | cv2.CALIB_CB_NORMALIZE_IMAGE
    found, corners = cv2.findChessboardCorners(gray, pattern_size, flags)
    if not found:
        return None
    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
    corners = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
    return corners.reshape(-1, 2)


def surface_grid(pattern_size):
    """Return the corresponding inner-corner positions on the surface, normalized
    to [0..1]x[0..1] (row-major, matching findChessboardCorners ordering)."""
    cols, rows = pattern_size
    pts = np.zeros((cols * rows, 2), dtype=np.float64)
    idx = 0
    for r in range(rows):
        for c in range(cols):
            pts[idx, 0] = (c + 1) / (cols + 1)
            pts[idx, 1] = (r + 1) / (rows + 1)
            idx += 1
    return pts


def main(argv=None):
    ap = argparse.ArgumentParser(description="Detect chessboard corners in an image")
    ap.add_argument("image", help="path to a camera image (PNG/JPG)")
    ap.add_argument("--cols", type=int, required=True, help="inner corners per row")
    ap.add_argument("--rows", type=int, required=True, help="inner corners per column")
    args = ap.parse_args(argv)

    img = cv2.imread(args.image, cv2.IMREAD_GRAYSCALE)
    if img is None:
        print(f"cannot read {args.image}", file=sys.stderr)
        return 1
    corners = detect_chessboard(img, (args.cols, args.rows))
    if corners is None:
        print("chessboard not found", file=sys.stderr)
        return 2
    for x, y in corners:
        print(f"{x:.3f} {y:.3f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
