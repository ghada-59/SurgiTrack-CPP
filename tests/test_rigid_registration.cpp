#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "surgitrack/RigidRegistration.hpp"

using namespace surgitrack;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kTolerance = 1e-9;

Eigen::Matrix3d rotationZ(double angle_rad) {
    const double cosine = std::cos(angle_rad);
    const double sine = std::sin(angle_rad);
    Eigen::Matrix3d rotation;
    rotation << cosine, -sine, 0.0,
                sine, cosine, 0.0,
                0.0, 0.0, 1.0;
    return rotation;
}

// Four non-coplanar fiducials, in millimetres, with unequal pair distances.
Eigen::Matrix3Xd spreadCloudMm() {
    Eigen::Matrix3Xd cloud(3, 4);
    cloud << 0.0, 30.0, 0.0, 5.0,
             0.0, 0.0, 40.0, 8.0,
             0.0, 0.0, 0.0, 25.0;
    return cloud;
}

void expectMatrixNear(const Eigen::Matrix3d& actual, const Eigen::Matrix3d& expected) {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            EXPECT_NEAR(actual(row, col), expected(row, col), kTolerance);
        }
    }
}

void expectVectorNear(const Eigen::Vector3d& actual, const Eigen::Vector3d& expected) {
    EXPECT_NEAR(actual.x(), expected.x(), kTolerance);
    EXPECT_NEAR(actual.y(), expected.y(), kTolerance);
    EXPECT_NEAR(actual.z(), expected.z(), kTolerance);
}

void expectProperRotation(const Eigen::Matrix3d& rotation) {
    EXPECT_TRUE((rotation.transpose() * rotation)
                    .isApprox(Eigen::Matrix3d::Identity(), kTolerance));
    EXPECT_NEAR(rotation.determinant(), 1.0, kTolerance);
}

void expectInvalidArgument(
    const Eigen::Matrix3Xd& moving_mm,
    const Eigen::Matrix3Xd& fixed_mm,
    const char* message_fragment) {
    try {
        static_cast<void>(registerRigid(moving_mm, fixed_mm));
        FAIL() << "expected std::invalid_argument";
    } catch (const std::invalid_argument& error) {
        EXPECT_NE(std::string(error.what()).find(message_fragment), std::string::npos)
            << error.what();
    }
}

} // namespace

TEST(RigidRegistrationTest, RecoversKnownPoseWithoutNoise) {
    const Eigen::Matrix3Xd moving_mm = spreadCloudMm();
    const Eigen::Matrix3d expected_rotation = rotationZ(kPi / 2.0);
    const Eigen::Vector3d expected_translation_mm(10.0, -2.0, 3.5);
    const Eigen::Matrix3Xd fixed_mm =
        (expected_rotation * moving_mm).colwise() + expected_translation_mm;

    const RigidRegistrationResult result = registerRigid(moving_mm, fixed_mm);

    expectMatrixNear(result.rotation, expected_rotation);
    expectVectorNear(result.translation_mm, expected_translation_mm);
    EXPECT_NEAR(result.fre_rms_mm, 0.0, kTolerance);
    expectProperRotation(result.rotation);
    EXPECT_FALSE(result.nearly_aligned_warning);
    EXPECT_FALSE(result.coplanar_warning);

    // 90 degrees about z must send this fiducial to one exact image.
    const Eigen::Vector3d source_mm = moving_mm.col(1);
    const Eigen::Vector3d mapped_mm = result.rotation * source_mm + result.translation_mm;
    expectVectorNear(mapped_mm, Eigen::Vector3d(10.0, 28.0, 3.5));
}

TEST(RigidRegistrationTest, IdentityHasZeroTranslationAndFre) {
    const Eigen::Matrix3Xd cloud_mm = spreadCloudMm();
    const RigidRegistrationResult result = registerRigid(cloud_mm, cloud_mm);

    expectMatrixNear(result.rotation, Eigen::Matrix3d::Identity());
    expectVectorNear(result.translation_mm, Eigen::Vector3d::Zero());
    EXPECT_NEAR(result.fre_rms_mm, 0.0, kTolerance);
    expectProperRotation(result.rotation);
}

TEST(RigidRegistrationTest, TranslationOf10Millimetres) {
    const Eigen::Matrix3Xd moving_mm = spreadCloudMm();
    const Eigen::Vector3d expected_translation_mm(10.0, 0.0, 0.0);
    const Eigen::Matrix3Xd fixed_mm = moving_mm.colwise() + expected_translation_mm;

    const RigidRegistrationResult result = registerRigid(moving_mm, fixed_mm);

    expectMatrixNear(result.rotation, Eigen::Matrix3d::Identity());
    EXPECT_NEAR(result.translation_mm.x(), 10.0, kTolerance);
    EXPECT_NEAR(result.translation_mm.y(), 0.0, kTolerance);
    EXPECT_NEAR(result.translation_mm.z(), 0.0, kTolerance);
    EXPECT_NEAR(result.fre_rms_mm, 0.0, kTolerance);
}

TEST(RigidRegistrationTest, MirroredCloudStaysAProperRotation) {
    Eigen::Matrix3Xd moving_mm(3, 4);
    moving_mm << 0.0, 20.0, 0.0, 0.0,
                 0.0, 0.0, 15.0, 0.0,
                 0.0, 0.0, 0.0, 10.0;
    Eigen::Matrix3Xd fixed_mm = moving_mm;
    fixed_mm.row(2) *= -1.0;

    const RigidRegistrationResult result = registerRigid(moving_mm, fixed_mm);

    expectProperRotation(result.rotation);
    EXPECT_GT(result.fre_rms_mm, 1e-3);
}

TEST(RigidRegistrationTest, PermutedPairingIsRejected) {
    const Eigen::Matrix3Xd moving_mm = spreadCloudMm();
    Eigen::Matrix3Xd fixed_mm = moving_mm;
    fixed_mm.col(1).swap(fixed_mm.col(2));

    expectInvalidArgument(moving_mm, fixed_mm, "pairing");
}

TEST(RigidRegistrationTest, PermutedPairingHasLargeFreWhenCheckIsDisabled) {
    const Eigen::Matrix3Xd moving_mm = spreadCloudMm();
    Eigen::Matrix3Xd fixed_mm = moving_mm;
    fixed_mm.col(1).swap(fixed_mm.col(2));

    RigidRegistrationOptions options;
    options.check_pairing = false;
    const RigidRegistrationResult result = registerRigid(moving_mm, fixed_mm, options);

    EXPECT_GT(result.fre_rms_mm, 1.0);
    expectProperRotation(result.rotation);
}

TEST(RigidRegistrationTest, MixedMillimetreAndMetreScaleIsRejected) {
    const Eigen::Matrix3Xd moving_mm = spreadCloudMm();
    const Eigen::Matrix3Xd fixed_m = moving_mm / 1000.0;
    expectInvalidArgument(moving_mm, fixed_m, "pairing");
}

TEST(RigidRegistrationTest, CollinearFiducialsThrow) {
    Eigen::Matrix3Xd collinear_mm(3, 3);
    collinear_mm << 0.0, 10.0, 25.0,
                    0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0;
    expectInvalidArgument(collinear_mm, collinear_mm, "collinear");

    Eigen::Matrix3Xd diagonal_mm(3, 4);
    diagonal_mm << 0.0, 1.0, 2.0, 4.0,
                   0.0, 2.0, 4.0, 8.0,
                   0.0, 3.0, 6.0, 12.0;
    expectInvalidArgument(diagonal_mm, diagonal_mm, "collinear");
}

TEST(RigidRegistrationTest, CoincidentPointsThrow) {
    Eigen::Matrix3Xd coincident_mm(3, 3);
    coincident_mm << 4.0, 4.0, 4.0,
                     -2.0, -2.0, -2.0,
                     7.0, 7.0, 7.0;
    expectInvalidArgument(coincident_mm, coincident_mm, "collinear");
}

TEST(RigidRegistrationTest, FewerThanThreePointsThrow) {
    Eigen::Matrix3Xd empty_mm(3, 0);
    expectInvalidArgument(empty_mm, empty_mm, "at least 3");

    Eigen::Matrix3Xd one_mm(3, 1);
    one_mm << 1.0, 2.0, 3.0;
    expectInvalidArgument(one_mm, one_mm, "at least 3");

    Eigen::Matrix3Xd two_mm(3, 2);
    two_mm << 0.0, 10.0,
              0.0, -4.0,
              0.0, 6.0;
    expectInvalidArgument(two_mm, two_mm, "at least 3");
}

TEST(RigidRegistrationTest, SizeMismatchThrows) {
    const Eigen::Matrix3Xd moving_mm = spreadCloudMm();
    const Eigen::Matrix3Xd fixed_mm = moving_mm.leftCols(3);
    expectInvalidArgument(moving_mm, fixed_mm, "same number");
}

TEST(RigidRegistrationTest, NonFiniteCoordinatesThrow) {
    Eigen::Matrix3Xd moving_mm = spreadCloudMm();
    Eigen::Matrix3Xd fixed_mm = spreadCloudMm();
    moving_mm(0, 1) = std::numeric_limits<double>::quiet_NaN();
    expectInvalidArgument(moving_mm, fixed_mm, "finite");

    moving_mm = spreadCloudMm();
    fixed_mm(2, 0) = std::numeric_limits<double>::infinity();
    expectInvalidArgument(moving_mm, fixed_mm, "finite");
}

TEST(RigidRegistrationTest, CoplanarFiducialsWarnAndStillRegister) {
    Eigen::Matrix3Xd plane_mm(3, 4);
    plane_mm << 0.0, 40.0, 0.0, 40.0,
                0.0, 0.0, 30.0, 30.0,
                0.0, 0.0, 0.0, 0.0;

    const RigidRegistrationResult result = registerRigid(plane_mm, plane_mm);

    EXPECT_TRUE(result.coplanar_warning);
    EXPECT_FALSE(result.nearly_aligned_warning);
    EXPECT_NEAR(result.fre_rms_mm, 0.0, kTolerance);
    expectMatrixNear(result.rotation, Eigen::Matrix3d::Identity());
    expectProperRotation(result.rotation);
}

TEST(RigidRegistrationTest, NearlyCollinearFiducialsWarn) {
    // Spread along x is about 120 mm. The cross-axis wiggle is a fraction
    // of a millimetre, so sigma2 / sigma1 falls in the warning band.
    Eigen::Matrix3Xd cloud_mm(3, 4);
    cloud_mm << 0.0, 40.0, 80.0, 120.0,
                0.0, 0.30, -0.20, 0.10,
                0.0, -0.10, 0.20, 0.05;

    const RigidRegistrationResult result = registerRigid(cloud_mm, cloud_mm);

    EXPECT_TRUE(result.nearly_aligned_warning);
    EXPECT_NEAR(result.fre_rms_mm, 0.0, kTolerance);
    expectProperRotation(result.rotation);
}

TEST(RigidRegistrationTest, RejectsInvalidPairingTolerance) {
    const Eigen::Matrix3Xd cloud_mm = spreadCloudMm();
    RigidRegistrationOptions negative;
    negative.pairing_tolerance_mm = -0.1;
    EXPECT_THROW(static_cast<void>(registerRigid(cloud_mm, cloud_mm, negative)),
                 std::invalid_argument);

    RigidRegistrationOptions non_finite;
    non_finite.pairing_tolerance_mm = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW(static_cast<void>(registerRigid(cloud_mm, cloud_mm, non_finite)),
                 std::invalid_argument);
}

TEST(RigidRegistrationTest, ThreePointTriangleRecoversPlanarRotation) {
    Eigen::Matrix3Xd moving_mm(3, 3);
    moving_mm << 0.0, 50.0, 10.0,
                 0.0, 0.0, 40.0,
                 0.0, 0.0, 0.0;
    const Eigen::Matrix3d expected_rotation = rotationZ(kPi / 2.0);
    const Eigen::Vector3d expected_translation_mm(10.0, 0.0, 0.0);
    const Eigen::Matrix3Xd fixed_mm =
        (expected_rotation * moving_mm).colwise() + expected_translation_mm;

    const RigidRegistrationResult result = registerRigid(moving_mm, fixed_mm);

    expectMatrixNear(result.rotation, expected_rotation);
    expectVectorNear(result.translation_mm, expected_translation_mm);
    EXPECT_NEAR(result.fre_rms_mm, 0.0, kTolerance);
    EXPECT_TRUE(result.coplanar_warning);
    expectProperRotation(result.rotation);

    const Eigen::Vector3d mapped_mm =
        result.rotation * moving_mm.col(1) + result.translation_mm;
    expectVectorNear(mapped_mm, Eigen::Vector3d(10.0, 50.0, 0.0));
}
