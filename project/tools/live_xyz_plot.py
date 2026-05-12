#!/usr/bin/env python3

import argparse
import os
import re
import sys
import time
from collections import deque


SCREENLOG = "screenlog.0"

XYZ_RE = re.compile(
    rb"xyz\s*=\s*"
    rb"([-+]?\d+(?:\.\d*)?(?:[eE][-+]?\d+)?),\s*"
    rb"([-+]?\d+(?:\.\d*)?(?:[eE][-+]?\d+)?),\s*"
    rb"([-+]?\d+(?:\.\d*)?(?:[eE][-+]?\d+)?)"
)


class PointParser:
    def __init__(self):
        self.buffer = b""

    def feed(self, chunk):
        if not chunk:
            return []

        data = self.buffer + chunk
        points = []
        last_end = -1

        for match in XYZ_RE.finditer(data):
            points.append(tuple(float(match.group(i)) for i in range(1, 4)))
            last_end = match.end()

        if last_end >= 0:
            self.buffer = data[last_end:][-1024:]
        else:
            self.buffer = data[-1024:]

        return points


class ScreenlogTail:
    def __init__(self, path, from_start=False):
        self.path = path
        self.offset = 0 if from_start else None
        self.parser = PointParser()

    def poll(self):
        try:
            size = os.path.getsize(self.path)
        except OSError:
            return []

        if self.offset is None:
            self.offset = size
            return []

        if size < self.offset:
            self.offset = 0
            self.parser = PointParser()

        try:
            with open(self.path, "rb") as log:
                log.seek(self.offset)
                chunk = log.read()
                self.offset = log.tell()
        except OSError:
            return []

        return self.parser.feed(chunk)


def equalize_axes(ax, xs, ys, zs):
    mins = [min(xs), min(ys), min(zs)]
    maxs = [max(xs), max(ys), max(zs)]
    centers = [(lo + hi) * 0.5 for lo, hi in zip(mins, maxs)]
    radius = max(max(hi - lo for lo, hi in zip(mins, maxs)) * 0.5, 1.0)

    ax.set_xlim(centers[0] - radius, centers[0] + radius)
    ax.set_ylim(centers[1] - radius, centers[1] + radius)
    ax.set_zlim(centers[2] - radius, centers[2] + radius)


def main():
    parser = argparse.ArgumentParser(
        description="Plot live xyz runtime output from a GNU screen log."
    )
    parser.add_argument("--seconds", type=float, default=5.0)
    parser.add_argument("--interval-ms", type=int, default=100)
    args = parser.parse_args()

    try:
        import matplotlib.pyplot as plt
        from matplotlib.animation import FuncAnimation
    except ImportError:
        print("matplotlib is required: python3 -m pip install matplotlib",
              file=sys.stderr)
        return 1

    try:
        os.remove(SCREENLOG)
        print(f"deleted stale {SCREENLOG}")
    except FileNotFoundError:
        pass
    except OSError as exc:
        print(f"warning: could not delete {SCREENLOG}: {exc}", file=sys.stderr)

    tail = ScreenlogTail(SCREENLOG, from_start=True)
    source_label = SCREENLOG
    points = deque()

    fig = plt.figure("stereo live xyz")
    ax = fig.add_subplot(111, projection="3d")
    line, = ax.plot([], [], [], "-", linewidth=1.0)
    current = ax.scatter([], [], [], s=40)

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.set_title(f"tailing {source_label}")

    def update(_frame):
        nonlocal current

        now = time.monotonic()
        for point in tail.poll():
            points.append((now, point))

        cutoff = now - max(args.seconds, 0.1)
        while points and points[0][0] < cutoff:
            points.popleft()

        if not points:
            return line, current

        coords = [p for _, p in points]
        xs = [p[0] for p in coords]
        ys = [p[1] for p in coords]
        zs = [p[2] for p in coords]

        line.set_data(xs, ys)
        line.set_3d_properties(zs)
        current.remove()
        current = ax.scatter([xs[-1]], [ys[-1]], [zs[-1]], s=45)
        equalize_axes(ax, xs, ys, zs)
        ax.set_title(
            f"{source_label}  xyz=({xs[-1]:.3f}, {ys[-1]:.3f}, {zs[-1]:.3f})"
        )
        return line, current

    animation = FuncAnimation(
        fig, update, interval=args.interval_ms, cache_frame_data=False
    )
    fig._stereo_animation = animation
    plt.show()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
