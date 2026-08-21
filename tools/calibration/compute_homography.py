#!/usr/bin/env python3
"""compute_homography.py — compute the camera->surface homography H.

Input: point correspondences between camera-normalized coordinates [0..1] and
projected-surface-normalized coordinates [0..1]. Output: the 3x3 matrix written
as 9 whitespace-separated row-major floats — exactly the format read by the
engine service (env NUI_CALIB, see services/engine/src/main.cpp).

Uses OpenCV cv2.findHomography (real API). Chosen over a hand-rolled DLT because
it is well-tested, supports optional RANSAC for outlier rejection, and is
already a project dependency (Phase 9 library list).
"""
import argparse
import json
import sys

import numpy as np
import cv2


def compute_homography(src_pts, dst_pts, ransac=False):
    """src_pts, dst_pts: (N,2) arrays. Returns a 3x3 float64 homography."""
    src = np.asarray(src_pts, dtype=np.float64).reshape(-1, 1, 2)
    dst = np.asarray(dst_pts, dtype=np.float64).reshape(-1, 1, 2)
    if src.shape[0] < 4:
        raise ValueError("need at least 4 correspondences")
    method = cv2.RANSAC if ransac else 0
    H, _mask = cv2.findHomography(src, dst, method)
    if H is None:
        raise RuntimeError("findHomography failed (degenerate correspondences?)")
    return H


def save_matrix(H, path):
    """Write 9 row-major floats, matching the engine's loader."""
    flat = np.asarray(H, dtype=np.float64).reshape(9)
    with open(path, "w", encoding="utf-8") as f:
        f.write(" ".join(repr(float(v)) for v in flat) + "\n")


def _load_correspondences(path):
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    return data["image_points"], data["surface_points"]


def main(argv=None):
    ap = argparse.ArgumentParser(description="Compute camera->surface homography")
    ap.add_argument("correspondences", help="JSON: {image_points:[[x,y]...], surface_points:[[u,v]...]}")
    ap.add_argument("-o", "--output", default="calibration.matrix",
                    help="output matrix file (9 floats)")
    ap.add_argument("--ransac", action="store_true", help="use RANSAC outlier rejection")
    args = ap.parse_args(argv)

    img, surf = _load_correspondences(args.correspondences)
    H = compute_homography(img, surf, ransac=args.ransac)
    save_matrix(H, args.output)
    print(f"wrote {args.output}")
    print(H)
    return 0


if __name__ == "__main__":
    sys.exit(main())
