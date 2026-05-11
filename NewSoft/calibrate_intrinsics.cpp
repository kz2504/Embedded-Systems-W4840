#include <opencv2/opencv.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static void usage(const char *prog)
{
    std::cerr << "usage:\n";
    std::cerr << "  " << prog
              << " image_dir output.yml circles cols rows spacing\n";
    std::cerr << "  " << prog
              << " image_dir output.yml chessboard cols rows square_size\n\n";
    std::cerr << "examples:\n";
    std::cerr << "  " << prog
              << " calib_A camera_A_intrinsics.yml circles 4 5 1.0\n";
    std::cerr << "  " << prog
              << " calib_A camera_A_intrinsics.yml chessboard 7 6 1.0\n";
}

static bool is_circles(const std::string &pattern)
{
    return pattern == "circles" || pattern == "circle" || pattern == "dots";
}

static bool is_chessboard(const std::string &pattern)
{
    return pattern == "chessboard" || pattern == "chess" || pattern == "board";
}

int main(int argc, char **argv)
{
    if (argc != 7) {
        usage(argv[0]);
        return 2;
    }

    std::string image_dir = argv[1];
    std::string output_path = argv[2];
    std::string pattern = argv[3];
    int board_cols = std::atoi(argv[4]);
    int board_rows = std::atoi(argv[5]);
    float spacing = std::atof(argv[6]);

    if (board_cols <= 0 || board_rows <= 0 || spacing <= 0.0f) {
        usage(argv[0]);
        return 2;
    }

    bool use_circles = is_circles(pattern);
    bool use_chessboard = is_chessboard(pattern);

    if (!use_circles && !use_chessboard) {
        usage(argv[0]);
        return 2;
    }

    cv::Size board_size(board_cols, board_rows);

    std::vector<cv::String> image_paths;
    cv::glob(image_dir + "/*.pgm", image_paths, false);

    if (image_paths.empty()) {
        cv::glob(image_dir + "/*.jpg", image_paths, false);
    }
    if (image_paths.empty()) {
        cv::glob(image_dir + "/*.png", image_paths, false);
    }

    if (image_paths.empty()) {
        std::cerr << "No calibration images found in " << image_dir << "\n";
        return 1;
    }

    std::vector<cv::Point3f> one_board_points;
    for (int y = 0; y < board_rows; y++) {
        for (int x = 0; x < board_cols; x++) {
            one_board_points.push_back(
                cv::Point3f(x * spacing, y * spacing, 0.0f));
        }
    }

    std::vector<std::vector<cv::Point3f> > object_points;
    std::vector<std::vector<cv::Point2f> > image_points;
    cv::Size image_size;

    for (size_t i = 0; i < image_paths.size(); i++) {
        cv::Mat gray = cv::imread(image_paths[i], cv::IMREAD_GRAYSCALE);
        if (gray.empty()) {
            std::cerr << "Skipping unreadable image: " << image_paths[i] << "\n";
            continue;
        }

        image_size = gray.size();

        cv::Mat work;
        cv::equalizeHist(gray, work);
        cv::GaussianBlur(work, work, cv::Size(5, 5), 0);

        std::vector<cv::Point2f> points;
        bool found = false;

        if (use_circles) {
            found = cv::findCirclesGrid(
                work,
                board_size,
                points,
                cv::CALIB_CB_SYMMETRIC_GRID);
        } else {
            found = cv::findChessboardCorners(
                work,
                board_size,
                points,
                cv::CALIB_CB_ADAPTIVE_THRESH |
                cv::CALIB_CB_NORMALIZE_IMAGE);

            if (found) {
                cv::cornerSubPix(
                    work,
                    points,
                    cv::Size(11, 11),
                    cv::Size(-1, -1),
                    cv::TermCriteria(cv::TermCriteria::EPS +
                                     cv::TermCriteria::MAX_ITER,
                                     30, 0.001));
            }
        }

        if (!found) {
            std::cout << "pattern not found: " << image_paths[i] << "\n";
            continue;
        }

        image_points.push_back(points);
        object_points.push_back(one_board_points);

        std::cout << "used: " << image_paths[i] << "\n";
    }

    if (image_points.size() < 5) {
        std::cerr << "Need at least 5 good calibration images. Got "
                  << image_points.size() << "\n";
        return 1;
    }

    cv::Mat camera_matrix = cv::Mat::eye(3, 3, CV_64F);
    cv::Mat dist_coeffs = cv::Mat::zeros(8, 1, CV_64F);
    std::vector<cv::Mat> rvecs;
    std::vector<cv::Mat> tvecs;

    double rms = cv::calibrateCamera(
        object_points,
        image_points,
        image_size,
        camera_matrix,
        dist_coeffs,
        rvecs,
        tvecs);

    cv::FileStorage fs(output_path, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        std::cerr << "Could not write " << output_path << "\n";
        return 1;
    }

    fs << "image_width" << image_size.width;
    fs << "image_height" << image_size.height;
    fs << "pattern" << pattern;
    fs << "board_cols" << board_cols;
    fs << "board_rows" << board_rows;
    fs << "spacing" << spacing;
    fs << "rms_reprojection_error" << rms;
    fs << "camera_matrix" << camera_matrix;
    fs << "distortion_coefficients" << dist_coeffs;
    fs << "used_images" << (int)image_points.size();
    fs.release();

    std::cout << "\nCalibration complete\n";
    std::cout << "used images: " << image_points.size() << "\n";
    std::cout << "RMS error: " << rms << "\n";
    std::cout << "camera matrix:\n" << camera_matrix << "\n";
    std::cout << "distortion:\n" << dist_coeffs.t() << "\n";
    std::cout << "wrote: " << output_path << "\n";

    return 0;
}

