#!/usr/bin/env python3
"""calibrate_pinch.py — derive the pinch engage/release thresholds from data.

The engine's ContactFusion engages a pinch when
    pinch_ratio = dist(thumb_tip, index_tip) / dist(wrist, middle_mcp)
falls below `pinch_on_ratio`, and releases above `pinch_off_ratio` (hysteresis).
This tool computes those thresholds from a LABELLED dataset of hand samples,
maximizing balanced accuracy, and derives a hysteresis margin from the class gap.

IMPORTANT (honesty): run this on REAL recorded hand samples captured on the
target. The values it emits are only as valid as the data fed in. On synthetic
data it validates the tool's logic, NOT the real device thresholds.

pinch_ratio geometry mirrors services/engine/include/engine/hand_shape.hpp.
"""
import argparse
import json
import math
import sys

LM_WRIST = 0
LM_THUMB_TIP = 4
LM_INDEX_TIP = 8
LM_MIDDLE_MCP = 9


def _dist(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def pinch_ratio_from_landmarks(lm):
    """lm: sequence of 21 (x, y). Matches hand_shape.hpp pinch_ratio()."""
    scale = _dist(lm[LM_WRIST], lm[LM_MIDDLE_MCP])
    if scale < 1e-4:
        scale = 1e-4
    return _dist(lm[LM_THUMB_TIP], lm[LM_INDEX_TIP]) / scale


def _balanced_accuracy(samples, thr):
    # pinch if ratio < thr
    tp = fn = tn = fp = 0
    for ratio, label in samples:
        pred_pinch = ratio < thr
        if label == "pinch":
            tp += pred_pinch
            fn += not pred_pinch
        else:
            tn += not pred_pinch
            fp += pred_pinch
    tpr = tp / (tp + fn) if (tp + fn) else 0.0
    tnr = tn / (tn + fp) if (tn + fp) else 0.0
    return 0.5 * (tpr + tnr)


def best_threshold(samples):
    """samples: list of (ratio, label in {'pinch','release'}).
    Returns dict with pinch_on_ratio, pinch_off_ratio, balanced_accuracy, margin."""
    if not samples:
        raise ValueError("empty dataset")
    ratios = sorted(set(r for r, _ in samples))
    # Candidate thresholds are midpoints between consecutive distinct ratios,
    # plus the extremes.
    cands = []
    for i in range(len(ratios) - 1):
        cands.append(0.5 * (ratios[i] + ratios[i + 1]))
    if not cands:
        cands = [ratios[0]]
    best = max(cands, key=lambda t: _balanced_accuracy(samples, t))
    acc = _balanced_accuracy(samples, best)

    pinch_ratios = [r for r, l in samples if l == "pinch"]
    release_ratios = [r for r, l in samples if l == "release"]
    # Separation gap between the classes (if separable).
    margin = 0.0
    if pinch_ratios and release_ratios:
        gap = min(release_ratios) - max(pinch_ratios)
        margin = max(0.0, gap)
    # Hysteresis: release threshold sits above engage by (up to) the gap; if the
    # classes overlap (margin 0) use a small default relative margin.
    pinch_on = best
    pinch_off = best + (margin if margin > 0 else 0.10 * best)
    return {
        "pinch_on_ratio": round(pinch_on, 4),
        "pinch_off_ratio": round(pinch_off, 4),
        "balanced_accuracy": round(acc, 4),
        "class_margin": round(margin, 4),
        "n_pinch": len(pinch_ratios),
        "n_release": len(release_ratios),
    }


def _load(path):
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    samples = []
    for row in data:
        label = row["label"]
        if "ratio" in row:
            samples.append((float(row["ratio"]), label))
        elif "landmarks" in row:
            samples.append((pinch_ratio_from_landmarks(row["landmarks"]), label))
        else:
            raise ValueError("each row needs 'ratio' or 'landmarks'")
    return samples


def main(argv=None):
    ap = argparse.ArgumentParser(description="Calibrate pinch thresholds from labelled data")
    ap.add_argument("dataset", help="JSON list of {ratio|landmarks, label:'pinch'|'release'}")
    args = ap.parse_args(argv)
    samples = _load(args.dataset)
    result = best_threshold(samples)
    print(json.dumps(result, indent=2))
    if result["balanced_accuracy"] < 0.9:
        print("WARNING: low separation; thresholds are unreliable — collect more/better data.",
              file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
