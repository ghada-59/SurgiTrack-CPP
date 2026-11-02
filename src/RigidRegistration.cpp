#include "surgitrack/RigidRegistration.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace surgitrack {
namespace {

constexpr double kCollinearRatio = 1e-6;
constexpr double kWarningRatio = 1e-2;
constexpr double kRotationTolerance = 1e-9;

void requireCloudShape(
    const Eigen::Matrix3Xd& moving_mm,
    const Eigen::Matrix3Xd& fixed_mm) {
    if (moving_mm.rows() != 3 || fixed_mm.rows() != 3) {
        throw std::invalid_argument(
            "Point clouds must be 3 x N (one point per column).");
    }
    if (moving_mm.cols() != fixed_mm.cols()) {
        throw std::invalid_argument(
            "Moving and fixed point clouds must have the same number of points.");
    }
    if (moving_mm.cols() < 3) {
        throw std::invalid_argument(
            "Rigid registration requires at least 3 points.");
    }
    if (!moving_mm.allFinite() || !fixed_mm.allFinite()) {
        throw std::invalid_argument(
            "Point coordinates must be finite.");
    }
}

void requirePairing(
    const Eigen::Matrix3Xd& moving_mm,
    const Eigen::Matrix3Xd& fixed_mm,
    double tolerance_mm) {
    if (!std::isfinite(tolerance_mm) || tolerance_mm < 0.0) {
        throw std::invalid_argument(
            "Pairing tolerance must be finite and non-negative.");
    }

    const Eigen::Index count = moving_mm.cols();
    for (Eigen::Index i = 0; i < count; ++i) {
        for (Eigen::Index j = i + 1; j < count; ++j) {
            const double moving_distance_mm =
                (moving_mm.col(i) - moving_mm.col(j)).norm();
            const double fixed_distance_mm =
                (fixed_mm.col(i) - fixed_mm.col(j)).norm();
            if (std::abs(moving_distance_mm - fixed_distance_mm) > tolerance_mm) {
                throw std::invalid_argument(
                    "Point pairing is inconsistent with a rigid motion "
                    "(pairwise distances differ).");
            }
        }
    }
}

void requireProperRotation(const Eigen::Matrix3d& rotation) {
    const Eigen::Matrix3d orthogonality =
        rotation.transpose() * rotation - Eigen::Matrix3d::Identity();
    if (orthogonality.cwiseAbs().maxCoeff() > kRotationTolerance
        || std::abs(rotation.determinant() - 1.0) > kRotationTolerance) {
        throw std::runtime_error(
            "Registration produced a matrix that is not a proper rotation.");
    }
}

} // namespace

RigidRegistrationResult registerRigid(
    const Eigen::Matrix3Xd& moving_mm,
    const Eigen::Matrix3Xd& fixed_mm,
    const RigidRegistrationOptions& options) {
    requireCloudShape(moving_mm, fixed_mm);
    if (options.check_pairing) {
        requirePairing(moving_mm, fixed_mm, options.pairing_tolerance_mm);
    }

    // Centroids, then the cross-covariance
    // H = Σ (p_i - p̄) (q_i - q̄)ᵀ = Pc * Qcᵀ.
    const Eigen::Vector3d moving_centroid_mm = moving_mm.rowwise().mean();
    const Eigen::Vector3d fixed_centroid_mm = fixed_mm.rowwise().mean();
    const Eigen::Matrix3Xd centered_moving_mm =
        moving_mm.colwise() - moving_centroid_mm;
    const Eigen::Matrix3Xd centered_fixed_mm =
        fixed_mm.colwise() - fixed_centroid_mm;
    const Eigen::Matrix3d covariance_mm2 =
        centered_moving_mm * centered_fixed_mm.transpose();

    // H = U Σ Vᵀ. Singular values are ordered σ1 >= σ2 >= σ3.
    const Eigen::JacobiSVD<Eigen::Matrix3d> svd(
        covariance_mm2, Eigen::ComputeFullU | Eigen::ComputeFullV);
    const Eigen::Vector3d singular_values = svd.singularValues();
    if (!singular_values.allFinite() || !(singular_values(0) > 0.0)
        || singular_values(1) < kCollinearRatio * singular_values(0)) {
        throw std::invalid_argument(
            "Fiducials are collinear: registration is indeterminate.");
    }

    RigidRegistrationResult result;
    result.nearly_aligned_warning =
        singular_values(1) < kWarningRatio * singular_values(0);
    // σ3 ≈ 0 means the fiducials lie in a plane. The fit is still defined
    // once the reflection term below is applied.
    result.coplanar_warning =
        singular_values(2) < kWarningRatio * singular_values(0);

    if (options.emit_warnings && result.nearly_aligned_warning) {
        std::cerr << "Warning: fiducials are nearly collinear "
                     "(sigma2 < 1e-2 * sigma1); the rotation about that "
                     "axis is weakly constrained.\n";
    }
    if (options.emit_warnings && result.coplanar_warning) {
        std::cerr << "Warning: fiducials are coplanar "
                     "(sigma3 < 1e-2 * sigma1); target error grows faster "
                     "out of that plane.\n";
    }

    // R = V diag(1, 1, det(V Uᵀ)) Uᵀ forces det(R) = +1 (no reflection).
    const double orientation =
        (svd.matrixV() * svd.matrixU().transpose()).determinant() > 0.0
            ? 1.0
            : -1.0;
    const Eigen::Matrix3d correction =
        Eigen::Vector3d(1.0, 1.0, orientation).asDiagonal();
    result.rotation =
        svd.matrixV() * correction * svd.matrixU().transpose();
    result.translation_mm =
        fixed_centroid_mm - result.rotation * moving_centroid_mm;
    requireProperRotation(result.rotation);

    const Eigen::Matrix3Xd mapped_mm =
        (result.rotation * moving_mm).colwise() + result.translation_mm;
    const Eigen::Matrix3Xd residual_mm = mapped_mm - fixed_mm;
    result.fre_rms_mm = std::sqrt(residual_mm.colwise().squaredNorm().mean());
    if (!std::isfinite(result.fre_rms_mm)) {
        throw std::runtime_error(
            "Fiducial registration error is not finite.");
    }
    return result;
}

} // namespace surgitrack
