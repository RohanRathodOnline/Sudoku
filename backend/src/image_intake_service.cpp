#include "image_intake_service.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

ImageIntakeResult ImageIntakeService::decodeAndPrepare(const std::string& rawBytes) {
    ImageIntakeResult result;

    // ── 1. Size check ────────────────────────────────────────────────────────
    if (rawBytes.empty()) {
        result.errorCode    = "E_UPLOAD_EMPTY";
        result.errorMessage = "Uploaded file is empty";
        return result;
    }

    if (rawBytes.size() > MAX_BYTES) {
        result.errorCode    = "E_IMAGE_TOO_LARGE";
        result.errorMessage = "Uploaded image exceeds the 10 MB limit";
        return result;
    }

    // ── 2. Decode bytes → cv::Mat ────────────────────────────────────────────
    const std::vector<uchar> buf(rawBytes.begin(), rawBytes.end());
    cv::Mat decoded = cv::imdecode(buf, cv::IMREAD_COLOR);

    if (decoded.empty()) {
        result.errorCode    = "E_DECODE_FAILED";
        result.errorMessage = "Could not decode image — unsupported format or corrupt file";
        return result;
    }

    // ── 3. Orientation / size sanity ─────────────────────────────────────────
    // Resize very large images to cap processing time (keep aspect ratio).
    constexpr int MAX_DIM = 1600;
    if (decoded.rows > MAX_DIM || decoded.cols > MAX_DIM) {
        const double scale = static_cast<double>(MAX_DIM) /
                             std::max(decoded.rows, decoded.cols);
        cv::resize(decoded, decoded, cv::Size(), scale, scale, cv::INTER_AREA);
    }

    result.success = true;
    result.image   = std::move(decoded);
    return result;
}
