#ifndef CABLE_DETECTOR_PROCESS_UTIL_H
#define CABLE_DETECTOR_PROCESS_UTIL_H

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <cstdint>

#include "algorithms/status_code.h"
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace cable_detector {

// Axis configuration structure
struct AxisConfig {
    std::string ind_name;     // Independent variable name
    std::string dep_name;     // Dependent variable name
    std::string min_ind_key;  // Min independent key
    std::string max_ind_key;  // Max independent key
    std::string min_dep_key;  // Min dependent key
    std::string max_dep_key;  // Max dependent key
};

// Global configuration
extern const std::map<std::string, AxisConfig> globalAxisConfig;

/**
 * @brief Read raw image data from file
 * @param raw_path Path to raw image file
 * @param raw_size Image size (width, height)
 * @param dtype Data type ("uint16" or "float32")
 * @param crop_size Optional crop size
 * @return Loaded image
 */
cv::Mat readImageData(const std::string& raw_path, 
                      const cv::Size& raw_size,
                      const std::string& dtype,
                      const cv::Size& crop_size = cv::Size());

/**
 * @brief Postprocess image for display/analysis
 * @param image Input image
 * @return Normalized uint8 image
 */
cv::Mat imagePostprocess(const cv::Mat& image);

/**
 * @brief Detect edges using Sobel operator
 * @param image Input grayscale image
 * @return Edge image
 */
cv::Mat getImageEdges(const cv::Mat& image);

/**
 * @brief Get binary image using peak detection
 * @param image Input edge image
 * @param return_main_axis Whether to return main axis
 * @return Binary image, optionally with main axis
 */
std::tuple<cv::Mat, std::string> getBinaryImage(const cv::Mat& image, bool return_main_axis = true);

// Cubic function for curve fitting
double cubicFunction(double x, double a, double b, double c, double d);
double cubicDerivative(double x, double a, double b, double c);

// Curve fit result structure
struct CurveFitResult {
    std::vector<double> params;  // [a, b, c, d]
    double min_x, max_x;
    double min_y, max_y;
    std::string independent_var;
    std::string dependent_var;
    cv::Point2d centroid;
    int label;
};

/**
 * @brief Fit cubic curves to binary image components
 * @param img_otsu Binary image
 * @param main_axis Main axis ("x" or "y")
 * @param exp_lines Expected number of components
 * @return Status code and vector of fit results
 */
std::tuple<cable_detector::StatusCode, std::vector<CurveFitResult>>
fitComponentCurve(const cv::Mat& img_otsu, 
                  const std::string& main_axis = "x",
                  int exp_lines = 4);

/**
 * @brief Fit median curve from component curves
 * @param img_otsu Binary image
 * @param all_fits Vector of curve fit results
 * @param main_axis Main axis
 * @param exp_lines Expected number of lines
 * @return Updated vector with median curve added
 */
std::vector<CurveFitResult> 
fitMedianCurve(const cv::Mat& img_otsu,
               std::vector<CurveFitResult> all_fits,
               const std::string& main_axis = "x",
               int exp_lines = 4);

// Result data structure
struct RadiusResult {
    int label;
    std::vector<double> radius;
    double avg_radius;
    double std_radius;
    cv::Mat mid_points;
    cv::Mat inter_points;
};

/**
 * @brief Calculate radius from median curve
 * @param all_fits Vector of curve fits
 * @param main_axis Main axis
 * @param sample_ratio Sample ratio
 * @param sample_count Number of samples
 * @param image Input image
 * @return Summed radial profile and result data
 */
std::tuple<cv::Mat, std::vector<RadiusResult>>
getRadiusByMidPopt(const std::vector<CurveFitResult>& all_fits,
                   const std::string& main_axis,
                   double sample_ratio,
                   int sample_count,
                   const cv::Mat& image);

// Measurement data structure
struct MeasurementData
{
    // Inner ellipse
    struct EllipseData
    {
        cv::Point2d center;
        double x_diameter = 0.0;
        double y_diameter = 0.0;
        double diameter = 0.0;

        EllipseData() = default;
        EllipseData(const EllipseData&) = default;
        EllipseData& operator=(const EllipseData&) = default;
    };

    // Wall thickness
    struct ThicknessData
    {
        double thickness = 0.0;
        double min_thickness = 0.0;
        int min_angle = 0;
        std::vector<double> spec_thickness;

        ThicknessData() = default;

        // 深拷贝构造
        ThicknessData(const ThicknessData& other)
            : thickness(other.thickness)
            , min_thickness(other.min_thickness)
            , min_angle(other.min_angle)
            , spec_thickness(other.spec_thickness) // vector 自动深拷贝
        {}

        // 深拷贝赋值
        ThicknessData& operator=(const ThicknessData& other)
        {
            if (this != &other) {
                thickness = other.thickness;
                min_thickness = other.min_thickness;
                min_angle = other.min_angle;
                spec_thickness = other.spec_thickness; // vector 自动深拷贝
            }
            return *this;
        }

        // 移动构造（可选，提高性能）
        ThicknessData(ThicknessData&& other) noexcept
            : thickness(other.thickness)
            , min_thickness(other.min_thickness)
            , min_angle(other.min_angle)
            , spec_thickness(std::move(other.spec_thickness))
        {}

        // 移动赋值
        ThicknessData& operator=(ThicknessData&& other) noexcept
        {
            if (this != &other) {
                thickness = other.thickness;
                min_thickness = other.min_thickness;
                min_angle = other.min_angle;
                spec_thickness = std::move(other.spec_thickness);
            }
            return *this;
        }
    };

    // Related data
    struct RelatedData
    {
        double eccentricity = 0.0;
        double ovality = 0.0;
        double x_ratio = 0.0;
        double y_ratio = 0.0;

        RelatedData() = default;
        RelatedData(const RelatedData&) = default;
        RelatedData& operator=(const RelatedData&) = default;
    };

    EllipseData inner_ellipse;
    EllipseData outer_ellipse;
    ThicknessData wall_thickness;
    RelatedData related;

    // 为整个 MeasurementData 提供深拷贝
    MeasurementData() = default;

    MeasurementData(const MeasurementData& other)
        : inner_ellipse(other.inner_ellipse)
        , outer_ellipse(other.outer_ellipse)
        , wall_thickness(other.wall_thickness) // 调用 ThicknessData 的深拷贝
        , related(other.related)
    {}

    MeasurementData& operator=(const MeasurementData& other)
    {
        if (this != &other) {
            inner_ellipse = other.inner_ellipse;
            outer_ellipse = other.outer_ellipse;
            wall_thickness = other.wall_thickness; // 调用 ThicknessData 的深拷贝赋值
            related = other.related;
        }
        return *this;
    }

    // 移动构造
    MeasurementData(MeasurementData&& other) noexcept
        : inner_ellipse(std::move(other.inner_ellipse))
        , outer_ellipse(std::move(other.outer_ellipse))
        , wall_thickness(std::move(other.wall_thickness))
        , related(std::move(other.related))
    {}

    MeasurementData& operator=(MeasurementData&& other) noexcept
    {
        if (this != &other) {
            inner_ellipse = std::move(other.inner_ellipse);
            outer_ellipse = std::move(other.outer_ellipse);
            wall_thickness = std::move(other.wall_thickness);
            related = std::move(other.related);
        }
        return *this;
    }
};
/**
 * @brief Analyze 2D wire data to get final measurements
 * @param left_right_data LR direction radius results
 * @param up_down_data UD direction radius results
 * @param target_diameter Target diameter in mm
 * @return Measurement data with ellipse and thickness info
 */
MeasurementData analyze2DWireData(const std::vector<RadiusResult>& left_right_data,
                                   const std::vector<RadiusResult>& up_down_data,
                                   double target_diameter);

}  // namespace cable_detector

#endif  // CABLE_DETECTOR_PROCESS_UTIL_H
