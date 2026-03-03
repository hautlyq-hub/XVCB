#include "algorithms/cable_detector.h"
#include <QDebug>

namespace cable_detector {

CableDetector::CableDetector(double sample_ratio,
                             int sample_count,
                             const std::string& model_type,
                             const std::string& model_file)
    : sample_ratio_(sample_ratio)
    , sample_count_(sample_count)
    , model_type_(model_type)
    , model_file_(model_file)
{
    // Input validation
    if (!(sample_ratio > 0.0 && sample_ratio <= 1.0)) {
        throw CableDetectorException(StatusCode::ALGORITHM_ERROR, "sample_ratio must be in (0, 1]");
    }
    if (!(sample_count > 0)) {
        throw CableDetectorException(StatusCode::ALGORITHM_ERROR,
                                     "sample_count must be a positive integer");
    }

    // Load ONNX model if specified
    if (model_type_ == "onnx" && !model_file_.empty()) {
        qDebug() << ("Loading ONNX model: " + model_file_);
        try {
            onnx_model_ = std::make_unique<ONNXModel>(model_file_);
            if (!onnx_model_->isLoaded()) {
                qDebug() << ("Failed to load ONNX model, continuing without denoising");
                onnx_model_.reset();
            }
        } catch (const std::exception& e) {
            qDebug() << ("Failed to load ONNX model: " + std::string(e.what())
                         + ", continuing without denoising");
            onnx_model_.reset();
        }
    } else {
        throw CableDetectorException(StatusCode::DENOISE_ERROR,
                                     "model_type must be 'onnx' if model_file is specified");
    }

    qDebug() << ("CableDetector initialized with sample_ratio=" + std::to_string(sample_ratio_)
                 + ", sample_count=" + std::to_string(sample_count_)
                 + ", model_type=" + model_type_);
}

CableDetector::~CableDetector() = default;

void CableDetector::validateInputs(const cv::Size& raw_size,
                                   const std::string& dtype,
                                   const cv::Size& crop_size,
                                   bool pos_perspective,
                                   double target_diameter) const
{
    if (raw_size.width <= 0 || raw_size.height <= 0) {
        throw CableDetectorException(StatusCode::READ_IMAGE_ERROR,
                                     "raw_size must have positive width and height");
    }
    if (dtype.empty()) {
        throw CableDetectorException(StatusCode::READ_IMAGE_ERROR,
                                     "dtype must be a non-empty string");
    }
    if (crop_size.width > 0 && crop_size.height > 0) {
        if (crop_size.width > raw_size.width || crop_size.height > raw_size.height) {
            throw CableDetectorException(StatusCode::READ_IMAGE_ERROR,
                                         "crop_size must be smaller than raw_size");
        }
    }
    if (!(pos_perspective == true || pos_perspective == false)) {
        throw CableDetectorException(StatusCode::ALGORITHM_ERROR,
                                     "pos_perspective must be a boolean");
    }
    if (!(target_diameter > 0)) {
        throw CableDetectorException(StatusCode::ALGORITHM_ERROR,
                                     "target_diameter must be a positive number");
    }
}

std::tuple<StatusCode, cv::Mat, cv::Mat, cv::Mat, cv::Mat, MeasurementData> CableDetector::measure(
    const std::string& lr_dcm_path,
    const std::string& ud_dcm_path,
    const cv::Size& raw_size,
    const std::string& dtype,
    const cv::Size& crop_size,
    bool pos_perspective,
    double target_diameter)
{
    // Validate inputs
    try {
        validateInputs(raw_size, dtype, crop_size, pos_perspective, target_diameter);
    } catch (const CableDetectorException& e) {
        qDebug() << (e.what());
        return std::make_tuple(e.getCode(),
                               cv::Mat(),
                               cv::Mat(),
                               cv::Mat(),
                               cv::Mat(),
                               MeasurementData{});
    }

    // Step 1: Read images
    try {
        lr_img_ = readImageData(lr_dcm_path, raw_size, dtype, crop_size);
        qDebug() << ("LR image loaded: rows=" + std::to_string(lr_img_.rows) + ", cols="
                     + std::to_string(lr_img_.cols) + ", type=" + std::to_string(lr_img_.type()));

        if (pos_perspective) {
            cv::flip(lr_img_, lr_img_, 1);
        }

        ud_img_ = readImageData(ud_dcm_path, raw_size, dtype, crop_size);
        qDebug() << ("UD image loaded: rows=" + std::to_string(ud_img_.rows) + ", cols="
                     + std::to_string(ud_img_.cols) + ", type=" + std::to_string(ud_img_.type()));

    } catch (const CableDetectorException& e) {
        qDebug() << ("Error reading images: " + std::string(e.what()));
        return std::make_tuple(StatusCode::READ_IMAGE_ERROR,
                               cv::Mat(),
                               cv::Mat(),
                               cv::Mat(),
                               cv::Mat(),
                               MeasurementData{});
    } catch (const std::exception& e) {
        qDebug() << ("Error reading images: " + std::string(e.what()));
        return std::make_tuple(StatusCode::READ_IMAGE_ERROR,
                               cv::Mat(),
                               cv::Mat(),
                               cv::Mat(),
                               cv::Mat(),
                               MeasurementData{});
    }

    // Step 2: ONNX denoising (if model is loaded)
    if (onnx_model_ && onnx_model_->isLoaded()) {
        try {
            qDebug() << ("Running ONNX denoising...");
            lr_img_ = onnx_model_->run(lr_img_);
            ud_img_ = onnx_model_->run(ud_img_);
            qDebug() << ("ONNX denoising complete");
        } catch (const std::exception& e) {
            qDebug() << ("ONNX inference error: " + std::string(e.what()));
            return std::make_tuple(StatusCode::DENOISE_ERROR,
                                   cv::Mat(),
                                   cv::Mat(),
                                   cv::Mat(),
                                   cv::Mat(),
                                   MeasurementData{});
        }
    }

    // Step 3: Image postprocessing
    try {
        lr_img_post_ = imagePostprocess(lr_img_);
        ud_img_post_ = imagePostprocess(ud_img_);
        qDebug() << ("LR postprocess done: size=" + std::to_string(lr_img_post_.rows) + "x"
                     + std::to_string(lr_img_post_.cols));
        qDebug() << ("UD postprocess done: size=" + std::to_string(ud_img_post_.rows) + "x"
                     + std::to_string(ud_img_post_.cols));

        // Edge detection
        cv::Mat lr_edges = getImageEdges(lr_img_post_);
        cv::Mat ud_edges = getImageEdges(ud_img_post_);
        qDebug() << ("Edge detection done");

        // Binary image
        auto [lr_binary, lr_axis] = getBinaryImage(lr_edges);
        auto [ud_binary, ud_axis] = getBinaryImage(ud_edges);

        lr_img_otsu_ = lr_binary;
        ud_img_otsu_ = ud_binary;
        lr_main_axis_ = lr_axis;
        ud_main_axis_ = ud_axis;
        qDebug() << ("Binary image done, lr_axis=" + lr_axis + ", ud_axis=" + ud_axis);

    } catch (const std::exception& e) {
        qDebug() << ("Image processing error: " + std::string(e.what()));
        return std::make_tuple(StatusCode::PROCESS_IMAGE_ERROR,
                               cv::Mat(),
                               cv::Mat(),
                               lr_img_post_,
                               ud_img_post_,
                               MeasurementData{});
    }

    // Step 4: Curve fitting
    try {
        auto [status1, lr_all_fits] = fitComponentCurve(lr_img_otsu_, lr_main_axis_);
        if (status1 != StatusCode::SUCCESS) {
            qDebug() << ("LR curve fitting failed: " + statusCodeToString(status1));
            return std::make_tuple(status1,
                                   cv::Mat(),
                                   cv::Mat(),
                                   lr_img_post_,
                                   ud_img_post_,
                                   MeasurementData{});
        }

        auto [status2, ud_all_fits] = fitComponentCurve(ud_img_otsu_, ud_main_axis_);
        if (status2 != StatusCode::SUCCESS) {
            qDebug() << ("UD curve fitting failed: " + statusCodeToString(status2));
            return std::make_tuple(status2,
                                   cv::Mat(),
                                   cv::Mat(),
                                   lr_img_post_,
                                   ud_img_post_,
                                   MeasurementData{});
        }

        qDebug() << ("Component curve fitting done, lr_fits=" + std::to_string(lr_all_fits.size())
                     + ", ud_fits=" + std::to_string(ud_all_fits.size()));

        // Fit median curves
        lr_all_fits = fitMedianCurve(lr_img_otsu_, lr_all_fits, lr_main_axis_);
        ud_all_fits = fitMedianCurve(ud_img_otsu_, ud_all_fits, ud_main_axis_);

        // Calculate radius profiles
        auto [lr_profile, lr_results] = getRadiusByMidPopt(lr_all_fits,
                                                           lr_main_axis_,
                                                           sample_ratio_,
                                                           sample_count_,
                                                           lr_img_otsu_);
        auto [ud_profile, ud_results] = getRadiusByMidPopt(ud_all_fits,
                                                           ud_main_axis_,
                                                           sample_ratio_,
                                                           sample_count_,
                                                           ud_img_otsu_);

        lr_profile_ = lr_profile;
        ud_profile_ = ud_profile;
        lr_result_data_ = lr_results;
        ud_result_data_ = ud_results;
        qDebug() << ("Radius calculation done");

    } catch (const std::exception& e) {
        qDebug() << ("Component curve fit error: " + std::string(e.what()));
        return std::make_tuple(StatusCode::ANORMAL_CABLE_ERROR,
                               cv::Mat(),
                               cv::Mat(),
                               lr_img_post_,
                               ud_img_post_,
                               MeasurementData{});
    }

    // Step 5: Analyze data
    try {
        measure_data_ = analyze2DWireData(lr_result_data_, ud_result_data_, target_diameter);
        qDebug() << ("Analysis complete");
    } catch (const std::exception& e) {
        qDebug() << ("Algorithm error: " + std::string(e.what()));
        return std::make_tuple(StatusCode::ALGORITHM_ERROR,
                               cv::Mat(),
                               cv::Mat(),
                               lr_img_post_,
                               ud_img_post_,
                               MeasurementData{});
    }

    return std::make_tuple(StatusCode::SUCCESS,
                           lr_profile_,
                           ud_profile_,
                           lr_img_post_,
                           ud_img_post_,
                           measure_data_);
}

} // namespace cable_detector
