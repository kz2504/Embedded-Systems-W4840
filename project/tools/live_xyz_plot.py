#!/usr/bin/env python3

import os
import re
import sys
import time
from collections import deque

SCREENLOG = "screenlog.0"
PLOT_SECONDS = 10.0
UPDATE_INTERVAL_MS = 100
FIXED_RANGE = 100.0
AUTOSCALE = False

FLOAT_RE = rb"[-+]?(?:(?:\d+\.\d*|\.\d+)(?:[eE][-+]?\d+)?|\d+[eE][-+]?\d+)"
XYZ_RE = re.compile(
    rb"xyz\s*=\s*"
    rb"(" + FLOAT_RE + rb"),\s*"
    rb"(" + FLOAT_RE + rb"),\s*"
    rb"(" + FLOAT_RE + rb")"
)


class PointParser:
    def __init__(self):
        self.buffer = b""

    def feed(self, chunk):
        if not chunk:
            return []

        data = self.buffer + chunk
        points = []
        records = re.split(rb"[\r\n]", data)

        if data[-1:] in (b"\r", b"\n"):
            complete_records = records
            self.buffer = b""
        else:
            complete_records = records[:-1]
            self.buffer = records[-1][-1024:]

        for record in complete_records:
            match = XYZ_RE.search(record)
            if match:
                points.append(tuple(float(match.group(i)) for i in range(1, 4)))

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


def find_screenlog_path():
    if os.path.exists(SCREENLOG):
        return SCREENLOG

    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root_log = os.path.join(os.path.dirname(script_dir), SCREENLOG)
    if os.path.exists(repo_root_log):
        return repo_root_log

    return SCREENLOG


def equalize_axes(ax, xs, ys, zs):
    mins = [min(xs), min(ys), min(zs)]
    maxs = [max(xs), max(ys), max(zs)]
    centers = [(lo + hi) * 0.5 for lo, hi in zip(mins, maxs)]
    radius = max(max(hi - lo for lo, hi in zip(mins, maxs)) * 0.5, 1.0)

    ax.set_xlim(centers[0] - radius, centers[0] + radius)
    ax.set_ylim(centers[1] - radius, centers[1] + radius)
    ax.set_zlim(centers[2] - radius, centers[2] + radius)


def set_fixed_axes(ax, axis_range):
    radius = max(axis_range, 0.1)

    ax.set_xlim(-radius, radius)
    ax.set_ylim(-radius, radius)
    ax.set_zlim(-radius, radius)


def main():
    try:
        import matplotlib.pyplot as plt
        from matplotlib.animation import FuncAnimation
    except ImportError:
        print("matplotlib is required: python3 -m pip install matplotlib",
              file=sys.stderr)
        return 1

    log_path = find_screenlog_path()
    tail = ScreenlogTail(log_path, from_start=True)
    source_label = log_path
    points = deque()

    fig = plt.figure("stereo live xyz")
    ax = fig.add_subplot(111, projection="3d")
    line, = ax.plot([], [], [], "-", linewidth=1.0)
    current = ax.scatter([], [], [], s=40)

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    if not AUTOSCALE:
        set_fixed_axes(ax, FIXED_RANGE)
    ax.set_title(f"Visualization")

    def update(_frame):
        nonlocal current

        now = time.monotonic()
        for point in tail.poll():
            points.append((now, point))

        cutoff = now - max(PLOT_SECONDS, 0.1)
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
        if AUTOSCALE:
            equalize_axes(ax, xs, ys, zs)
        ax.set_title(
            f"{source_label}  xyz=({xs[-1]:.3f}, {ys[-1]:.3f}, {zs[-1]:.3f})"
        )
        return line, current

    animation = FuncAnimation(
        fig, update, interval=UPDATE_INTERVAL_MS, cache_frame_data=False
    )
    fig._stereo_animation = animation
    plt.show()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
