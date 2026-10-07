#include "surgitrack/CoordinateTransform.hpp"

#include <cmath>

namespace surgitrack {

CoordinateTransform::CoordinateTransform()
    : matrix_(Eigen::Matrix4d::Identity()) {}

CoordinateTransform::CoordinateTransform(const Eigen::Matrix4d& matrix)
    : matrix_(matrix) {}

CoordinateTransform CoordinateTransform::createTranslation(
    double tx, double ty, double tz) {
    Eigen::Matrix4d matrix = Eigen::Matrix4d::Identity();
    matrix(0, 3) = tx;
    matrix(1, 3) = ty;
    matrix(2, 3) = tz;
    return CoordinateTransform(matrix);
}

CoordinateTransform CoordinateTransform::createRotationX(double angle_rad) {
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
    const Eigen::Vector4d homogeneous_point(point.x(), point.y(), point.z(), 1.0);
    const Eigen::Vector4d transformed_point = matrix_ * homogeneous_point;
    return transformed_point.head<3>();
}

CoordinateTransform CoordinateTransform::combine(
    const CoordinateTransform& other) const {
    return CoordinateTransform(matrix_ * other.matrix_);
}

CoordinateTransform CoordinateTransform::inverse() const {
    return CoordinateTransform(matrix_.inverse());
}

} // namespace surgitrack
