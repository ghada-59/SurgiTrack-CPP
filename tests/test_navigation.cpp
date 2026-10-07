#include <cmath>
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
