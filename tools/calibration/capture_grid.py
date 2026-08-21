#!/usr/bin/env python3
"""capture_grid.py — capture still frames from the AI Camera for calibration.

TARGET-ONLY: requires Picamera2 and the physical Raspberry Pi AI Camera; it
cannot run on a development host without the camera stack. The IMX500 is used
here as a plain Bayer/RGB sensor (no on-sensor network loaded), consistent with
the Phase 3 decision IA-1.

Picamera2 API used (official manual / raspberrypi/picamera2):
  Picamera2(), create_still_configuration(main={...}), configure(),
  start(), capture_file(), capture_array(), stop().
Reference: https://datasheets.raspberrypi.com/camera/picamera2-manual.pdf
"""
import argparse
import sys
import time


def main(argv=None):
    ap = argparse.ArgumentParser(description="Capture calibration stills (AI Camera)")
    ap.add_argument("-n", "--count", type=int, default=1, help="number of frames")
    ap.add_argument("-o", "--outdir", default=".", help="output directory")
    ap.add_argument("--width", type=int, default=1280)
    ap.add_argument("--height", type=int, default=720)
    ap.add_argument("--interval", type=float, default=1.0, help="seconds between frames")
    args = ap.parse_args(argv)

    try:
        from picamera2 import Picamera2
    except ImportError:
        print("Picamera2 not available: run this on the target with the AI Camera.",
              file=sys.stderr)
        return 1

    picam2 = Picamera2()
    # BGR888 so cv2 (BGR) can read the arrays directly if needed.
    config = picam2.create_still_configuration(
        main={"size": (args.width, args.height), "format": "BGR888"})
    picam2.configure(config)
    picam2.start()
    try:
        time.sleep(1.0)  # let AE/AWB settle
        for i in range(args.count):
            path = f"{args.outdir}/calib_{i:03d}.png"
            picam2.capture_file(path)
            print(f"captured {path}")
            if i + 1 < args.count:
                time.sleep(args.interval)
    finally:
        picam2.stop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
