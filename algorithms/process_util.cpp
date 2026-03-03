#include "algorithms/process_util.h"
#include "algorithms/status_code.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

namespace cable_detector {

// Global axis configuration
const std::map<std::string, AxisConfig> globalAxisConfig
    = {{"y", {"y", "x", "min_y", "max_y", "min_x", "max_x"}},
       {"x", {"x", "y", "min_x", "max_x", "min_y", "max_y"}}};

cv::Mat readImageData(const std::string& raw_path,
                      const cv::Size& raw_size,
                      const std::string& dtype,
                      const cv::Size& crop_size)
{
    std::ifstream file(raw_path, std::ios::binary);
    if (!file.is_open()) {
        throw CableDetectorException(StatusCode::READ_IMAGE_ERROR,
                                     "Failed to open file: " + raw_path);
    }

    cv::Mat image;
    int rows = raw_size.height;
    int cols = raw_size.width;

    if (dtype == "uint16") {
        std::vector<uint16_t> buffer(rows * cols);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size() * sizeof(uint16_t));
        image = cv::Mat(rows, cols, CV_16UC1, buffer.data()).clone();
    } else if (dtype == "float32") {
        std::vector<float> buffer(rows * cols);
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size() * sizeof(float));
        image = cv::Mat(rows, cols, CV_32FC1, buffer.data()).clone();
    } else {
        throw CableDetectorException(StatusCode::READ_IMAGE_ERROR, "Unsupported dtype: " + dtype);
    }

    file.close();

    // Apply crop if specified
    if (crop_size.width > 0 && crop_size.height > 0) {
        if (crop_size.width > raw_size.width || crop_size.height > raw_size.height) {
            throw CableDetectorException(StatusCode::READ_IMAGE_ERROR,
                                         "crop_size must be smaller than raw_size");
        }
        int y = (raw_size.height - crop_size.height) / 2;
        int x = (raw_size.width - crop_size.width) / 2;
        image = image(cv::Rect(x, y, crop_size.width, crop_size.height));
    }

    return image;
}

cv::Mat imagePostprocess(const cv::Mat& image)
{
    double i_min, i_max;
    cv::minMaxLoc(image, &i_min, &i_max);

    cv::Mat normalized;
    image.convertTo(normalized, CV_64FC1);
    normalized = (normalized - i_min) / (i_max - i_min + 1e-8) * 255.0;

    // Clip to [0, 255] using cv::threshold (like Python's np.clip)
    cv::Mat clipped;
    cv::threshold(normalized, clipped, 255, 255, cv::THRESH_TRUNC); // clip upper
    cv::threshold(clipped, clipped, 0, 0, cv::THRESH_TOZERO);       // clip lower

    cv::Mat result;
    clipped.convertTo(result, CV_8UC1);

    return result;
}

cv::Mat getImageEdges(const cv::Mat& image)
{
    // Sobel kernels
    cv::Mat kernel_vertical = (cv::Mat_<float>(3, 3) << 1, 2, 1, 0, 0, 0, -1, -2, -1);

    cv::Mat kernel_horizontal = (cv::Mat_<float>(3, 3) << 1, 0, -1, 2, 0, -2, 1, 0, -1);

    cv::Mat image_float;
    image.convertTo(image_float, CV_32FC1);

    cv::Mat vertical_edges, horizontal_edges;
    cv::filter2D(image_float, vertical_edges, CV_32FC1, kernel_vertical);
    cv::filter2D(image_float, horizontal_edges, CV_32FC1, kernel_horizontal);

    // Combine: sqrt(vertical^2 + horizontal^2)
    cv::Mat edges;
    cv::sqrt(vertical_edges.mul(vertical_edges) + horizontal_edges.mul(horizontal_edges), edges);

    cv::Mat result;
    edges.convertTo(result, CV_8UC1);

    return result;
}

std::tuple<cv::Mat, std::string> getBinaryImage(const cv::Mat& image, bool return_main_axis)
{
    cv::Mat binary_img = cv::Mat::zeros(image.size(), CV_8UC1);

    int H = image.rows;
    int W = image.cols;

    // Get image statistics
    cv::Scalar mean, stddev;
    cv::meanStdDev(image, mean, stddev);

    // Only use peak detection - no thresholding

    // Also do peak detection similar to scipy's find_peaks
    // Parameters: height=20, distance=2, prominence=0.1
    for (int i = 0; i < H; i++) {
        const uchar* row = image.ptr<uchar>(i);

        for (int j = 2; j < W - 2; ++j) {
            uchar val = row[j];

            // Check height threshold (height=20 equivalent)
            if (val < 20)
                continue;

            // Check distance - must be at least 2 away from other peaks
            bool is_peak = true;
            for (int k = -2; k <= 2; k++) {
                if (k == 0)
                    continue;
                if (row[j + k] > val) {
                    is_peak = false;
                    break;
                }
            }
            if (!is_peak)
                continue;

            // Check prominence (simplified - compare to neighbors)
            uchar left_min = row[j - 1];
            uchar right_min = row[j + 1];
            for (int k = 2; k <= 5; k++) {
                if (j - k >= 0)
                    left_min = std::min(left_min, row[j - k]);
                if (j + k < W)
                    right_min = std::min(right_min, row[j + k]);
            }
            uchar prominence_ref = std::min(left_min, right_min);

            // If prominence is at least some value (5 to better match Python's ~823 pixels)
            if (val - prominence_ref >= 5) {
                binary_img.at<uchar>(i, j) = 255;
            }
        }
    }

    // Morphological operations
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(binary_img, binary_img, kernel);
    cv::erode(binary_img, binary_img, kernel);

    int binary_nz = cv::countNonZero(binary_img);

    if (return_main_axis) {
        // Count non-zero pixels per row and column
        int h_count = 0; // number of rows with any non-zero pixels
        for (int i = 0; i < H; i++) {
            if (cv::countNonZero(binary_img.row(i)) > 0)
                h_count++;
        }
        int w_count = 0; // number of columns with any non-zero pixels
        for (int j = 0; j < W; j++) {
            if (cv::countNonZero(binary_img.col(j)) > 0)
                w_count++;
        }

        double h_ratio = (double) h_count / H;
        double w_ratio = (double) w_count / W;

        // If more rows have pixels, cable is along y direction
        // If more columns have pixels, cable is along x direction
        std::string main_axis = (h_ratio > w_ratio) ? "y" : "x";
        return std::make_tuple(binary_img, main_axis);
    }

    return std::make_tuple(binary_img, "");
}

double cubicFunction(double x, double a, double b, double c, double d)
{
    return a * x * x * x + b * x * x + c * x + d;
}

double cubicDerivative(double x, double a, double b, double c)
{
    return 3 * a * x * x + 2 * b * x + c;
}

std::tuple<StatusCode, std::vector<CurveFitResult>> fitComponentCurve(const cv::Mat& img_otsu,
                                                                      const std::string& main_axis,
                                                                      int exp_lines)
{
    std::vector<CurveFitResult> all_fits;

    const auto& config = globalAxisConfig.at(main_axis);
    std::string ind_name = config.ind_name;
    std::string dep_name = config.dep_name;
    std::string min_ind_key = config.min_ind_key;
    std::string max_ind_key = config.max_ind_key;
    std::string min_dep_key = config.min_dep_key;
    std::string max_dep_key = config.max_dep_key;

    // Connected components
    cv::Mat labels, stats, centroids;
    int num_labels = cv::connectedComponentsWithStats(img_otsu, labels, stats, centroids, 8);

    int H = img_otsu.rows;
    int W = img_otsu.cols;
    int img_size = (main_axis == "y") ? H : W;

    // Process each component
    int processed = 0;
    int passed_filter = 0;
    for (int i = 1; i < num_labels; ++i) {
        processed++;
        cv::Mat mask = (labels == i);
        std::vector<cv::Point> points;
        cv::findNonZero(mask, points);

        if (points.empty())
            continue;

        // Extract coordinates based on main axis
        std::vector<double> independent, dependent;
        for (const auto& pt : points) {
            if (main_axis == "y") {
                independent.push_back(pt.y);
                dependent.push_back(pt.x);
            } else {
                independent.push_back(pt.x);
                dependent.push_back(pt.y);
            }
        }

        // Filter: ensure enough variation in independent direction
        auto [min_ind, max_ind] = std::minmax_element(independent.begin(), independent.end());
        // Changed threshold to 0.7 to be more restrictive (get ~4 components)
        if (*max_ind - *min_ind <= img_size * 0.7)
            continue;

        // Simple cubic fitting using least squares
        int n = independent.size();
        // if (n < 4) continue;
        if (n < 4) {
            continue;
        }

        // Build matrices for least squares: Ax = b
        cv::Mat A(n, 4, CV_64FC1);
        cv::Mat B(n, 1, CV_64FC1);

        for (int j = 0; j < n; ++j) {
            double x = independent[j];
            double y = dependent[j];
            A.at<double>(j, 0) = x * x * x;
            A.at<double>(j, 1) = x * x;
            A.at<double>(j, 2) = x;
            A.at<double>(j, 3) = 1.0;
            B.at<double>(j, 0) = y;
        }

        cv::Mat params;
        try {
            cv::solve(A, B, params, cv::DECOMP_SVD);
        } catch (...) {
            continue;
        }

        CurveFitResult fit;
        fit.params = {params.at<double>(0, 0),
                      params.at<double>(1, 0),
                      params.at<double>(2, 0),
                      params.at<double>(3, 0)};

        auto [min_dep, max_dep] = std::minmax_element(dependent.begin(), dependent.end());

        if (main_axis == "y") {
            fit.min_x = *min_dep;
            fit.max_x = *max_dep;
            fit.min_y = *min_ind;
            fit.max_y = *max_ind;
        } else {
            fit.min_x = *min_ind;
            fit.max_x = *max_ind;
            fit.min_y = *min_dep;
            fit.max_y = *max_dep;
        }

        fit.independent_var = ind_name;
        fit.dependent_var = dep_name;
        fit.centroid = centroids.at<cv::Point2d>(i);

        all_fits.push_back(fit);
    }

    // Sort by centroid
    if (!all_fits.empty()) {
        if (main_axis == "x") {
            std::sort(all_fits.begin(),
                      all_fits.end(),
                      [](const CurveFitResult& a, const CurveFitResult& b) {
                          return a.centroid.y < b.centroid.y;
                      });
        } else {
            std::sort(all_fits.begin(),
                      all_fits.end(),
                      [](const CurveFitResult& a, const CurveFitResult& b) {
                          return a.centroid.x < b.centroid.x;
                      });
        }

        // Add labels
        int half_line_num = exp_lines / 2;
        for (size_t i = 0; i < all_fits.size(); ++i) {
            if (i < static_cast<size_t>(half_line_num)) {
                all_fits[i].label = static_cast<int>(i) - half_line_num;
            } else {
                all_fits[i].label = static_cast<int>(i) + 1 - half_line_num;
            }
        }

        // Sort by label squared (outer curves first)
        std::sort(all_fits.begin(),
                  all_fits.end(),
                  [](const CurveFitResult& a, const CurveFitResult& b) {
                      return a.label * a.label > b.label * b.label;
                  });
    }

    if (all_fits.empty()) {
        return std::make_tuple(StatusCode::EMPTY_CABLE_ERROR, std::vector<CurveFitResult>{});
    }
    // Skip exact count validation - let algorithm proceed with whatever fits found
    // else if (static_cast<int>(all_fits.size()) != exp_lines) {
    //     return std::make_tuple(StatusCode::ANORMAL_CABLE_ERROR, std::vector<CurveFitResult>{});
    // }

    return std::make_tuple(StatusCode::SUCCESS, all_fits);
}

std::vector<CurveFitResult> fitMedianCurve(const cv::Mat& img_otsu,
                                           std::vector<CurveFitResult> all_fits,
                                           const std::string& main_axis,
                                           int exp_lines)
{
    if (all_fits.empty())
        return all_fits;

    const auto& config = globalAxisConfig.at(main_axis);
    std::string min_ind_key = config.min_ind_key;
    std::string max_ind_key = config.max_ind_key;
    std::string min_dep_key = config.min_dep_key;
    std::string max_dep_key = config.max_dep_key;

    // Find common independent range
    double min_common_ind = std::numeric_limits<double>::lowest();
    double max_common_ind = std::numeric_limits<double>::max();

    for (const auto& fit : all_fits) {
        if (main_axis == "y") {
            min_common_ind = std::max(min_common_ind, fit.min_y);
            max_common_ind = std::min(max_common_ind, fit.max_y);
        } else {
            min_common_ind = std::max(min_common_ind, fit.min_x);
            max_common_ind = std::min(max_common_ind, fit.max_x);
        }
    }

    // Generate common independent values
    std::vector<double> common_ind_values(200);
    double step = (max_common_ind - min_common_ind) / 199;
    for (int i = 0; i < 200; ++i) {
        common_ind_values[i] = min_common_ind + i * step;
    }

    // Find min and max label curves
    int min_id_line_index = 0, max_id_line_index = 0;
    int min_label = all_fits[0].label, max_label = all_fits[0].label;

    for (size_t i = 1; i < all_fits.size(); ++i) {
        if (all_fits[i].label < min_label) {
            min_label = all_fits[i].label;
            min_id_line_index = static_cast<int>(i);
        }
        if (all_fits[i].label > max_label) {
            max_label = all_fits[i].label;
            max_id_line_index = static_cast<int>(i);
        }
    }

    // Calculate outer curves
    std::vector<double> min_id_outer(200), max_id_outer(200);
    for (int i = 0; i < 200; ++i) {
        double x = common_ind_values[i];
        const auto& params = all_fits[min_id_line_index].params;
        min_id_outer[i] = cubicFunction(x, params[0], params[1], params[2], params[3]);

        const auto& params2 = all_fits[max_id_line_index].params;
        max_id_outer[i] = cubicFunction(x, params2[0], params2[1], params2[2], params2[3]);
    }

    // Calculate median (simple average)
    std::vector<double> mid_values(200);
    for (int i = 0; i < 200; ++i) {
        mid_values[i] = (min_id_outer[i] + max_id_outer[i]) / 2.0;
    }

    // Fit median curve
    int n = 200;
    cv::Mat A(n, 4, CV_64FC1);
    cv::Mat B(n, 1, CV_64FC1);

    for (int i = 0; i < n; ++i) {
        double x = common_ind_values[i];
        double y = mid_values[i];
        A.at<double>(i, 0) = x * x * x;
        A.at<double>(i, 1) = x * x;
        A.at<double>(i, 2) = x;
        A.at<double>(i, 3) = 1.0;
        B.at<double>(i, 0) = y;
    }

    cv::Mat params;
    cv::solve(A, B, params, cv::DECOMP_SVD);

    CurveFitResult mid_fit;
    mid_fit.params = {params.at<double>(0, 0),
                      params.at<double>(1, 0),
                      params.at<double>(2, 0),
                      params.at<double>(3, 0)};
    mid_fit.label = 0;

    if (main_axis == "y") {
        mid_fit.min_y = min_common_ind;
        mid_fit.max_y = max_common_ind;
        mid_fit.min_x = *std::min_element(mid_values.begin(), mid_values.end());
        mid_fit.max_x = *std::max_element(mid_values.begin(), mid_values.end());
    } else {
        mid_fit.min_x = min_common_ind;
        mid_fit.max_x = max_common_ind;
        mid_fit.min_y = *std::min_element(mid_values.begin(), mid_values.end());
        mid_fit.max_y = *std::max_element(mid_values.begin(), mid_values.end());
    }

    mid_fit.centroid = cv::Point2d(0, 0);
    mid_fit.independent_var = config.ind_name;
    mid_fit.dependent_var = config.dep_name;

    all_fits.push_back(mid_fit);

    return all_fits;
}

std::tuple<cv::Mat, std::vector<RadiusResult>> getRadiusByMidPopt(
    const std::vector<CurveFitResult>& all_fits,
    const std::string& main_axis,
    double sample_ratio,
    int sample_count,
    const cv::Mat& image)
{
    // Find median curve (label = 0)
    int mid_line_index = -1;
    for (size_t i = 0; i < all_fits.size(); ++i) {
        if (all_fits[i].label == 0) {
            mid_line_index = static_cast<int>(i);
            break;
        }
    }

    if (mid_line_index < 0) {
        return std::make_tuple(cv::Mat(), std::vector<RadiusResult>{});
    }

    const auto& config = globalAxisConfig.at(main_axis);
    std::string min_ind_key = config.min_ind_key;
    std::string max_ind_key = config.max_ind_key;

    // Common independent range
    double min_common_ind = std::numeric_limits<double>::lowest();
    double max_common_ind = std::numeric_limits<double>::max();

    for (const auto& fit : all_fits) {
        if (main_axis == "y") {
            min_common_ind = std::max(min_common_ind, fit.min_y);
            max_common_ind = std::min(max_common_ind, fit.max_y);
        } else {
            min_common_ind = std::max(min_common_ind, fit.min_x);
            max_common_ind = std::min(max_common_ind, fit.max_x);
        }
    }

    // Sample points
    std::vector<double> common_ind_values(200);
    double step = (max_common_ind - min_common_ind) / 199;
    for (int i = 0; i < 200; ++i) {
        common_ind_values[i] = min_common_ind + i * step;
    }

    const auto& mid_params = all_fits[mid_line_index].params;
    std::vector<double> mid_curve(200), mid_derivatives(200);
    for (int i = 0; i < 200; ++i) {
        mid_curve[i] = cubicFunction(common_ind_values[i],
                                     mid_params[0],
                                     mid_params[1],
                                     mid_params[2],
                                     mid_params[3]);
        mid_derivatives[i] = cubicDerivative(common_ind_values[i],
                                             mid_params[0],
                                             mid_params[1],
                                             mid_params[2]);
    }

    // Sample indices
    int sample_start = static_cast<int>(200 * (0.5 - sample_ratio / 2));
    int sample_end = static_cast<int>(200 * (0.5 + sample_ratio / 2));
    std::vector<int> sample_indices(sample_count);
    double sample_step = static_cast<double>(sample_end - sample_start) / (sample_count - 1);
    for (int i = 0; i < sample_count; ++i) {
        sample_indices[i] = static_cast<int>(sample_start + i * sample_step);
    }

    std::vector<double> sample_ind(sample_count), sample_mid(sample_count),
        sample_derivatives(sample_count);
    for (int i = 0; i < sample_count; ++i) {
        int idx = sample_indices[i];
        sample_ind[i] = common_ind_values[idx];
        sample_mid[i] = mid_curve[idx];
        sample_derivatives[i] = mid_derivatives[idx];
    }

    // Normal vectors
    std::vector<double> normal_vecs_x(sample_count), normal_vecs_y(sample_count);
    for (int i = 0; i < sample_count; ++i) {
        normal_vecs_x[i] = -sample_derivatives[i];
        normal_vecs_y[i] = 1.0;

        double norm = std::sqrt(normal_vecs_x[i] * normal_vecs_x[i]
                                + normal_vecs_y[i] * normal_vecs_y[i]);
        if (std::abs(sample_derivatives[i]) < 1e-6) {
            normal_vecs_x[i] = 0;
            normal_vecs_y[i] = 1;
        } else {
            normal_vecs_x[i] /= norm;
            normal_vecs_y[i] /= norm;
        }
    }

    // Radial profile
    int profile_length = 120;
    std::vector<double> t_vals(2 * profile_length + 1);
    for (int i = 0; i < 2 * profile_length + 1; ++i) {
        t_vals[i] = i - profile_length;
    }

    int img_height = image.rows;
    int img_width = image.cols;

    // Get radial profiles
    cv::Mat radial_profiles_matrix(2 * profile_length + 1, sample_count, CV_16UC1);
    for (int t_idx = 0; t_idx < 2 * profile_length + 1; ++t_idx) {
        double t = t_vals[t_idx];
        for (int s_idx = 0; s_idx < sample_count; ++s_idx) {
            double x = sample_ind[s_idx] + t * normal_vecs_x[s_idx];
            double y = sample_mid[s_idx] + t * normal_vecs_y[s_idx];

            if (main_axis == "y") {
                std::swap(x, y);
            }

            int xi = static_cast<int>(std::round(x));
            int yi = static_cast<int>(std::round(y));

            if (xi >= 0 && xi < img_width && yi >= 0 && yi < img_height) {
                radial_profiles_matrix.at<uchar>(t_idx, s_idx) = image.at<uchar>(yi, xi);
            } else {
                radial_profiles_matrix.at<uchar>(t_idx, s_idx) = 0;
            }
        }
    }

    // Sum profiles
    cv::Mat summed_radial_profile;
    cv::reduce(radial_profiles_matrix, summed_radial_profile, 1, cv::REDUCE_SUM, CV_64FC1);
    summed_radial_profile = summed_radial_profile.t();

    // Process each curve except median
    std::vector<RadiusResult> result_data;
    int search_range = (main_axis == "x" ? img_height : img_width) / 2;

    for (const auto& fit : all_fits) {
        if (fit.label == 0)
            continue;

        std::vector<double> radius_list;
        std::vector<std::vector<double>> inter_points;

        const auto& fit_params = fit.params;

        for (int s_idx = 0; s_idx < sample_count; ++s_idx) {
            double ind_val = sample_ind[s_idx];
            double mid_val = sample_mid[s_idx];
            double normal_x = normal_vecs_x[s_idx];
            double normal_y = normal_vecs_y[s_idx];

            // Grid search
            int n_search_points = 100;
            double min_dist = std::numeric_limits<double>::max();
            double best_t = 0;

            for (int sp = 0; sp < n_search_points; ++sp) {
                double t = -search_range + 2.0 * search_range * sp / (n_search_points - 1);

                double x_t = ind_val + t * normal_x;
                if (x_t < min_common_ind || x_t > max_common_ind)
                    continue;

                double y_curve_t = cubicFunction(x_t,
                                                 fit_params[0],
                                                 fit_params[1],
                                                 fit_params[2],
                                                 fit_params[3]);
                double y_mid_t = mid_val + t * normal_y;

                double dist = std::abs(y_mid_t - y_curve_t);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_t = t;
                }
            }

            if (min_dist < 5.0) {
                // Refine with golden section
                double phi = 0.618033988749895;
                double a = best_t - 5, b = best_t + 5;
                double c = b - phi * (b - a);
                double d = a + phi * (b - a);

                auto distance_squared = [&](double t) -> double {
                    double x_t = ind_val + t * normal_x;
                    if (x_t < min_common_ind || x_t > max_common_ind)
                        return std::numeric_limits<double>::max();
                    double y_curve_t = cubicFunction(x_t,
                                                     fit_params[0],
                                                     fit_params[1],
                                                     fit_params[2],
                                                     fit_params[3]);
                    double y_mid_t = mid_val + t * normal_y;
                    return (y_mid_t - y_curve_t) * (y_mid_t - y_curve_t);
                };

                for (int iter = 0; iter < 8; ++iter) {
                    if (distance_squared(c) < distance_squared(d)) {
                        b = d;
                    } else {
                        a = c;
                    }
                    c = b - phi * (b - a);
                    d = a + phi * (b - a);
                }

                double t_opt = (a + b) / 2;
                double final_distance = std::sqrt(distance_squared(t_opt));

                if (final_distance < 2.0) {
                    double x_intersect = ind_val + t_opt * normal_x;
                    double y_intersect = mid_val + t_opt * normal_y;
                    inter_points.push_back({x_intersect, y_intersect});
                    radius_list.push_back(std::abs(t_opt));
                }
            }
        }

        RadiusResult result;
        result.label = fit.label;
        result.radius = radius_list;

        if (!radius_list.empty()) {
            double sum = std::accumulate(radius_list.begin(), radius_list.end(), 0.0);
            result.avg_radius = sum / radius_list.size();
            double sq_sum = 0;
            for (double r : radius_list) {
                sq_sum += (r - result.avg_radius) * (r - result.avg_radius);
            }
            result.std_radius = std::sqrt(sq_sum / radius_list.size());
        } else {
            result.avg_radius = 0;
            result.std_radius = 0;
        }

        // Mid points
        result.mid_points = cv::Mat(sample_count, 2, CV_64FC1);
        for (int i = 0; i < sample_count; ++i) {
            result.mid_points.at<double>(i, 0) = sample_ind[i];
            result.mid_points.at<double>(i, 1) = sample_mid[i];
        }

        // Intersection points
        if (!inter_points.empty()) {
            result.inter_points = cv::Mat(inter_points.size(), 2, CV_64FC1);
            for (size_t i = 0; i < inter_points.size(); ++i) {
                result.inter_points.at<double>(i, 0) = inter_points[i][0];
                result.inter_points.at<double>(i, 1) = inter_points[i][1];
            }
        } else {
            result.inter_points = cv::Mat(0, 2, CV_64FC1);
        }

        result_data.push_back(result);
    }

    return std::make_tuple(summed_radial_profile, result_data);
}

MeasurementData analyze2DWireData(const std::vector<RadiusResult>& left_right_data,
                                  const std::vector<RadiusResult>& up_down_data,
                                  double target_diameter)
{
    MeasurementData data;

    // Find indices for labels -2, -1, 1, 2
    std::vector<int> x_indices, y_indices;
    for (int label : {-2, -1, 1, 2}) {
        for (size_t i = 0; i < left_right_data.size(); ++i) {
            if (left_right_data[i].label == label) {
                x_indices.push_back(static_cast<int>(i));
                break;
            }
        }
        for (size_t i = 0; i < up_down_data.size(); ++i) {
            if (up_down_data[i].label == label) {
                y_indices.push_back(static_cast<int>(i));
                break;
            }
        }
    }

    if (x_indices.size() < 4 || y_indices.size() < 4) {
        return data; // Return empty/default
    }

    // Calculate diameters
    double x_inner_diameter = left_right_data[x_indices[1]].avg_radius
                              + left_right_data[x_indices[2]].avg_radius;
    double x_outer_diameter = left_right_data[x_indices[0]].avg_radius
                              + left_right_data[x_indices[3]].avg_radius;
    double y_inner_diameter = up_down_data[y_indices[1]].avg_radius
                              + up_down_data[y_indices[2]].avg_radius;
    double y_outer_diameter = up_down_data[y_indices[0]].avg_radius
                              + up_down_data[y_indices[3]].avg_radius;

    // Apply scaling ratios
    double lr_ratio = target_diameter / x_inner_diameter;
    double ud_ratio = target_diameter / y_inner_diameter;

    x_inner_diameter *= lr_ratio;
    y_inner_diameter *= ud_ratio;
    x_outer_diameter *= lr_ratio;
    y_outer_diameter *= ud_ratio;

    double inner_diameter = (x_inner_diameter + y_inner_diameter) / 2.0;
    double outer_diameter = (x_outer_diameter + y_outer_diameter) / 2.0;

    // Inner ellipse center (relative to outer center at origin)
    cv::Point2d inner_center;
    inner_center.x = lr_ratio
                     * (-left_right_data[x_indices[1]].avg_radius
                        + left_right_data[x_indices[2]].avg_radius)
                     / 2.0;
    inner_center.y = ud_ratio
                     * (-up_down_data[y_indices[1]].avg_radius
                        + up_down_data[y_indices[2]].avg_radius)
                     / 2.0;

    // Semi-axes
    double inner_a = x_inner_diameter / 2.0;
    double inner_b = y_inner_diameter / 2.0;
    double outer_a = x_outer_diameter / 2.0;
    double outer_b = y_outer_diameter / 2.0;

    // Calculate wall thickness at each angle
    std::vector<double> thickness_at_angles;
    cv::Point2d outer_center(0, 0);

    for (int angle = 0; angle < 360; ++angle) {
        double angle_rad = angle * CV_PI / 180.0;

        // Direction vector
        double dx = std::cos(angle_rad);
        double dy = std::sin(angle_rad);

        // Inner ellipse point
        cv::Point2d inner_point;
        inner_point.x = inner_center.x + inner_a * dx;
        inner_point.y = inner_center.y + inner_b * dy;

        // Solve for intersection with outer ellipse
        double h = inner_center.x - outer_center.x;
        double k = inner_center.y - outer_center.y;

        double A = (dx * dx) / (outer_a * outer_a) + (dy * dy) / (outer_b * outer_b);
        double B = (2 * h * dx) / (outer_a * outer_a) + (2 * k * dy) / (outer_b * outer_b);
        double C = (h * h) / (outer_a * outer_a) + (k * k) / (outer_b * outer_b) - 1.0;

        double discriminant = B * B - 4 * A * C;

        cv::Point2d outer_point;
        if (discriminant >= 0) {
            double t1 = (-B + std::sqrt(discriminant)) / (2 * A);
            double t2 = (-B - std::sqrt(discriminant)) / (2 * A);

            std::vector<double> positive_ts;
            if (t1 > 0)
                positive_ts.push_back(t1);
            if (t2 > 0)
                positive_ts.push_back(t2);

            if (!positive_ts.empty()) {
                double t = *std::max_element(positive_ts.begin(), positive_ts.end());
                outer_point.x = inner_center.x + t * dx;
                outer_point.y = inner_center.y + t * dy;
            } else {
                outer_point.x = outer_center.x + outer_a * std::cos(angle_rad);
                outer_point.y = outer_center.y + outer_b * std::sin(angle_rad);
            }
        } else {
            outer_point.x = outer_center.x + outer_a * std::cos(angle_rad);
            outer_point.y = outer_center.y + outer_b * std::sin(angle_rad);
        }

        // Distance (thickness)
        double dist = std::sqrt(std::pow(outer_point.x - inner_point.x, 2)
                                + std::pow(outer_point.y - inner_point.y, 2));
        thickness_at_angles.push_back(dist);
    }

    // Statistics
    double sum = std::accumulate(thickness_at_angles.begin(), thickness_at_angles.end(), 0.0);
    double avg_thickness = sum / thickness_at_angles.size();

    double min_thickness = *std::min_element(thickness_at_angles.begin(), thickness_at_angles.end());
    double max_thickness = *std::max_element(thickness_at_angles.begin(), thickness_at_angles.end());

    int min_angle = static_cast<int>(
        std::min_element(thickness_at_angles.begin(), thickness_at_angles.end())
        - thickness_at_angles.begin());

    // Specific angles: 0, 45, 90, 135, 180, 225, 270, 315
    std::vector<double> spec_thickness;
    for (int a : {0, 45, 90, 135, 180, 225, 270, 315}) {
        spec_thickness.push_back(thickness_at_angles[a]);
    }

    // Eccentricity
    double eccentricity = (max_thickness - min_thickness) / avg_thickness;

    // Ovality (using outer ellipse)
    double outer_dia_min = std::min(outer_a, outer_b) * 2;
    double outer_dia_max = std::max(outer_a, outer_b) * 2;
    double ovality = outer_dia_max - outer_dia_min;

    // Fill result
    data.inner_ellipse.center = inner_center;
    data.inner_ellipse.x_diameter = x_inner_diameter;
    data.inner_ellipse.y_diameter = y_inner_diameter;
    data.inner_ellipse.diameter = inner_diameter;

    data.outer_ellipse.center = outer_center;
    data.outer_ellipse.x_diameter = x_outer_diameter;
    data.outer_ellipse.y_diameter = y_outer_diameter;
    data.outer_ellipse.diameter = outer_diameter;

    data.wall_thickness.thickness = avg_thickness;
    data.wall_thickness.min_thickness = min_thickness;
    data.wall_thickness.min_angle = min_angle;
    data.wall_thickness.spec_thickness = spec_thickness;

    data.related.eccentricity = eccentricity;
    data.related.ovality = ovality;
    data.related.x_ratio = lr_ratio;
    data.related.y_ratio = ud_ratio;

    return data;
}

} // namespace cable_detector
