#ifndef CABLE_DETECTOR_CABLE_DETECTOR_H
#define CABLE_DETECTOR_CABLE_DETECTOR_H

#include "algorithms/onnx_model.h"
#include "algorithms/process_util.h"
#include "algorithms/status_code.h"

#include <string>
#include <memory>
#include <opencv2/opencv.hpp>

namespace cable_detector {

/**
 * @brief CableDetector - Main class for cable diameter detection
 */
class CableDetector {
public:
    /**
     * @brief Construct a CableDetector
     * @param sample_ratio Ratio of samples to take from the image (0, 1]
     * @param sample_count Number of samples to take
     * @param model_type Type of denoising model ("onnx", "nbg", or empty for none)
     * @param model_file Path to the model file
     */
    CableDetector(double sample_ratio, int sample_count,
                  const std::string& model_type = "",
                  const std::string& model_file = "");
    
    ~CableDetector();
    
    /**
     * @brief Measure cable diameter from two images
     * @param lr_dcm_path Path to left-right cable image (vertical group detector)
     * @param ud_dcm_path Path to up-down cable image (horizontal group detector)
     * @param raw_size Raw image size (width, height)
     * @param dtype Data type ("uint16" or "float32")
     * @param crop_size Crop size (width, height), optional
     * @param pos_perspective Whether to flip the LR image horizontally
     * @param target_diameter Target inner diameter in mm
     * @return Tuple of: status code, LR profile, UD profile, 
     *                   LR processed image, UD processed image, measurement data
     */
    std::tuple<StatusCode, cv::Mat, cv::Mat, cv::Mat, cv::Mat, MeasurementData>
    measure(const std::string& lr_dcm_path,
            const std::string& ud_dcm_path,
            const cv::Size& raw_size,
            const std::string& dtype,
            const cv::Size& crop_size = cv::Size(),
            bool pos_perspective = false,
            double target_diameter = 0.0);
    
    // Getters for debugging/visualization
    cv::Mat getLrImage() const { return lr_img_; }
    cv::Mat getUdImage() const { return ud_img_; }
    cv::Mat getLrImagePost() const { return lr_img_post_; }
    cv::Mat getUdImagePost() const { return ud_img_post_; }
    cv::Mat getLrImageOtsu() const { return lr_img_otsu_; }
    cv::Mat getUdImageOtsu() const { return ud_img_otsu_; }
    cv::Mat getLrProfile() const { return lr_profile_; }
    cv::Mat getUdProfile() const { return ud_profile_; }
    MeasurementData getMeasureData() const { return measure_data_; }
    
private:
    double sample_ratio_;
    int sample_count_;
    std::string model_type_;
    std::string model_file_;
    
    // Image storage
    cv::Mat lr_img_;
    cv::Mat ud_img_;
    cv::Mat lr_img_post_;
    cv::Mat ud_img_post_;
    cv::Mat lr_img_otsu_;
    cv::Mat ud_img_otsu_;
    cv::Mat lr_profile_;
    cv::Mat ud_profile_;
    
    // Axis info
    std::string lr_main_axis_;
    std::string ud_main_axis_;
    
    // Result data
    std::vector<RadiusResult> lr_result_data_;
    std::vector<RadiusResult> ud_result_data_;
    MeasurementData measure_data_;
    
    // ONNX model
    std::unique_ptr<ONNXModel> onnx_model_;
    
    // Internal methods
    void validateInputs(const cv::Size& raw_size, const std::string& dtype,
                        const cv::Size& crop_size, bool pos_perspective,
                        double target_diameter) const;
};

}  // namespace cable_detector

#endif  // CABLE_DETECTOR_CABLE_DETECTOR_H
