#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "surgitrack/CoordinateTransform.hpp"
#include "surgitrack/SafetyGeofence.hpp"

using namespace surgitrack;

namespace {
constexpr double kPi = 3.14159265358979323846;
}

TEST(CoordinateTransformTest, DefaultIsIdentity) {
    const CoordinateTransform transform;
    const Eigen::Vector3d point(1.0, -2.0, 3.5);

    EXPECT_TRUE(transform.transformPoint(point).isApprox(point, 1e-12));
}

TEST(CoordinateTransformTest, TranslationPrecision) {
    const auto transform =
        CoordinateTransform::createTranslation(10.0, -5.0, 2.5);

    const Eigen::Vector3d result =
        transform.transformPoint(Eigen::Vector3d::Zero());

    EXPECT_NEAR(result.x(), 10.0, 1e-9);
    EXPECT_NEAR(result.y(), -5.0, 1e-9);
    EXPECT_NEAR(result.z(), 2.5, 1e-9);
}

TEST(CoordinateTransformTest, RotationZ90Degrees) {
    const auto transform =
        CoordinateTransform::createRotationZ(kPi / 2.0);

    const Eigen::Vector3d result =
        transform.transformPoint(Eigen::Vector3d(1.0, 0.0, 0.0));

    EXPECT_NEAR(result.x(), 0.0, 1e-9);
    EXPECT_NEAR(result.y(), 1.0, 1e-9);
    EXPECT_NEAR(result.z(), 0.0, 1e-9);
}

TEST(CoordinateTransformTest, RotationX90Degrees) {
    const auto transform =
        CoordinateTransform::createRotationX(kPi / 2.0);

    const Eigen::Vector3d result =
        transform.transformPoint(Eigen::Vector3d(0.0, 1.0, 0.0));

    EXPECT_NEAR(result.x(), 0.0, 1e-9);
    EXPECT_NEAR(result.y(), 0.0, 1e-9);
    EXPECT_NEAR(result.z(), 1.0, 1e-9);
}

TEST(CoordinateTransformTest, RotationY90Degrees) {
    const auto transform =
        CoordinateTransform::createRotationY(kPi / 2.0);

    const Eigen::Vector3d result =
        transform.transformPoint(Eigen::Vector3d(0.0, 0.0, 1.0));

    EXPECT_NEAR(result.x(), 1.0, 1e-9);
    EXPECT_NEAR(result.y(), 0.0, 1e-9);
    EXPECT_NEAR(result.z(), 0.0, 1e-9);
}

TEST(CoordinateTransformTest, CompositionAppliesOtherFirst) {
    const auto translation =
        CoordinateTransform::createTranslation(10.0, 0.0, 0.0);
    const auto rotation =
        CoordinateTransform::createRotationZ(kPi / 2.0);

    const auto combined = translation.combine(rotation);
    const Eigen::Vector3d result =
        combined.transformPoint(Eigen::Vector3d(1.0, 0.0, 0.0));

    EXPECT_NEAR(result.x(), 10.0, 1e-9);
    EXPECT_NEAR(result.y(), 1.0, 1e-9);
    EXPECT_NEAR(result.z(), 0.0, 1e-9);
}

TEST(CoordinateTransformTest, InverseRestoresPoint) {
    const auto transform =
        CoordinateTransform::createTranslation(10.0, -4.0, 2.0)
            .combine(CoordinateTransform::createRotationZ(0.35));

    const Eigen::Vector3d original(4.0, -2.0, 7.0);
    const Eigen::Vector3d transformed = transform.transformPoint(original);

    EXPECT_TRUE(
        transform.inverse().transformPoint(transformed).isApprox(original, 1e-9));
}

TEST(CoordinateTransformTest, RejectsNonFiniteFactoryInputs) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    EXPECT_THROW(CoordinateTransform::createTranslation(inf, 0.0, 0.0),
                 std::invalid_argument);
    EXPECT_THROW(CoordinateTransform::createRotationX(nan),
                 std::invalid_argument);
    EXPECT_THROW(CoordinateTransform::createRotationY(inf),
                 std::invalid_argument);
    EXPECT_THROW(CoordinateTransform::createRotationZ(nan),
                 std::invalid_argument);
}

TEST(CoordinateTransformTest, RejectsInvalidHomogeneousMatrices) {
    Eigen::Matrix4d non_finite = Eigen::Matrix4d::Identity();
    non_finite(0, 0) = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW(static_cast<void>(CoordinateTransform(non_finite)), std::invalid_argument);

    Eigen::Matrix4d invalid_bottom_row = Eigen::Matrix4d::Identity();
    invalid_bottom_row(3, 0) = 1.0;
    EXPECT_THROW(static_cast<void>(CoordinateTransform(invalid_bottom_row)), std::invalid_argument);
}

TEST(CoordinateTransformTest, RejectsNonFinitePoints) {
    const double inf = std::numeric_limits<double>::infinity();
    const CoordinateTransform transform;
    EXPECT_THROW(transform.transformPoint(Eigen::Vector3d(0.0, inf, 1.0)),
                 std::invalid_argument);
}

TEST(CoordinateTransformTest, RejectsSingularMatrixInverse) {
    Eigen::Matrix4d singular = Eigen::Matrix4d::Zero();
    singular(3, 3) = 1.0;
    const CoordinateTransform transform(singular);
    EXPECT_THROW(transform.inverse(), std::invalid_argument);
}

TEST(SafetyGeofenceTest, SafePosition) {
    SafetyGeofence fence(2.0);
    fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), 10.0);

    std::string message;
    const SafetyStatus status =
        fence.evaluatePosition(Eigen::Vector3d(20.0, 0.0, 0.0), message);

    EXPECT_EQ(status, SafetyStatus::SAFE);
    EXPECT_NE(message.find("SAFE"), std::string::npos);
}

TEST(SafetyGeofenceTest, WarningPosition) {
    SafetyGeofence fence(2.0);
    fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), 10.0);

    std::string message;
    const SafetyStatus status =
        fence.evaluatePosition(Eigen::Vector3d(11.0, 0.0, 0.0), message);

    EXPECT_EQ(status, SafetyStatus::WARNING_APPROACHING);
    EXPECT_NE(message.find("Artery"), std::string::npos);
}

TEST(SafetyGeofenceTest, DetectsCriticalViolation) {
    SafetyGeofence fence(2.0);
    fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), 10.0);

    std::string message;
    const SafetyStatus status =
        fence.evaluatePosition(Eigen::Vector3d(5.0, 0.0, 0.0), message);

    EXPECT_EQ(status, SafetyStatus::CRITICAL_VIOLATION);
    EXPECT_NE(message.find("CRITICAL"), std::string::npos);
}

TEST(SafetyGeofenceTest, CriticalViolationTakesPriority) {
    SafetyGeofence fence(5.0);
    fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), 10.0);
    fence.addCriticalStructure("Nerve", Eigen::Vector3d(30.0, 0.0, 0.0), 5.0);

    std::string message;
    const SafetyStatus status =
        fence.evaluatePosition(Eigen::Vector3d(5.0, 0.0, 0.0), message);

    EXPECT_EQ(status, SafetyStatus::CRITICAL_VIOLATION);
}

TEST(SafetyGeofenceTest, RejectsInvalidParameters) {
    EXPECT_THROW(SafetyGeofence(-1.0), std::invalid_argument);

    SafetyGeofence fence;
    EXPECT_THROW(
        fence.addCriticalStructure("", Eigen::Vector3d::Zero(), 5.0),
        std::invalid_argument);
    EXPECT_THROW(
        fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), -1.0),
        std::invalid_argument);
}


TEST(SafetyGeofenceTest, RejectsNonFiniteWarningBuffer) {
    EXPECT_THROW(
        SafetyGeofence(std::numeric_limits<double>::quiet_NaN()),
        std::invalid_argument);
    EXPECT_THROW(
        SafetyGeofence(std::numeric_limits<double>::infinity()),
        std::invalid_argument);
}

TEST(SafetyGeofenceTest, RejectsNonFiniteStructureParameters) {
    SafetyGeofence fence;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    EXPECT_THROW(
        fence.addCriticalStructure("Artery", Eigen::Vector3d(nan, 0.0, 0.0), 5.0),
        std::invalid_argument);
    EXPECT_THROW(
        fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), nan),
        std::invalid_argument);
}

TEST(SafetyGeofenceTest, HandlesExactSafetyBoundaries) {
    SafetyGeofence fence(2.0);
    fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), 10.0);
    std::string message;

    EXPECT_EQ(
        fence.evaluatePosition(Eigen::Vector3d(10.0, 0.0, 0.0), message),
        SafetyStatus::CRITICAL_VIOLATION);
    EXPECT_EQ(
        fence.evaluatePosition(Eigen::Vector3d(12.0, 0.0, 0.0), message),
        SafetyStatus::WARNING_APPROACHING);
    EXPECT_EQ(
        fence.evaluatePosition(Eigen::Vector3d(12.0001, 0.0, 0.0), message),
        SafetyStatus::SAFE);
}

TEST(SafetyGeofenceTest, ChoosesWarningClosestToProtectedBoundary) {
    SafetyGeofence fence(5.0);
    fence.addCriticalStructure("Large zone", Eigen::Vector3d::Zero(), 10.0);
    fence.addCriticalStructure("Small zone", Eigen::Vector3d(16.0, 0.0, 0.0), 2.0);

    std::string message;
    const SafetyStatus status =
        fence.evaluatePosition(Eigen::Vector3d(12.5, 0.0, 0.0), message);

    EXPECT_EQ(status, SafetyStatus::WARNING_APPROACHING);
    EXPECT_NE(message.find("Small zone"), std::string::npos);
}

TEST(SafetyGeofenceTest, RejectsOverflowingRadiusAndWarningBuffer) {
    const double max = std::numeric_limits<double>::max();
    SafetyGeofence fence(max / 2.0);

    EXPECT_THROW(
        fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), max),
        std::invalid_argument);
}

TEST(SafetyGeofenceTest, RejectsOverflowingDistanceCalculation) {
    SafetyGeofence fence;
    fence.addCriticalStructure("Artery", Eigen::Vector3d::Zero(), 1.0);
    std::string message;
    const double max = std::numeric_limits<double>::max();

    EXPECT_THROW(
        fence.evaluatePosition(Eigen::Vector3d(max, max, max), message),
        std::overflow_error);
}

TEST(SafetyGeofenceTest, RejectsNonFiniteToolPosition) {
    SafetyGeofence fence;
    std::string message;
    const double nan = std::numeric_limits<double>::quiet_NaN();

    EXPECT_THROW(
        fence.evaluatePosition(Eigen::Vector3d(nan, 0.0, 0.0), message),
        std::invalid_argument);
}
