#ifndef SURGITRACK_RIGID_REGISTRATION_HPP
#define SURGITRACK_RIGID_REGISTRATION_HPP

#include <Eigen/Dense>

namespace surgitrack {

/**
 * @brief Result of a point-based rigid registration.
 *
 * Convention: one point is one column of an Eigen::Matrix3Xd.
 * All coordinates and the translation are in millimetres.
 * rotation maps a moving point into the fixed frame:
 * fixed ≈ rotation * moving + translation_mm.
 */
struct RigidRegistrationResult {
    Eigen::Matrix3d rotation = Eigen::Matrix3d::Identity();
    Eigen::Vector3d translation_mm = Eigen::Vector3d::Zero();
    double fre_rms_mm = 0.0;
    /// sigma2 < 1e-2 * sigma1. The fit is still returned.
    bool nearly_aligned_warning = false;
    /// sigma3 < 1e-2 * sigma1. Coplanar fiducials remain a valid registration.
    bool coplanar_warning = false;
};

/**
 * @brief Optional controls around the Kabsch core.
 *
 * pairing_tolerance_mm is the maximum absolute difference, in millimetres,
 * allowed between a pairwise distance in the moving cloud and the same
 * pair in the fixed cloud. A rigid motion preserves those distances, so a
 * larger gap means a bad correspondence or mixed units.
 */
struct RigidRegistrationOptions {
    double pairing_tolerance_mm = 1e-4;
    bool check_pairing = true;
    bool emit_warnings = true;
};

/**
 * @brief Rigid registration of two labelled point clouds (Kabsch / Arun).
 *
 * @param moving_mm 3 x N cloud, one point per column, millimetres.
 * @param fixed_mm  3 x N cloud in the same column order, millimetres.
 *
 * The rigid map minimises the RMS residual of
 * fixed_mm ≈ R * moving_mm + t.
 * R is forced to a proper rotation (det(R) = +1).
 *
 * Throws std::invalid_argument when the clouds are unusable: fewer than
 * three points, a size mismatch, a non-finite coordinate, a pairwise
 * distance that breaks the rigid-motion check, or collinear fiducials
 * (sigma2 < 1e-6 * sigma1), including a cloud with no spatial spread.
 */
RigidRegistrationResult registerRigid(
    const Eigen::Matrix3Xd& moving_mm,
    const Eigen::Matrix3Xd& fixed_mm,
    const RigidRegistrationOptions& options = {});

} // namespace surgitrack

#endif
