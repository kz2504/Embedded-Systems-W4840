#!/usr/bin/env python3
"""
Estimate stereo geometry from matched beacon centroids.

Input CSV must contain:
  xA,yA,xB,yB

Inputs from OpenCV intrinsic calibration:
  camera_A_intrinsics.npz
  camera_B_intrinsics.npz

Output:
  stereo_model.npz
  stereo_model.json

This implements:
  matched centroids -> normalize with K1/K2 -> essential matrix E
  -> recover pose R,t with chirality check -> projection matrices P1/P2
  -> triangulated 3D points up to unknown scale
"""

import argparse
import csv
import json
from pathlib import Path

import cv2 as cv
import numpy as np


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--matches", default="stereo_matches.csv",
                        help="CSV with xA,yA,xB,yB columns")
    parser.add_argument("--intrinsics-a", default="camera_A_intrinsics.npz",
                        help="Camera A intrinsics .npz")
    parser.add_argument("--intrinsics-b", default="camera_B_intrinsics.npz",
                        help="Camera B intrinsics .npz")
    parser.add_argument("--out", default="stereo_model.npz",
                        help="Output stereo model .npz")
    parser.add_argument("--min-points", type=int, default=9,
                        help="Minimum matches. Must be at least 9.")
    return parser.parse_args()


def load_intrinsics(path):
    data = np.load(path)
    K = data["camera_matrix"].astype(np.float64)
    dist = data["dist_coeffs"].astype(np.float64)
    return K, dist


def load_matches(path):
    pts_a = []
    pts_b = []

    with open(path, newline="") as f:
        reader = csv.DictReader(f)

        for row in reader:
            try:
                xa = float(row["xA"])
                ya = float(row["yA"])
                xb = float(row["xB"])
                yb = float(row["yB"])
            except (KeyError, ValueError):
                continue

            pts_a.append([xa, ya])
            pts_b.append([xb, yb])

    return np.asarray(pts_a, dtype=np.float64), np.asarray(pts_b, dtype=np.float64)


def main():
    args = parse_args()

    if args.min_points < 9:
        raise SystemExit("--min-points must be at least 9")

    K1, dist1 = load_intrinsics(args.intrinsics_a)
    K2, dist2 = load_intrinsics(args.intrinsics_b)

    pts1, pts2 = load_matches(args.matches)

    if len(pts1) < args.min_points:
        raise SystemExit(
            f"Need at least {args.min_points} matched points, found {len(pts1)}"
        )

    # Normalize image points using each camera's intrinsics and distortion.
    # Output coordinates are normalized camera coordinates.
    norm1 = cv.undistortPoints(
        pts1.reshape(-1, 1, 2),
        K1,
        dist1,
    ).reshape(-1, 2)

    norm2 = cv.undistortPoints(
        pts2.reshape(-1, 1, 2),
        K2,
        dist2,
    ).reshape(-1, 2)

    # Estimate E from normalized coordinates.
    E, inlier_mask = cv.findEssentialMat(
        norm1,
        norm2,
        focal=1.0,
        pp=(0.0, 0.0),
        method=cv.RANSAC,
        prob=0.999,
        threshold=0.001,
    )

    if E is None:
        raise SystemExit("Could not estimate essential matrix E")

    # OpenCV can return multiple stacked 3x3 candidates. Use the first.
    if E.shape != (3, 3):
        E = E[:3, :3]

    # Decompose E and apply chirality check.
    inliers_pose, R, t, pose_mask = cv.recoverPose(
        E,
        norm1,
        norm2,
        focal=1.0,
        pp=(0.0, 0.0),
    )

    # Projection matrices in normalized coordinates.
    P1 = np.hstack([np.eye(3), np.zeros((3, 1))])
    P2 = np.hstack([R, t])

    # Triangulate all matched points. Scale is unknown unless a real-world
    # distance is later used to scale t.
    hom = cv.triangulatePoints(P1, P2, norm1.T, norm2.T)
    points_3d = (hom[:3, :] / hom[3:4, :]).T

    np.savez(
        args.out,
        E=E,
        R=R,
        t=t,
        P1=P1,
        P2=P2,
        points_3d=points_3d,
        K1=K1,
        K2=K2,
        dist1=dist1,
        dist2=dist2,
        inlier_mask=inlier_mask,
        pose_mask=pose_mask,
    )

    summary = {
        "matches": int(len(pts1)),
        "essential_inliers": int(np.count_nonzero(inlier_mask))
        if inlier_mask is not None else None,
        "recover_pose_inliers": int(inliers_pose),
        "E": E.tolist(),
        "R": R.tolist(),
        "t_direction_scale_unknown": t.reshape(-1).tolist(),
        "P1": P1.tolist(),
        "P2": P2.tolist(),
        "first_10_points_3d_scale_unknown": points_3d[:10].tolist(),
    }

    json_path = Path(args.out).with_suffix(".json")
    json_path.write_text(json.dumps(summary, indent=2))

    print("Estimated essential matrix E:")
    print(E)

    print("\nRecovered rotation R:")
    print(R)

    print("\nRecovered translation direction t, scale unknown:")
    print(t.reshape(-1))

    print("\nProjection matrix P1:")
    print(P1)

    print("\nProjection matrix P2:")
    print(P2)

    print(f"\nMatches used: {len(pts1)}")
    print(f"recoverPose inliers: {inliers_pose}")
    print(f"Saved {args.out}")
    print(f"Saved {json_path}")


if __name__ == "__main__":
    main()

