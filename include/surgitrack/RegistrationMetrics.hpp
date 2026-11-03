#ifndef SURGITRACK_REGISTRATION_METRICS_HPP
#define SURGITRACK_REGISTRATION_METRICS_HPP

#include <Eigen/Dense>

namespace surgitrack {

/**
 * @brief Principal-axis description of a fiducial cloud.
 *
 * axes_mm columns are unit principal axes. mean_squared_axis_distance_mm2(k)
 * is f_k² in Fitzpatrick's formula: the mean squared distance of the
 * fiducials from axis k. Coordinates are millimetres.
 */
struct FiducialMoments {
    Eigen::Vector3d centroid_mm = Eigen::Vector3d::Zero();
    Eigen::Matrix3d axes = Eigen::Matrix3d::Identity();
    Eigen::Vector3d mean_squared_axis_distance_mm2 = Eigen::Vector3d::Zero();
};

/**
 * @brief RMS fiducial registration error, in millimetres.
 *
 * Both clouds are 3 x N, one point per column, in the same order.
 * FRE = sqrt(mean_i ||estimated_i - reference_i||²).
 */
double freRmsMm(
    const Eigen::Matrix3Xd& estimated_mm,
    const Eigen::Matrix3Xd& reference_mm);

/// Euclidean target registration error, in millimetres.
double treMm(
    const Eigen::Vector3d& estimated_mm,
    const Eigen::Vector3d& truth_mm);

/// Centroid and principal-axis distances used by the TRE formula.
FiducialMoments fiducialMomentsMm(const Eigen::Matrix3Xd& fiducials_mm);

/**
 * @brief Fitzpatrick expected squared TRE, in mm².
 *
 * ⟨TRE²⟩ ≈ (FLE² / N) * (1 + (1/3) Σ_k d_k² / f_k²)
 * (PLAN.md section 3.5). FLE is the total 3D RMS localisation error.
 * d_k is the target's distance to principal axis k. f_k² is
 * moments.mean_squared_axis_distance_mm2(k).
 */
double fitzpatrickTreSquaredMm2(
    double fle_rms_mm,
    int fiducial_count,
    const FiducialMoments& moments,
    const Eigen::Vector3d& target_mm);

} // namespace surgitrack

#endif
