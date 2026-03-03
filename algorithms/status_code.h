#ifndef CABLE_DETECTOR_STATUS_CODE_H
#define CABLE_DETECTOR_STATUS_CODE_H

#include <stdexcept>
#include <string>

namespace cable_detector {

/**
 * @brief Status codes for cable detection operations
 */
enum class StatusCode {
    SUCCESS = 1000,             ///< Processing successful
    READ_IMAGE_ERROR = 1001,    ///< Image read failed, check file path
    PROCESS_IMAGE_ERROR = 1002, ///< Image processing failed, check image quality
    EMPTY_CABLE_ERROR = 1003,   ///< No cable detected, place cable properly
    ANORMAL_CABLE_ERROR = 1004, ///< Cable detection abnormal, check cable status
    ALGORITHM_ERROR = 1005,     ///< Algorithm processing error, feedback needed
    DENOISE_ERROR = 1006        ///< Denoising error, check environment configuration
};

/**
 * @brief Get string description of status code
 * @param code Status code
 * @return Human-readable description
 */
inline std::string statusCodeToString(StatusCode code)
{
    switch (code) {
    case StatusCode::SUCCESS:
        return "SUCCESS";
    case StatusCode::READ_IMAGE_ERROR:
        return "READ_IMAGE_ERROR";
    case StatusCode::PROCESS_IMAGE_ERROR:
        return "PROCESS_IMAGE_ERROR";
    case StatusCode::EMPTY_CABLE_ERROR:
        return "EMPTY_CABLE_ERROR";
    case StatusCode::ANORMAL_CABLE_ERROR:
        return "ANORMAL_CABLE_ERROR";
    case StatusCode::ALGORITHM_ERROR:
        return "ALGORITHM_ERROR";
    case StatusCode::DENOISE_ERROR:
        return "DENOISE_ERROR";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief CableDetectorException - custom exception for cable detector errors
 */
class CableDetectorException : public std::runtime_error
{
public:
    explicit CableDetectorException(StatusCode code, const std::string& message)
        : std::runtime_error(message)
        , code_(code)
    {}

    StatusCode getCode() const { return code_; }

private:
    StatusCode code_;
};

} // namespace cable_detector

#endif // CABLE_DETECTOR_STATUS_CODE_H
