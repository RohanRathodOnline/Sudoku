#pragma once

#include <opencv2/core.hpp>
#include <string>
#include <vector>

// Returned by VisionPipeline::process()
struct PipelineResult {
    bool success = false;

    // Raw OCR/digit-recognition output (9×9, 0 = empty cell).
    std::vector<std::vector<int>> detectedGrid;

    // Solver output (9×9, fully filled).
    std::vector<std::vector<int>> solvedGrid;

    std::string errorCode;
    std::string errorMessage;
};

// Full image → solved-grid pipeline.
//
// Steps performed internally:
//   1. Grayscale + adaptive threshold
//   2. Largest-quadrilateral board detection
//   3. Perspective warp → 450×450 canonical grid
//   4. Cell extraction (81 cells)
//   5. Digit recognition (CNN via ONNX or pixel-ratio fallback)
//   6. Sudoku solver
class VisionPipeline {
public:
    PipelineResult process(const cv::Mat& image);
};
