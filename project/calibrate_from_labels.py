import argparse
import glob
import json
import os
import cv2 as cv
import numpy as np

def make_object_points(cols, rows, square, missing):
    pts = []
    for r in range(rows):
        for c in range(cols):
            idx = r * cols + c
            if idx == missing:
                continue
            pts.append([c * square, r * square, 0.0])
    return np.array(pts, dtype=np.float32)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--images", required=True)
    ap.add_argument("--labels", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--board", nargs=2, type=int, required=True)
    ap.add_argument("--square", type=float, default=1.0)
    ap.add_argument("--missing", type=int, required=True)
    args = ap.parse_args()

    cols, rows = args.board
    expected_points = cols * rows - 1

    objp = make_object_points(cols, rows, args.square, args.missing)
    if len(objp) != expected_points:
        raise SystemExit("object point count is wrong")

    with open(args.labels, "r") as f:
        labels = json.load(f)

    paths = sorted(glob.glob(args.images))
    if not paths:
        raise SystemExit(f"No images matched: {args.images}")

    objpoints = []
    imgpoints = []
    image_size = None

    for path in paths:
        name = os.path.basename(path)
        if name not in labels:
            continue

        gray = cv.imread(path, cv.IMREAD_GRAYSCALE)
        if gray is None:
            print(f"skip unreadable: {path}")
            continue

        pts = np.array(labels[name], dtype=np.float32)
        if len(pts) != expected_points:
            print(f"skip {name}: expected {expected_points}, got {len(pts)}")
            continue

        image_size = gray.shape[::-1]

        objpoints.append(objp.reshape(-1, 1, 3))
        imgpoints.append(pts.reshape(-1, 1, 2))

        print(f"used: {name}")

    if len(objpoints) < 4:
        raise SystemExit(f"Need at least 4 labeled images, got {len(objpoints)}")

    width, height = image_size
    K_init = np.array([
        [500.0, 0.0, width / 2.0],
        [0.0, 500.0, height / 2.0],
        [0.0, 0.0, 1.0]
    ], dtype=np.float64)

    dist_init = np.zeros((5, 1), dtype=np.float64)

    flags = (
        cv.CALIB_USE_INTRINSIC_GUESS |
        cv.CALIB_FIX_PRINCIPAL_POINT |
        cv.CALIB_ZERO_TANGENT_DIST |
        cv.CALIB_FIX_K3 |
        cv.CALIB_FIX_K4 |
        cv.CALIB_FIX_K5 |
        cv.CALIB_FIX_K6
    )

    rms, K, dist, rvecs, tvecs = cv.calibrateCamera(
        objpoints,
        imgpoints,
        image_size,
        K_init,
        dist_init,
        flags=flags
    )

    fs = cv.FileStorage(args.out, cv.FILE_STORAGE_WRITE)
    fs.write("image_width", image_size[0])
    fs.write("image_height", image_size[1])
    fs.write("board_cols", cols)
    fs.write("board_rows", rows)
    fs.write("missing_index", args.missing)
    fs.write("used_points_per_image", expected_points)
    fs.write("spacing", args.square)
    fs.write("rms_reprojection_error", rms)
    fs.write("camera_matrix", K)
    fs.write("distortion_coefficients", dist)
    fs.write("used_images", len(objpoints))
    fs.write("calibration_flags", int(flags))
    fs.release()

    print("\nCalibration complete")
    print("used images:", len(objpoints))
    print("RMS:", rms)
    print("K:\n", K)
    print("dist:", dist.ravel())
    print("wrote:", args.out)

if __name__ == "__main__":
    main()

