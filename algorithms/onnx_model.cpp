#include "algorithms/onnx_model.h"

#include <QDebug>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <numeric>
#include <onnxruntime_cxx_api.h>
#include <vector>

namespace cable_detector {

// Global ONNX Runtime environment (shared across all instances)
static std::unique_ptr<Ort::Env> global_env;
static std::mutex env_mutex;

// Initialize global environment if not already done
static Ort::Env* getGlobalEnv()
{
    std::lock_guard<std::mutex> lock(env_mutex);
    if (!global_env) {
        global_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "CableDetector");
    }
    return global_env.get();
}

// Clean up global environment (call at end of tests)
void cleanupGlobalONNXEnv()
{
    std::lock_guard<std::mutex> lock(env_mutex);
    global_env.reset();
}

ONNXModel::ONNXModel(const std::string& model_path)
    : model_path_(model_path)
    , session_(nullptr)
    , loaded_(false)
{
    // Get or initialize global ONNX Runtime environment
    Ort::Env* env = getGlobalEnv();

    // Session options
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(1);
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    try {
        // Create session using Ort::Session directly
        Ort::Session* onnx_session = new Ort::Session(*env, model_path.c_str(), session_options);
        session_ = onnx_session;

        // Get input/output names using AllocatedStringPtr
        Ort::AllocatorWithDefaultOptions allocator;

        // Input name - cast session_ back to Ort::Session*
        auto input_name_ptr = onnx_session->GetInputNameAllocated(0, allocator);
        input_name_ = input_name_ptr.get();
        input_name_ptr.release();

        // Output name
        auto output_name_ptr = onnx_session->GetOutputNameAllocated(0, allocator);
        output_name_ = output_name_ptr.get();
        output_name_ptr.release();

        loaded_ = true;
        qDebug() << ("ONNX model loaded successfully: " + model_path);

    } catch (const std::exception& e) {
        qDebug() << ("Failed to load ONNX model: " + std::string(e.what()));
        loaded_ = false;
        // Re-throw the exception to let the caller handle it
        throw std::runtime_error("Failed to load ONNX model: " + std::string(e.what()));
    }
}

ONNXModel::~ONNXModel()
{
    if (session_) {
        delete static_cast<Ort::Session*>(session_);
        session_ = nullptr;
    }
}

cv::Mat ONNXModel::preprocess(const cv::Mat& image)
{
    // Convert uint16 single channel image to float32 (no normalization)
    cv::Mat float_image;
    image.convertTo(float_image, CV_32FC1);

    // Apply sqrt transformation
    cv::sqrt(float_image, float_image);

    // Calculate percentile values (0.1% and 99.9%)
    std::vector<float> pixels;
    float_image.reshape(0, 1).copyTo(pixels);
    std::sort(pixels.begin(), pixels.end());

    size_t size = pixels.size();
    float p01_idx = size * 0.001f;
    float p999_idx = size * 0.999f;

    // Linear interpolation for percentile values
    int p01_idx_low = static_cast<int>(std::floor(p01_idx));
    int p01_idx_high = std::min(p01_idx_low + 1, static_cast<int>(size) - 1);
    float p01_weight = p01_idx - p01_idx_low;

    int p999_idx_low = static_cast<int>(std::floor(p999_idx));
    int p999_idx_high = std::min(p999_idx_low + 1, static_cast<int>(size) - 1);
    float p999_weight = p999_idx - p999_idx_low;

    i_min_ = (1.0f - p01_weight) * pixels[p01_idx_low] + p01_weight * pixels[p01_idx_high];
    i_max_ = (1.0f - p999_weight) * pixels[p999_idx_low] + p999_weight * pixels[p999_idx_high];

    // Normalize using percentile values
    float_image = (float_image - i_min_) / (i_max_ - i_min_);

    // Calculate noise standard deviation
    float noise_std = 0.5f / (i_max_ - i_min_ + 1e-8f);

    // Create 2-channel input: [normalized_image, noise_std_map]
    cv::Mat channels[2];
    channels[0] = float_image;
    channels[1] = cv::Mat(float_image.size(), CV_32FC1, cv::Scalar(noise_std));

    cv::Mat result;
    cv::merge(channels, 2, result);

    return result;
}

cv::Mat ONNXModel::postprocess(const cv::Mat& output)
{
    // Get the first channel and first batch (output[0, 0] in Python)
    cv::Mat result;
    result = output.clone();

    // Apply inverse transformations: denormalize and square
    // output = output * (i_max_ - i_min_) + i_min_
    result = result * (i_max_ - i_min_) + i_min_;

    // output = output * output (square)
    cv::multiply(result, result, result);

    return result;
}

cv::Mat ONNXModel::run(const cv::Mat& input)
{
    if (!loaded_ || !session_) {
        qDebug() << ("ONNX model not loaded, returning input");
        return input;
    }

    try {
        // Preprocess
        cv::Mat preprocessed = preprocess(input);

        // Get model input shape
        auto* session = static_cast<Ort::Session*>(session_);
        Ort::AllocatorWithDefaultOptions allocator;

        // Get input shape
        auto input_name_ptr = session->GetInputNameAllocated(0, allocator);
        std::string input_name = input_name_ptr.get();
        input_name_ptr.release();

        auto input_type_info = session->GetInputTypeInfo(0);
        auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
        auto input_dims = input_tensor_info.GetShape();

        // Extract expected dimensions (should be [1, 2, height, width])
        // Use actual input dimensions if model dimensions are dynamic (-1)
        int expected_height = (input_dims[2] == -1) ? preprocessed.rows
                                                    : static_cast<int>(input_dims[2]);
        int expected_width = (input_dims[3] == -1) ? preprocessed.cols
                                                   : static_cast<int>(input_dims[3]);

        // Create input tensor (1x2xHxW)
        const std::vector<int64_t> input_shape = {1, 2, expected_height, expected_width};

        // Prepare input data - flat array for 2 channels
        static_assert(sizeof(float) == 4, "float must be 32 bits");
        std::vector<float> input_data(2 * expected_height * expected_width);

        // Split channels and copy data
        std::vector<cv::Mat> channels(2);
        cv::split(preprocessed, channels);

        // Copy channel 0, 1 (normalized image, noise std map)
        int input_index = 0, input_offset = expected_height * expected_width;
        for (int h = 0; h < expected_height; h++) {
            for (int w = 0; w < expected_width; w++) {
                input_data[input_index] = channels[0].at<float>(h, w);
                input_data[input_offset + input_index] = channels[1].at<float>(h, w);
                input_index += 1;
            }
        }

        // Create tensor
        Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        auto input_tensor = Ort::Value::CreateTensor<float>(mem_info,
                                                            input_data.data(),
                                                            input_data.size(),
                                                            input_shape.data(),
                                                            input_shape.size());

        // Run inference
        const char* input_names[] = {input_name_.c_str()};
        const char* output_names[] = {output_name_.c_str()};

        auto output_tensors = session->Run(Ort::RunOptions{nullptr},
                                           input_names,
                                           &input_tensor,
                                           1,
                                           output_names,
                                           1);

        // Get output
        auto& output_tensor = output_tensors[0];
        float* output_data = output_tensor.GetTensorMutableData<float>();

        // Get output shape
        auto output_shape = output_tensor.GetTensorTypeAndShapeInfo().GetShape();

        // Create output Mat - assuming 1x1xHxW or 1xHxW
        int64_t out_h = expected_height, out_w = expected_width;
        if (output_shape.size() >= 3) {
            out_h = output_shape[output_shape.size() - 2];
            out_w = output_shape[output_shape.size() - 1];
        }

        cv::Mat output_mat(out_h, out_w, CV_32FC1);
        memcpy(output_mat.data, output_data, sizeof(float) * out_h * out_w);

        // Postprocess
        cv::Mat result = postprocess(output_mat);

        return result;

    } catch (const std::exception& e) {
        qDebug() << ("ONNX inference error: " + std::string(e.what()));
        throw;
    }
}

} // namespace cable_detector
