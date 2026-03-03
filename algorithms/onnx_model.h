#ifndef CABLE_DETECTOR_ONNX_MODEL_H
#define CABLE_DETECTOR_ONNX_MODEL_H

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <string>

namespace cable_detector {

/**
 * @brief ONNX Model wrapper for image denoising
 */
class ONNXModel
{
public:
    /**
     * @brief Construct ONNX model
     * @param model_path Path to ONNX model file
     */
    explicit ONNXModel(const std::string& model_path);

    ~ONNXModel();

    /**
     * @brief Run inference on image
     * @param input Input image (uint16 or float32)
     * @return Denoised image
     */
    cv::Mat run(const cv::Mat& input);

    /**
     * @brief Check if model is loaded
     * @return true if loaded
     */
    bool isLoaded() const { return loaded_; }

private:
    std::string model_path_;
    void* session_; // OrtSession*
    std::string input_name_;
    std::string output_name_;
    double i_min_, i_max_;
    bool loaded_;

    // Preprocessing
    cv::Mat preprocess(const cv::Mat& image);

    // Postprocessing
    cv::Mat postprocess(const cv::Mat& output);
};

/**
 * @brief Clean up global ONNX Runtime environment
 * Call this at the end of tests to prevent memory leaks
 */
void cleanupGlobalONNXEnv();

} // namespace cable_detector

#endif // CABLE_DETECTOR_ONNX_MODEL_H
