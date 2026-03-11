/*  vision_pipeline.cpp
 *
 *  Pipeline stages
 *  ───────────────
 *  1. preprocess()   – grayscale → Gaussian blur → adaptive threshold → dilate
 *  2. detectBoard()  – find the largest quadrilateral contour (the Sudoku border)
 *  3. warpBoard()    – four-point perspective transform → 450×450 px
 *  4. extractCells() – slice the 450×450 grid into 81 equal cell images
 *  5. recognizeDigit()– pixel-ink-ratio based classifier (works without a CNN model)
 *                       Override with ONNX CNN when assets/models/mnist.onnx is present.
 *  6. SudokuSolver   – backtracking solver (from sudoku_solver.cpp)
 */

#include "vision_pipeline.h"
#include "sudoku_solver.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <numeric>

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

// ── Geometry helpers ──────────────────────────────────────────────────────────

float dist2(cv::Point2f a, cv::Point2f b) {
    float dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Order four corners as: top-left, top-right, bottom-right, bottom-left
std::array<cv::Point2f, 4> orderCorners(const std::vector<cv::Point2f>& pts) {
    std::array<cv::Point2f, 4> ordered{};

    // Top-left = smallest (x+y), Bottom-right = largest (x+y)
    // Top-right = smallest (y-x), Bottom-left = largest (y-x)
    std::vector<float> sums, diffs;
    for (const auto& p : pts) { sums.push_back(p.x + p.y); diffs.push_back(p.y - p.x); }

    ordered[0] = pts[std::min_element(sums.begin(),  sums.end())  - sums.begin()];
    ordered[2] = pts[std::max_element(sums.begin(),  sums.end())  - sums.begin()];
    ordered[1] = pts[std::min_element(diffs.begin(), diffs.end()) - diffs.begin()];
    ordered[3] = pts[std::max_element(diffs.begin(), diffs.end()) - diffs.begin()];

    return ordered;
}

// ── Pre-processing ────────────────────────────────────────────────────────────

cv::Mat preprocess(const cv::Mat& src) {
    cv::Mat gray, blurred, thresh;
    if (src.channels() == 3)
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else
        gray = src.clone();

    cv::GaussianBlur(gray, blurred, cv::Size(9, 9), 0);
    cv::adaptiveThreshold(blurred, thresh, 255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY_INV, 11, 2);

    // Small dilation to close breaks in grid lines
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(thresh, thresh, kernel, cv::Point(-1, -1), 1);
    return thresh;
}

// ── Board detection ───────────────────────────────────────────────────────────

// Returns the four corners of the largest quadrilateral found in `thresh`.
// Returns empty vector if no convincing board is found.
std::vector<cv::Point2f> detectBoardCorners(const cv::Mat& thresh) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) return {};

    // Sort by area descending
    std::sort(contours.begin(), contours.end(),
              [](const auto& a, const auto& b) {
                  return cv::contourArea(a) > cv::contourArea(b);
              });

    const double imageArea = static_cast<double>(thresh.rows * thresh.cols);

    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < imageArea * 0.10) break; // too small — nothing useful left

        // Approximate polygon
        std::vector<cv::Point> approx;
        const double peri = cv::arcLength(contour, true);
        cv::approxPolyDP(contour, approx, 0.02 * peri, true);

        if (approx.size() == 4 && cv::isContourConvex(approx)) {
            std::vector<cv::Point2f> pts;
            for (const auto& p : approx) pts.push_back(static_cast<cv::Point2f>(p));
            return pts;
        }
    }
    return {};
}

// ── Perspective warp ──────────────────────────────────────────────────────────

cv::Mat warpBoard(const cv::Mat& src, const std::vector<cv::Point2f>& corners) {
    constexpr int GRID_SIZE = 450;

    auto ordered = orderCorners(corners);

    // Compute output side length from the longer diagonal
    float widthA  = std::sqrt(dist2(ordered[0], ordered[1]));
    float widthB  = std::sqrt(dist2(ordered[2], ordered[3]));
    float heightA = std::sqrt(dist2(ordered[0], ordered[3]));
    float heightB = std::sqrt(dist2(ordered[1], ordered[2]));

    // Clamp to GRID_SIZE for a canonical square output
    (void)widthA; (void)widthB; (void)heightA; (void)heightB;

    std::vector<cv::Point2f> dst = {
        {0.f, 0.f},
        {(float)GRID_SIZE, 0.f},
        {(float)GRID_SIZE, (float)GRID_SIZE},
        {0.f, (float)GRID_SIZE}
    };

    cv::Mat M   = cv::getPerspectiveTransform(ordered.data(), dst.data());
    cv::Mat out;
    cv::warpPerspective(src, out, M, {GRID_SIZE, GRID_SIZE});
    return out;
}

// ── Cell extraction ───────────────────────────────────────────────────────────

// Returns 81 grayscale cell images (top-left → bottom-right, row-major).
std::vector<cv::Mat> extractCells(const cv::Mat& grid) {
    constexpr int GRID_SIZE = 450;
    constexpr int CELL_SIZE = GRID_SIZE / 9; // 50 px

    std::vector<cv::Mat> cells;
    cells.reserve(81);

    cv::Mat gray;
    if (grid.channels() == 3)
        cv::cvtColor(grid, gray, cv::COLOR_BGR2GRAY);
    else
        gray = grid.clone();

    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            const int x = col * CELL_SIZE;
            const int y = row * CELL_SIZE;

            // Crop cell and remove border noise (~15% margin)
            const int margin = CELL_SIZE / 7;
            cv::Rect roi(x + margin, y + margin,
                         CELL_SIZE - 2 * margin, CELL_SIZE - 2 * margin);
            cells.push_back(gray(roi).clone());
        }
    }
    return cells;
}

// ── CNN-based digit recognizer (optional) ─────────────────────────────────────

struct CnnRecognizer {
    cv::dnn::Net net;
    bool loaded = false;

    void tryLoad() {
        const std::filesystem::path candidates[] = {
            "assets/models/mnist.onnx",
            "../assets/models/mnist.onnx"
        };
        for (const auto& p : candidates) {
            if (std::filesystem::exists(p)) {
                try {
                    net    = cv::dnn::readNet(p.string());
                    loaded = !net.empty();
                    if (loaded)
                        std::cout << "[VisionPipeline] CNN model loaded: " << p << "\n";
                    return;
                } catch (...) {}
            }
        }
        std::cout << "[VisionPipeline] CNN model not found — using pixel-ratio classifier.\n";
    }

    // Returns 1-9 or 0 (empty).
    int classify(const cv::Mat& cellGray) {
        cv::Mat thresh;
        cv::threshold(cellGray, thresh, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

        // Empty-cell test: low ink ratio means blank
        const double inkRatio = cv::countNonZero(thresh) /
                                 static_cast<double>(thresh.rows * thresh.cols);
        if (inkRatio < 0.04) return 0;

        if (!loaded) return -1; // signal to fallback classifier

        // Prepare 28×28 float blob
        cv::Mat resized;
        cv::resize(thresh, resized, {28, 28});
        resized.convertTo(resized, CV_32F, 1.0 / 255.0);

        cv::Mat blob = cv::dnn::blobFromImage(resized);
        net.setInput(blob);
        cv::Mat prob = net.forward();

        cv::Point maxLoc;
        cv::minMaxLoc(prob, nullptr, nullptr, nullptr, &maxLoc);
        const int digit = maxLoc.x; // 0-9
        return (digit == 0) ? 0 : digit;
    }
};

// ── Pixel-ratio fallback classifier ──────────────────────────────────────────
//
// Without a trained CNN we cannot reliably distinguish digits 1-9 from an
// arbitrary Sudoku image.  This fallback returns 0 for empty cells and -1 for
// filled cells so the caller knows "there is a digit here but we could not read
// it."  The frontend will show the raw extraction result and ask the user to
// correct it.
int pixelRatioClassify(const cv::Mat& cellGray) {
    cv::Mat thresh;
    cv::threshold(cellGray, thresh, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    const double inkRatio = cv::countNonZero(thresh) /
                             static_cast<double>(thresh.rows * thresh.cols);
    return (inkRatio < 0.04) ? 0 : -1; // -1 = "has digit, unknown value"
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// VisionPipeline::process
// ─────────────────────────────────────────────────────────────────────────────
PipelineResult VisionPipeline::process(const cv::Mat& image) {
    PipelineResult result;

    if (image.empty()) {
        result.errorCode    = "E_EMPTY_IMAGE";
        result.errorMessage = "Received empty image";
        return result;
    }

    // ── Stage 1: preprocess ───────────────────────────────────────────────────
    const cv::Mat thresh = preprocess(image);

    // ── Stage 2: board detection ──────────────────────────────────────────────
    const auto corners = detectBoardCorners(thresh);
    if (corners.size() != 4) {
        result.errorCode    = "E_GRID_NOT_FOUND";
        result.errorMessage = "Could not locate a Sudoku grid in the image. "
                              "Ensure the grid is well-lit and fully visible.";
        return result;
    }

    // ── Stage 3: perspective warp ─────────────────────────────────────────────
    const cv::Mat warped = warpBoard(image, corners);

    // ── Stage 4: cell extraction ──────────────────────────────────────────────
    const auto cells = extractCells(warped);
    if (cells.size() != 81) {
        result.errorCode    = "E_CELL_EXTRACT_FAILED";
        result.errorMessage = "Failed to extract 81 cells from the detected grid";
        return result;
    }

    // ── Stage 5: digit recognition ────────────────────────────────────────────
    static CnnRecognizer cnn = []() {
        CnnRecognizer r;
        r.tryLoad();
        return r;
    }();

    std::vector<std::vector<int>> grid(9, std::vector<int>(9, 0));
    bool anyUnknown = false;

    for (int i = 0; i < 81; ++i) {
        int digit = cnn.classify(cells[i]);

        if (digit == -1) {
            // CNN absent — try pixel ratio
            digit = pixelRatioClassify(cells[i]);
            if (digit == -1) {
                anyUnknown = true;
                digit = 0; // treat unknown as empty so the solver can try
            }
        }

        if (digit < 0 || digit > 9) digit = 0;
        grid[i / 9][i % 9] = digit;
    }

    result.detectedGrid = grid;

    if (anyUnknown && !cnn.loaded) {
        // We found cell content but could not read digits without the CNN model.
        // Return the blank template so the user can fill it manually.
        result.errorCode    = "E_DIGIT_RECOGNITION_FAILED";
        result.errorMessage = "Grid detected but digit recognition requires the "
                              "CNN model (assets/models/mnist.onnx). "
                              "Use manual entry or provide the model file.";
        return result;
    }

    // ── Stage 6: solve ────────────────────────────────────────────────────────
    SudokuSolver solver;
    auto toSolve = grid;
    if (!solver.solve(toSolve)) {
        result.errorCode    = "E_SOLVER_FAILED";
        result.errorMessage = "Detected board has no valid solution — "
                              "digit recognition may have made errors.";
        return result;
    }

    result.solvedGrid = toSolve;
    result.success    = true;
    return result;
}
