#include <gtest/gtest.h>
#include "surgitrack/CoordinateTransform.hpp"
#include "surgitrack/SafetyGeofence.hpp"

using namespace surgitrack;

// Test geometric transformation precision
TEST(CoordinateTransformTest, TranslationPrecision) {
    auto t = CoordinateTransform::createTranslation(10.0, -5.0, 2.5);
    Eigen::Vector3d origin(0.0, 0.0, 0.0);
    Eigen::Vector3d result = t.transformPoint(origin);

    EXPECT_NEAR(result.x(), 10.0, 1e-5);
    EXPECT_NEAR(result.y(), -5.0, 1e-5);
    EXPECT_NEAR(result.z(), 2.5, 1e-5);
}

// Test geofence violation detection
TEST(SafetyGeofenceTest, DetectsCriticalViolation) {
    SafetyGeofence fence(2.0);
    fence.addCriticalStructure("Artery", Eigen::Vector3d(0.0, 0.0, 0.0), 10.0);

    std::string msg;
    SecurityStatus status = fence.evaluatePosition(Eigen::Vector3d(5.0, 0.0, 0.0), msg);

    EXPECT_EQ(status, SecurityStatus::CRITICAL_VIOLATION);
}