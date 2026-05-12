import argparse
import csv
import re
import time
from pathlib import Path

import matplotlib.pyplot as plt

XYZ_RE = re.compile(
    r"xyz\s*=\s*([-+0-9.eE]+)\s*,\s*([-+0-9.eE]+)\s*,\s*([-+0-9.eE]+)"
)

def load_points(path):
    points = []
    text = Path(path).read_text(errors="ignore")

    # Reads lines like: xyz = 1.23, 4.56, 7.89
    for m in XYZ_RE.finditer(text):
        points.append((float(m.group(1)), float(m.group(2)), float(m.group(3))))

    if points:
        return points

    # Also supports CSV with x,y,z columns or plain x,y,z rows.
    with open(path, newline="") as f:
        sample = f.read(512)
        f.seek(0)

        has_header = "x" in sample.lower() and "y" in sample.lower() and "z" in sample.lower()
        if has_header:
            reader = csv.DictReader(f)
            for row in reader:
                points.append((float(row["x"]), float(row["y"]), float(row["z"])))
        else:
            reader = csv.reader(f)
            for row in reader:
                if len(row) >= 3:
                    points.append((float(row[0]), float(row[1]), float(row[2])))

    return points

def set_equal_axes(ax, xs, ys, zs):
    max_range = max(
        max(xs) - min(xs),
        max(ys) - min(ys),
        max(zs) - min(zs),
        1e-9,
    )

    cx = (max(xs) + min(xs)) / 2.0
    cy = (max(ys) + min(ys)) / 2.0
    cz = (max(zs) + min(zs)) / 2.0
    r = max_range / 2.0

    ax.set_xlim(cx - r, cx + r)
    ax.set_ylim(cy - r, cy + r)
    ax.set_zlim(cz - r, cz + r)

def plot_points(points, title):
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    zs = [p[2] for p in points]

    fig = plt.figure(figsize=(9, 7))
    ax = fig.add_subplot(111, projection="3d")

    ax.plot(xs, ys, zs, linewidth=1)
    ax.scatter(xs, ys, zs, s=35)

    ax.scatter([xs[-1]], [ys[-1]], [zs[-1]], s=120, marker="*", label="latest")

    ax.set_title(title)
    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.legend()

    set_equal_axes(ax, xs, ys, zs)
    plt.tight_layout()
    plt.show()

def live_plot(path, interval):
    plt.ion()
    fig = plt.figure(figsize=(9, 7))
    ax = fig.add_subplot(111, projection="3d")

    last_count = -1

    while True:
        points = load_points(path)

        if len(points) != last_count and points:
            last_count = len(points)

            xs = [p[0] for p in points]
            ys = [p[1] for p in points]
            zs = [p[2] for p in points]

            ax.clear()
            ax.plot(xs, ys, zs, linewidth=1)
            ax.scatter(xs, ys, zs, s=35)
            ax.scatter([xs[-1]], [ys[-1]], [zs[-1]], s=120, marker="*", label="latest")

            ax.set_title(f"3D stereo coordinates ({len(points)} points)")
            ax.set_xlabel("X")
            ax.set_ylabel("Y")
            ax.set_zlabel("Z")
            ax.legend()

            set_equal_axes(ax, xs, ys, zs)

            plt.draw()
            plt.pause(0.01)

        time.sleep(interval)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file", help="terminal log or CSV containing coordinates")
    ap.add_argument("--live", action="store_true", help="keep updating as file grows")
    ap.add_argument("--interval", type=float, default=0.5)
    args = ap.parse_args()

    if args.live:
        live_plot(args.file, args.interval)
    else:
        points = load_points(args.file)
        if not points:
            raise SystemExit("No coordinates found. Expected lines like: xyz = 1.0, 2.0, 3.0")
        plot_points(points, f"3D stereo coordinates ({len(points)} points)")

if __name__ == "__main__":
    main()

