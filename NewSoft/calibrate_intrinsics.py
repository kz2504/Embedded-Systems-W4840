#!/usr/bin/env python3

import argparse
import glob
import json
from pathlib import Path

import cv2 as cv
import numpy as np


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--images", required=True, help="Image glob, like 'calib_A/*.pgm'")
    parser.add_argument("--board", nargs=2, type=int, default=[7, 6],
                        metavar=("COLS", "ROWS"),
                        help="internal chessboard corners, default 7 6")
    parser.add_argument("--square", type=float, default=1.0,
                        help="square size, any unit. Use mm if known")
    parser.add_argument("--out", required=True, help="output .npz file")
    return parser.parse_args()


def main():
    args = parse_args()
    board = tuple(args.board)

    paths = sorted(glob.glob(args.images))

    if not paths:
        raise SystemExit(f"No images found for: {args.images}")

    criteria = (
        cv.TERM_CRITERIA_EPS + cv.TERM_CRITERIA_MAX_ITER,
        30,
        0.001,
    )

    objp = np.zeros((board[0] * board[1], 3), np.float32)
    objp[:, :2] = np.mgrid[0:board[0], 0:board[1]].T.reshape(-1, 2)
    objp *= args.square

    objpoints = []
    imgpoints = []
    image_size = None

    for path in paths:
        img = cv.imread(path)

        if img is None:
            print(f"Skipping unreadable image: {path}")
            continue

        gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
        image_size = gray.shape[::-1]

        found, corners = cv.findChessboardCorners(gray, board, None)

        if not found:
            print(f"No chessboard found: {path}")
            continue

        refined = cv.cornerSubPix(
            gray,
            corners,
            (11, 11),
            (-1, -1),
            criteria,
        )

        objpoints.append(objp.copy())
        imgpoints.append(refined)

        print(f"Found chessboard: {path}")

    if len(objpoints) < 10:
        print(f"Warning: only {len(objpoints)} usable images. 10+ is better.")

    if not objpoints:
        raise SystemExit("No usable chessboard images found.")

    rms, camera_matrix, dist_coeffs, rvecs, tvecs = cv.calibrateCamera(
        objpoints,
        imgpoints,
        image_size,
        None,
        None,
    )

    np.savez(
        args.out,
        camera_matrix=camera_matrix,
        dist_coeffs=dist_coeffs,
        image_size=image_size,
        rms=rms,
    )

    summary = {
        "rms": float(rms),
        "image_size": list(image_size),
        "camera_matrix": camera_matrix.tolist(),
        "dist_coeffs": dist_coeffs.reshape(-1).tolist(),
        "usable_images": len(objpoints),
    }

    json_path = Path(args.out).with_suffix(".json")
    json_path.write_text(json.dumps(summary, indent=2))

    print("\nRMS error:")
    print(rms)

    print("\nCamera matrix:")
    print(camera_matrix)

    print("\nDistortion coefficients:")
    print(dist_coeffs.reshape(-1))

    print(f"\nSaved {args.out}")
    print(f"Saved {json_path}")


if __name__ == "__main__":
    main()

