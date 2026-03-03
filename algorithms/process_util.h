#ifndef CABLE_DETECTOR_PROCESS_UTIL_H
#define CABLE_DETECTOR_PROCESS_UTIL_H

#include <cstdint>
#include <map>
#include <opencv2/opencv.hpp>
#include <string>
#include <tuple>
#include <vector>

#include "algorithms/status_code.h"
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace cable_detector {

// Axis configuration structure
struct AxisConfig
{
    std::string ind_name;    // Independent variable name
    std::string dep_name;    // Dependent variable name
    std::string min_ind_key; // Min independent key
    std::string max_ind_key; // Max independent key
    std::string min_dep_key; // Min dependent key
    std::string max_dep_key; // Max dependent key
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
struct CurveFitResult
{
    std::vector<double> params; // [a, b, c, d]
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
std::tuple<cable_detector::StatusCode, std::vector<CurveFitResult>> fitComponentCurve(
    const cv::Mat& img_otsu, const std::string& main_axis = "x", int exp_lines = 4);

/**
 * @brief Fit median curve from component curves
 * @param img_otsu Binary image
 * @param all_fits Vector of curve fit results
 * @param main_axis Main axis
 * @param exp_lines Expected number of lines
 * @return Updated vector with median curve added
 */
std::vector<CurveFitResult> fitMedianCurve(const cv::Mat& img_otsu,
                                           std::vector<CurveFitResult> all_fits,
                                           const std::string& main_axis = "x",
                                           int exp_lines = 4);

// Result data structure
struct RadiusResult
{
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
std::tuple<cv::Mat, std::vector<RadiusResult>> getRadiusByMidPopt(
    const std::vector<CurveFitResult>& all_fits,
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
        double x_diameter;
        double y_diameter;
        double diameter;
    };

    // Wall thickness
    struct ThicknessData
    {
        double thickness;
        double min_thickness;
        int min_angle;
        std::vector<double> spec_thickness;
    };

    // Related data
    struct RelatedData
    {
        double eccentricity;
        double ovality;
        double x_ratio;
        double y_ratio;
    };

    EllipseData inner_ellipse;
    EllipseData outer_ellipse;
    ThicknessData wall_thickness;
    RelatedData related;
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

} // namespace cable_detector

#endif // CABLE_DETECTOR_PROCESS_UTIL_H
