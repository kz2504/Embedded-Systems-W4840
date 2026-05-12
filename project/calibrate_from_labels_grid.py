import argparse
import glob
import json
import os
import itertools
import cv2 as cv
import numpy as np

def load_image_size(paths):
    for p in paths:
        img = cv.imread(p, cv.IMREAD_GRAYSCALE)
        if img is not None:
            return img.shape[::-1]
    return None

def make_full_grid(cols, rows, square):
    pts = []
    for r in range(rows):
        for c in range(cols):
            pts.append([c * square, r * square, 0.0])
    return np.array(pts, dtype=np.float32)

def transform_grid_indices(cols, rows, mode):
    coords = []
    for r in range(rows):
        for c in range(cols):
            if mode == 0:
                cc, rr = c, r
            elif mode == 1:
                cc, rr = cols - 1 - c, r
            elif mode == 2:
                cc, rr = c, rows - 1 - r
            elif mode == 3:
                cc, rr = cols - 1 - c, rows - 1 - r
            elif mode == 4:
                cc, rr = r, c
            elif mode == 5:
                cc, rr = rows - 1 - r, c
            elif mode == 6:
                cc, rr = r, cols - 1 - c
            else:
                cc, rr = rows - 1 - r, cols - 1 - c
            coords.append((cc, rr))
    return coords

def object_points_for_missing(cols, rows, square, missing, mode):
    coords = transform_grid_indices(cols, rows, mode)

    pts = []
    for i, (c, r) in enumerate(coords):
        if i == missing:
            continue
        pts.append([c * square, r * square, 0.0])

    return np.array(pts, dtype=np.float32)

def sort_image_points_into_grid(points, cols, rows):
    pts = np.array(points, dtype=np.float32).reshape(-1, 2)

    if len(pts) != cols * rows - 1:
        return None

    mean = np.mean(pts, axis=0)
    centered = pts - mean

    _, _, vt = np.linalg.svd(centered, full_matrices=False)
    axes = [vt[0], vt[1]]

    best_order = None
    best_score = None

    for swap in [False, True]:
        au = axes[1] if swap else axes[0]
        av = axes[0] if swap else axes[1]

        uv = np.column_stack((centered @ au, centered @ av))

        for flip_u in [False, True]:
            for flip_v in [False, True]:
                u = -uv[:, 0] if flip_u else uv[:, 0]
                v = -uv[:, 1] if flip_v else uv[:, 1]

                u_min, u_max = np.min(u), np.max(u)
                v_min, v_max = np.min(v), np.max(v)

                ideal = []
                for r in range(rows):
                    for c in range(cols):
                        iu = u_min + (u_max - u_min) * c / max(cols - 1, 1)
                        iv = v_min + (v_max - v_min) * r / max(rows - 1, 1)
                        ideal.append([iu, iv])
                ideal = np.array(ideal, dtype=np.float32)

                assigned = {}
                score = 0.0

                projected = np.column_stack((u, v))

                for pi, p in enumerate(projected):
                    d = np.linalg.norm(ideal - p, axis=1)
                    for gi in np.argsort(d):
                        gi = int(gi)
                        if gi not in assigned:
                            assigned[gi] = pi
                            score += float(d[gi])
                            break

                if len(assigned) != len(pts):
                    continue

                missing = [i for i in range(cols * rows) if i not in assigned]
                if len(missing) != 1:
                    continue

                ordered = []
                for gi in range(cols * rows):
                    if gi in assigned:
                        ordered.append(pts[assigned[gi]])

                ordered = np.array(ordered, dtype=np.float32)

                if best_score is None or score < best_score:
                    best_score = score
                    best_order = (ordered, missing[0])

    return best_order

def calibrate_case(paths, labels, cols, rows, square, mode):
    objpoints = []
    imgpoints = []
    image_size = load_image_size(paths)

    if image_size is None:
        return None

    path_by_name = {os.path.basename(p): p for p in paths}

    missing_votes = []

    for name, raw_points in labels.items():
        if name not in path_by_name:
            continue

        result = sort_image_points_into_grid(raw_points, cols, rows)
        if result is None:
            continue

        ordered_img, missing = result
        missing_votes.append(missing)

        obj = object_points_for_missing(cols, rows, square, missing, mode)

        objpoints.append(obj.reshape(-1, 1, 3))
        imgpoints.append(ordered_img.reshape(-1, 1, 2))

    if len(objpoints) < 4:
        return None

    rms, K, dist, rvecs, tvecs = cv.calibrateCamera(
        objpoints, imgpoints, image_size, None, None
    )

    return rms, K, dist, len(objpoints), missing_votes

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--images", required=True)
    ap.add_argument("--labels", required=True)
    ap.add_argument("--board", nargs=2, type=int, required=True)
    ap.add_argument("--square", type=float, default=1.0)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    cols, rows = args.board

    paths = sorted(glob.glob(args.images))
    if not paths:
        raise SystemExit(f"No images matched: {args.images}")

    with open(args.labels, "r") as f:
        labels = json.load(f)

    best = None

    for mode in range(8):
        result = calibrate_case(paths, labels, cols, rows, args.square, mode)
        if result is None:
            continue

        rms, K, dist, used, missing_votes = result
        print(f"mode {mode}: RMS={rms:.4f}, used={used}, missing votes={missing_votes}")

        if best is None or rms < best[0]:
            best = (rms, K, dist, used, mode, missing_votes)

    if best is None:
        raise SystemExit("No valid calibration case found")

    rms, K, dist, used, mode, missing_votes = best
    image_size = load_image_size(paths)

    fs = cv.FileStorage(args.out, cv.FILE_STORAGE_WRITE)
    fs.write("image_width", image_size[0])
    fs.write("image_height", image_size[1])
    fs.write("board_cols", cols)
    fs.write("board_rows", rows)
    fs.write("spacing", args.square)
    fs.write("grid_mode", mode)
    fs.write("rms_reprojection_error", rms)
    fs.write("camera_matrix", K)
    fs.write("distortion_coefficients", dist)
    fs.write("used_images", used)
    fs.release()

    print("\nBEST")
    print("mode:", mode)
    print("used images:", used)
    print("missing votes:", missing_votes)
    print("RMS:", rms)
    print("K:\n", K)
    print("dist:", dist.ravel())
    print("wrote:", args.out)

if __name__ == "__main__":
    main()

