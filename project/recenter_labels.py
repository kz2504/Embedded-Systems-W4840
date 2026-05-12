import argparse
import glob
import json
import os
import cv2 as cv
import numpy as np

def snap_to_centroid(gray, x, y, radius):
    h, w = gray.shape

    x = int(round(x))
    y = int(round(y))

    x0 = max(0, x - radius)
    x1 = min(w, x + radius + 1)
    y0 = max(0, y - radius)
    y1 = min(h, y + radius + 1)

    roi = gray[y0:y1, x0:x1]
    if roi.size == 0:
        return float(x), float(y)

    blur = cv.GaussianBlur(roi, (5, 5), 0)
    max_val = int(np.max(blur))

    if max_val <= 0:
        return float(x), float(y)

    threshold_value = max(10, int(max_val * 0.55))
    _, th = cv.threshold(blur, threshold_value, 255, cv.THRESH_BINARY)

    contours, _ = cv.findContours(th, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE)
    if not contours:
        return float(x), float(y)

    local_click = np.array([x - x0, y - y0], dtype=np.float32)

    best = None
    best_dist = None

    for c in contours:
        area = cv.contourArea(c)
        if area < 3:
            continue

        m = cv.moments(c)
        if m["m00"] == 0:
            continue

        cx_local = m["m10"] / m["m00"]
        cy_local = m["m01"] / m["m00"]

        center = np.array([cx_local, cy_local], dtype=np.float32)
        dist = float(np.linalg.norm(center - local_click))

        if best is None or dist < best_dist:
            best = (cx_local, cy_local)
            best_dist = dist

    if best is None:
        return float(x), float(y)

    return float(x0 + best[0]), float(y0 + best[1])

def draw_preview(gray, points, out_path):
    img = cv.cvtColor(gray, cv.COLOR_GRAY2BGR)

    for i, (x, y) in enumerate(points):
        cv.circle(img, (int(round(x)), int(round(y))), 7, (0, 255, 0), 2)
        cv.putText(img, str(i), (int(x) + 8, int(y) - 8),
                   cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)

    cv.imwrite(out_path, img)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--images", required=True)
    ap.add_argument("--labels-in", required=True)
    ap.add_argument("--labels-out", required=True)
    ap.add_argument("--radius", type=int, default=24)
    ap.add_argument("--preview-dir", default="recenter_debug")
    args = ap.parse_args()

    with open(args.labels_in, "r") as f:
        labels = json.load(f)

    paths = sorted(glob.glob(args.images))
    by_name = {os.path.basename(p): p for p in paths}

    os.makedirs(args.preview_dir, exist_ok=True)

    new_labels = {}

    for name, points in labels.items():
        if name not in by_name:
            print(f"missing image for {name}, skipping")
            continue

        gray = cv.imread(by_name[name], cv.IMREAD_GRAYSCALE)
        if gray is None:
            print(f"unreadable image for {name}, skipping")
            continue

        snapped = []
        total_shift = 0.0

        for x, y in points:
            cx, cy = snap_to_centroid(gray, x, y, args.radius)
            snapped.append([cx, cy])
            total_shift += float(np.hypot(cx - x, cy - y))

        new_labels[name] = snapped

        preview_path = os.path.join(
            args.preview_dir,
            os.path.splitext(name)[0] + "_recentered.png"
        )
        draw_preview(gray, snapped, preview_path)

        avg_shift = total_shift / max(len(points), 1)
        print(f"{name}: recentered {len(points)} points, avg shift {avg_shift:.2f}px")

    with open(args.labels_out, "w") as f:
        json.dump(new_labels, f, indent=2)

    print(f"\nwrote {args.labels_out}")
    print(f"preview images in {args.preview_dir}")

if __name__ == "__main__":
    main()

