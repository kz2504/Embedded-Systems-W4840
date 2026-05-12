import argparse
import glob
import json
import os
import cv2 as cv

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--images", required=True)
    ap.add_argument("--labels", required=True)
    ap.add_argument("--outdir", required=True)
    args = ap.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    with open(args.labels, "r") as f:
        labels = json.load(f)

    paths = sorted(glob.glob(args.images))
    path_by_name = {os.path.basename(p): p for p in paths}

    for name, points in labels.items():
        if name not in path_by_name:
            print(f"missing image for {name}")
            continue

        gray = cv.imread(path_by_name[name], cv.IMREAD_GRAYSCALE)
        if gray is None:
            print(f"unreadable {name}")
            continue

        img = cv.cvtColor(gray, cv.COLOR_GRAY2BGR)

        for i, (x, y) in enumerate(points):
            x = int(round(x))
            y = int(round(y))
            cv.circle(img, (x, y), 7, (0, 255, 0), 2)
            cv.putText(img, str(i), (x + 8, y - 8),
                       cv.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)

        out = os.path.join(args.outdir, os.path.splitext(name)[0] + "_labels.png")
        cv.imwrite(out, img)
        print("wrote", out)

if __name__ == "__main__":
    main()

