#pragma once

#include <opencv2/core.hpp>
#include <string>

// Result returned by ImageIntakeService::decodeAndPrepare()
struct ImageIntakeResult {
    bool        success      = false;
    cv::Mat     image;                 // decoded BGR image (valid when success == true)
    std::string errorCode;
    std::string errorMessage;
};

// Validates and decodes raw image bytes (JPEG / PNG / BMP / TIFF) into a cv::Mat.
class ImageIntakeService {
public:
    // Maximum accepted upload size (10 MB).
    static constexpr std::size_t MAX_BYTES = 10 * 1024 * 1024;

    // rawBytes — the binary content of the uploaded file (from multipart form).
    ImageIntakeResult decodeAndPrepare(const std::string& rawBytes);
};
