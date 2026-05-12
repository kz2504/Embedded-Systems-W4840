import argparse
import glob
import cv2 as cv
import numpy as np

def make_bright_blob_detector():
    params = cv.SimpleBlobDetector_Params()

    params.filterByColor = True
    params.blobColor = 255

    params.filterByArea = True
    params.minArea = 20
    params.maxArea = 8000

    params.filterByCircularity = False
    params.filterByConvexity = False
    params.filterByInertia = False

    return cv.SimpleBlobDetector_create(params)

def preprocess(gray):
    work = cv.equalizeHist(gray)
    work = cv.GaussianBlur(work, (5, 5), 0)
    _, thresh = cv.threshold(work, 0, 255, cv.THRESH_BINARY + cv.THRESH_OTSU)
    return thresh

def order_points_grid(points, cols, rows):
    pts = np.array(points, dtype=np.float32).reshape(-1, 2)

    # Sort into rows using y coordinate, then x within each row.
    idx = np.argsort(pts[:, 1])
    pts = pts[idx]

    ordered = []
    for r in range(rows):
        row = pts[r * cols:(r + 1) * cols]
        row = row[np.argsort(row[:, 0])]
        ordered.append(row)

    return np.vstack(ordered).astype(np.float32)

def infer_missing_grid_point(points, cols, rows):
    pts = np.array(points, dtype=np.float32).reshape(-1, 2)

    # Use PCA to get grid directions even when board is tilted.
    mean = np.mean(pts, axis=0)
    centered = pts - mean
    _, _, vt = np.linalg.svd(centered, full_matrices=False)
    axis_u = vt[0]
    axis_v = vt[1]

    uv = np.column_stack((centered @ axis_u, centered @ axis_v))

    # Make sure u is the longer grid direction for 5x4 boards.
    if cols >= rows:
        if np.ptp(uv[:, 0]) < np.ptp(uv[:, 1]):
            axis_u, axis_v = axis_v, axis_u
            uv = np.column_stack((centered @ axis_u, centered @ axis_v))

    u_min, u_max = np.min(uv[:, 0]), np.max(uv[:, 0])
    v_min, v_max = np.min(uv[:, 1]), np.max(uv[:, 1])

    best = None

    # Try both row directions because the board can be rotated/flipped.
    for flip_v in [False, True]:
        grid = []
        for r in range(rows):
            for c in range(cols):
                u = u_min + (u_max - u_min) * c / max(cols - 1, 1)
                if not flip_v:
                    v = v_min + (v_max - v_min) * r / max(rows - 1, 1)
                else:
                    v = v_max - (v_max - v_min) * r / max(rows - 1, 1)
                xy = mean + u * axis_u + v * axis_v
                grid.append(xy)

        grid = np.array(grid, dtype=np.float32)

        dists = []
        used = set()
        total = 0.0

        for p in pts:
            ds = np.linalg.norm(grid - p, axis=1)
            for j in np.argsort(ds):
                if int(j) not in used:
                    used.add(int(j))
                    total += float(ds[j])
                    break

        if best is None or total < best[0]:
            best = (total, grid, used)

    _, grid, used = best
    missing = [i for i in range(cols * rows) if i not in used]

    if len(missing) != 1:
        return None

    complete = []
    for i in range(cols * rows):
        complete.append(grid[i])

    # Replace the estimated grid positions with actual detected points where possible.
    used_grid = {}
    for p in pts:
        ds = np.linalg.norm(grid - p, axis=1)
        for j in np.argsort(ds):
            j = int(j)
            if j not in used_grid:
                used_grid[j] = p
                break

    for j, p in used_grid.items():
        complete[j] = p

    return np.array(complete, dtype=np.float32)

def detect_circle_points(gray, cols, rows, detector):
    thresh = preprocess(gray)
    keypoints = detector.detect(thresh)
    points = np.array([kp.pt for kp in keypoints], dtype=np.float32)

    expected = cols * rows

    if len(points) == expected:
        return order_points_grid(points, cols, rows), "detected all"

    if len(points) == expected - 1:
        complete = infer_missing_grid_point(points, cols, rows)
        if complete is not None:
            return complete, "inferred missing one"

    return None, f"detected {len(points)} dots"

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--images", required=True)
    ap.add_argument("--board", nargs=2, type=int, required=True)
    ap.add_argument("--square", type=float, default=1.0)
    ap.add_argument("--pattern", choices=["circles", "chessboard"], default="circles")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    cols, rows = args.board
    pattern_size = (cols, rows)
    expected = cols * rows

    objp = np.zeros((expected, 3), np.float32)
    objp[:, :2] = np.mgrid[0:cols, 0:rows].T.reshape(-1, 2)
    objp *= args.square

    objpoints = []
    imgpoints = []
    image_size = None

    paths = sorted(glob.glob(args.images))
    if not paths:
        raise SystemExit(f"No images matched: {args.images}")

    detector = make_bright_blob_detector()

    for path in paths:
        gray = cv.imread(path, cv.IMREAD_GRAYSCALE)
        if gray is None:
            print(f"skip unreadable: {path}")
            continue

        image_size = gray.shape[::-1]

        if args.pattern == "circles":
            points, status = detect_circle_points(gray, cols, rows, detector)
            found = points is not None
            if found:
                points = points.reshape(-1, 1, 2).astype(np.float32)
        else:
            work = cv.equalizeHist(gray)
            found, points = cv.findChessboardCorners(
                work,
                pattern_size,
                flags=cv.CALIB_CB_ADAPTIVE_THRESH + cv.CALIB_CB_NORMALIZE_IMAGE
            )
            if found:
                points = cv.cornerSubPix(
                    work, points, (11, 11), (-1, -1),
                    (cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER, 30, 0.001)
                )
            status = "chessboard"

        if found:
            objpoints.append(objp)
            imgpoints.append(points)
            print(f"used: {path} ({status})")
        else:
            print(f"pattern not found: {path} ({status})")

    if len(objpoints) < 5:
        raise SystemExit(f"Need at least 5 usable images, got {len(objpoints)}")

    rms, K, dist, rvecs, tvecs = cv.calibrateCamera(
        objpoints, imgpoints, image_size, None, None
    )

    fs = cv.FileStorage(args.out, cv.FILE_STORAGE_WRITE)
    fs.write("image_width", image_size[0])
    fs.write("image_height", image_size[1])
    fs.write("pattern", args.pattern)
    fs.write("board_cols", cols)
    fs.write("board_rows", rows)
    fs.write("spacing", args.square)
    fs.write("rms_reprojection_error", rms)
    fs.write("camera_matrix", K)
    fs.write("distortion_coefficients", dist)
    fs.write("used_images", len(objpoints))
    fs.release()

    print("\nCalibration complete")
    print("used images:", len(objpoints))
    print("RMS:", rms)
    print("K:\n", K)
    print("dist:", dist.ravel())
    print("wrote:", args.out)

if __name__ == "__main__":
    main()


