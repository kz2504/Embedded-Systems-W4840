import argparse
import glob
import json
import os
import cv2 as cv
import numpy as np

clicked = []
display = None
base = None
gray_image = None
snap_radius = 18

def snap_to_centroid(gray, x, y, radius):
    h, w = gray.shape

    x0 = max(0, x - radius)
    x1 = min(w, x + radius + 1)
    y0 = max(0, y - radius)
    y1 = min(h, y + radius + 1)

    roi = gray[y0:y1, x0:x1]
    if roi.size == 0:
        return float(x), float(y)

    blur = cv.GaussianBlur(roi, (5, 5), 0)
    _, th = cv.threshold(blur, 0, 255, cv.THRESH_BINARY + cv.THRESH_OTSU)

    m = cv.moments(th)
    if m["m00"] == 0:
        return float(x), float(y)

    cx = x0 + (m["m10"] / m["m00"])
    cy = y0 + (m["m01"] / m["m00"])

    return float(cx), float(cy)

def redraw():
    global display
    display = base.copy()

    for i, (x, y) in enumerate(clicked):
        cv.circle(display, (int(round(x)), int(round(y))), 7, (0, 255, 0), 2)
        cv.putText(display, str(i), (int(x) + 8, int(y) - 8),
                   cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)

    cv.imshow("label points", display)

def mouse_cb(event, x, y, flags, userdata):
    if event == cv.EVENT_LBUTTONDOWN:
        cx, cy = snap_to_centroid(gray_image, x, y, snap_radius)
        clicked.append([cx, cy])
        redraw()

def main():
    global clicked, base, gray_image, snap_radius

    ap = argparse.ArgumentParser()
    ap.add_argument("--images", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--points", type=int, default=19)
    ap.add_argument("--snap-radius", type=int, default=18)
    args = ap.parse_args()

    snap_radius = args.snap_radius

    paths = sorted(glob.glob(args.images))
    if not paths:
        raise SystemExit(f"No images matched: {args.images}")

    labels = {}

    print("\nControls:")
    print("  left click = add point, snapped to bright centroid")
    print("  u = undo")
    print("  s = save image labels")
    print("  n = skip image")
    print("  q = quit")
    print("\nClick near each dot in the same physical order every image.")
    print("The saved point becomes the local bright-pixel centroid.\n")

    cv.namedWindow("label points", cv.WINDOW_NORMAL)
    cv.setMouseCallback("label points", mouse_cb)

    for path in paths:
        gray = cv.imread(path, cv.IMREAD_GRAYSCALE)
        if gray is None:
            print(f"skip unreadable: {path}")
            continue

        gray_image = gray
        base = cv.cvtColor(gray, cv.COLOR_GRAY2BGR)
        clicked = []
        redraw()

        print(f"\nImage: {path}")
        print(f"Need {args.points} points")

        while True:
            key = cv.waitKey(0) & 0xff

            if key == ord("u"):
                if clicked:
                    clicked.pop()
                redraw()

            elif key == ord("s"):
                if len(clicked) != args.points:
                    print(f"Need exactly {args.points} points, currently {len(clicked)}")
                    continue
                labels[os.path.basename(path)] = clicked.copy()
                print(f"saved {len(clicked)} centroid points for {path}")
                break

            elif key == ord("n"):
                print(f"skipped {path}")
                break

            elif key == ord("q"):
                with open(args.out, "w") as f:
                    json.dump(labels, f, indent=2)
                cv.destroyAllWindows()
                print(f"wrote {args.out}")
                return

    with open(args.out, "w") as f:
        json.dump(labels, f, indent=2)

    cv.destroyAllWindows()
    print(f"\nwrote {args.out}")

if __name__ == "__main__":
    main()

