#include "surgitrack/CoordinateTransform.hpp"

#include <cmath>
#include <stdexcept>

#include <Eigen/LU>

namespace surgitrack {
namespace {

void requireFinite(double value, const char* name) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(std::string(name) + " must be finite.");
    }
}

void requireFiniteVector(const Eigen::Vector3d& value, const char* name) {
    if (!value.allFinite()) {
        throw std::invalid_argument(std::string(name) + " must contain only finite values.");
    }
}

} // namespace

CoordinateTransform::CoordinateTransform()
    : matrix_(Eigen::Matrix4d::Identity()) {}

CoordinateTransform::CoordinateTransform(const Eigen::Matrix4d& matrix)
    : matrix_(matrix) {
    if (!matrix_.allFinite()) {
        throw std::invalid_argument("Transformation matrix must contain only finite values.");
    }

    const Eigen::RowVector4d expected_bottom_row(0.0, 0.0, 0.0, 1.0);
    if (!matrix_.row(3).isApprox(expected_bottom_row, 1e-12)) {
        throw std::invalid_argument(
            "Transformation matrix must use a valid homogeneous bottom row [0, 0, 0, 1].");
    }
}

CoordinateTransform CoordinateTransform::createTranslation(
    double tx, double ty, double tz) {
    requireFinite(tx, "Translation x");
    requireFinite(ty, "Translation y");
    requireFinite(tz, "Translation z");

    Eigen::Matrix4d matrix = Eigen::Matrix4d::Identity();
    matrix(0, 3) = tx;
    matrix(1, 3) = ty;
    matrix(2, 3) = tz;
    return CoordinateTransform(matrix);
}

CoordinateTransform CoordinateTransform::createRotationX(double angle_rad) {
    requireFinite(angle_rad, "Rotation angle");
    Eigen::Matrix4d matrix = Eigen::Matrix4d::Identity();
    const double c = std::cos(angle_rad);
    const double s = std::sin(angle_rad);

    matrix(1, 1) = c;
    matrix(1, 2) = -s;
    matrix(2, 1) = s;
    matrix(2, 2) = c;

    return CoordinateTransform(matrix);
}

CoordinateTransform CoordinateTransform::createRotationY(double angle_rad) {
    requireFinite(angle_rad, "Rotation angle");
    Eigen::Matrix4d matrix = Eigen::Matrix4d::Identity();
    const double c = std::cos(angle_rad);
    const double s = std::sin(angle_rad);

    matrix(0, 0) = c;
    matrix(0, 2) = s;
    matrix(2, 0) = -s;
    matrix(2, 2) = c;

    return CoordinateTransform(matrix);
}

CoordinateTransform CoordinateTransform::createRotationZ(double angle_rad) {
    requireFinite(angle_rad, "Rotation angle");
    Eigen::Matrix4d matrix = Eigen::Matrix4d::Identity();
    const double c = std::cos(angle_rad);
    const double s = std::sin(angle_rad);

    matrix(0, 0) = c;
    matrix(0, 1) = -s;
    matrix(1, 0) = s;
    matrix(1, 1) = c;

    return CoordinateTransform(matrix);
}

Eigen::Vector3d CoordinateTransform::transformPoint(
    const Eigen::Vector3d& point) const {
    requireFiniteVector(point, "Point");

    const Eigen::Vector4d homogeneous_point(point.x(), point.y(), point.z(), 1.0);
    const Eigen::Vector4d transformed_point = matrix_ * homogeneous_point;
    const Eigen::Vector3d result = transformed_point.head<3>();
    requireFiniteVector(result, "Transformed point");
    return result;
}

CoordinateTransform CoordinateTransform::combine(
    const CoordinateTransform& other) const {
    return CoordinateTransform(matrix_ * other.matrix_);
}

CoordinateTransform CoordinateTransform::inverse() const {
    if (!matrix_.fullPivLu().isInvertible()) {
        throw std::invalid_argument("Cannot invert a singular transformation matrix.");
    }
    return CoordinateTransform(matrix_.inverse());
}

} // namespace surgitrack
