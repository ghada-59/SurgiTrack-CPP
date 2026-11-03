#include "surgitrack/RegistrationMetrics.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace surgitrack {
namespace {

void requireFiniteVector(const Eigen::Vector3d& value, const char* name) {
    if (!value.allFinite()) {
        throw std::invalid_argument(std::string(name) + " must be finite.");
    }
}

} // namespace

double freRmsMm(
    const Eigen::Matrix3Xd& estimated_mm,
    const Eigen::Matrix3Xd& reference_mm) {
    if (estimated_mm.rows() != 3 || reference_mm.rows() != 3) {
        throw std::invalid_argument("FRE clouds must be 3 x N.");
    }
    if (estimated_mm.cols() != reference_mm.cols() || estimated_mm.cols() < 1) {
        throw std::invalid_argument(
            "FRE requires two non-empty clouds with the same number of points.");
    }
    if (!estimated_mm.allFinite() || !reference_mm.allFinite()) {
        throw std::invalid_argument("FRE coordinates must be finite.");
    }

    const Eigen::Matrix3Xd residual_mm = estimated_mm - reference_mm;
    const double fre_mm = std::sqrt(residual_mm.colwise().squaredNorm().mean());
    if (!std::isfinite(fre_mm)) {
        throw std::runtime_error("FRE is not finite.");
    }
    return fre_mm;
}

double treMm(
    const Eigen::Vector3d& estimated_mm,
    const Eigen::Vector3d& truth_mm) {
    requireFiniteVector(estimated_mm, "Estimated target");
    requireFiniteVector(truth_mm, "True target");
    const double error_mm = (estimated_mm - truth_mm).norm();
    if (!std::isfinite(error_mm)) {
        throw std::runtime_error("TRE is not finite.");
    }
    return error_mm;
}

FiducialMoments fiducialMomentsMm(const Eigen::Matrix3Xd& fiducials_mm) {
    if (fiducials_mm.rows() != 3 || fiducials_mm.cols() < 3) {
        throw std::invalid_argument(
            "Fiducial moments require a 3 x N cloud with N >= 3.");
    }
    if (!fiducials_mm.allFinite()) {
        throw std::invalid_argument("Fiducial coordinates must be finite.");
    }

    FiducialMoments moments;
    moments.centroid_mm = fiducials_mm.rowwise().mean();
    const Eigen::Matrix3Xd centered_mm = fiducials_mm.colwise() - moments.centroid_mm;
    const double count = static_cast<double>(fiducials_mm.cols());
    const Eigen::Matrix3d covariance_mm2 =
        (centered_mm * centered_mm.transpose()) / count;

    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> solver(covariance_mm2);
    if (solver.info() != Eigen::Success) {
        throw std::runtime_error("Principal-axis decomposition failed.");
    }

    moments.axes = solver.eigenvectors();
    const Eigen::Vector3d eigenvalues_mm2 = solver.eigenvalues();
    const double trace_mm2 = eigenvalues_mm2.sum();
    for (int axis = 0; axis < 3; ++axis) {
        // Distance to axis k ignores the coordinate along that axis, so
        // f_k² = trace(covariance) - λ_k.
        const double axis_distance_mm2 = trace_mm2 - eigenvalues_mm2(axis);
        if (!(axis_distance_mm2 > 0.0) || !std::isfinite(axis_distance_mm2)) {
            throw std::invalid_argument(
                "Fiducials are degenerate: a principal-axis distance is zero.");
        }
        moments.mean_squared_axis_distance_mm2(axis) = axis_distance_mm2;
    }
    return moments;
}

double fitzpatrickTreSquaredMm2(
    double fle_rms_mm,
    int fiducial_count,
    const FiducialMoments& moments,
    const Eigen::Vector3d& target_mm) {
    if (!std::isfinite(fle_rms_mm) || fle_rms_mm < 0.0) {
        throw std::invalid_argument("FLE must be finite and non-negative.");
    }
    if (fiducial_count < 3) {
        throw std::invalid_argument(
            "Fitzpatrick formula requires at least 3 fiducials.");
    }
    requireFiniteVector(target_mm, "Target");
    requireFiniteVector(moments.centroid_mm, "Fiducial centroid");
    if (!moments.axes.allFinite()
        || !moments.mean_squared_axis_distance_mm2.allFinite()) {
        throw std::invalid_argument("Fiducial moments must be finite.");
    }

    const Eigen::Vector3d relative_mm = target_mm - moments.centroid_mm;
    const double radius_squared_mm2 = relative_mm.squaredNorm();
    double axis_term = 0.0;
    for (int axis = 0; axis < 3; ++axis) {
        const double axis_scale_mm2 = moments.mean_squared_axis_distance_mm2(axis);
        if (!(axis_scale_mm2 > 0.0)) {
            throw std::invalid_argument(
                "Fitzpatrick formula needs a positive fiducial spread about each axis.");
        }
        const double along_axis_mm = relative_mm.dot(moments.axes.col(axis));
        const double distance_squared_mm2 =
            radius_squared_mm2 - along_axis_mm * along_axis_mm;
        const double clamped_mm2 =
            distance_squared_mm2 > 0.0 ? distance_squared_mm2 : 0.0;
        axis_term += clamped_mm2 / axis_scale_mm2;
    }

    const double count = static_cast<double>(fiducial_count);
    return (fle_rms_mm * fle_rms_mm / count) * (1.0 + axis_term / 3.0);
}

} // namespace surgitrack
