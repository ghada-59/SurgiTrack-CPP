#include "surgitrack/CoordinateTransform.hpp"
#include <cmath>

namespace surgitrack {

CoordinateTransform::CoordinateTransform() {
    matrix_ = Eigen::Matrix4d::Identity();
}

CoordinateTransform::CoordinateTransform(const Eigen::Matrix4d& matrix) : matrix_(matrix) {}

CoordinateTransform CoordinateTransform::createTranslation(double tx, double ty, double tz) {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    m(0, 3) = tx;
    m(1, 3) = ty;
    m(2, 3) = tz;
    return CoordinateTransform(m);
}

CoordinateTransform CoordinateTransform::createRotationX(double angle_rad) {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    double c = std::cos(angle_rad);
    double s = std::sin(angle_rad);
    m(1, 1) = c;  m(1, 2) = -s;
    m(2, 1) = s;  m(2, 2) = c;
    return CoordinateTransform(m);
}

CoordinateTransform CoordinateTransform::createRotationY(double angle_rad) {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    double c = std::cos(angle_rad);
    double s = std::sin(angle_rad);
    m(0, 0) = c;  m(0, 2) = s;
    m(2, 0) = -s; m(2, 2) = c;
    return CoordinateTransform(m);
}

CoordinateTransform CoordinateTransform::createRotationZ(double angle_rad) {
    Eigen::Matrix4d m = Eigen::Matrix4d::Identity();
    double c = std::cos(angle_rad);
    double s = std::sin(angle_rad);
    m(0, 0) = c;  m(0, 1) = -s;
    m(1, 0) = s;  m(1, 1) = c;
    return CoordinateTransform(m);
}

Eigen::Vector3d CoordinateTransform::transformPoint(const Eigen::Vector3d& point) const {
    Eigen::Vector4d point_h(point.x(), point.y(), point.z(), 1.0);
    Eigen::Vector4d transformed_h = matrix_ * point_h;
    return transformed_h.head<3>();
}

CoordinateTransform CoordinateTransform::combine(const CoordinateTransform& other) const {
    return CoordinateTransform(this->matrix_ * other.matrix_);
}

CoordinateTransform CoordinateTransform::inverse() const {
    return CoordinateTransform(this->matrix_.inverse());
}

} // namespace surgitrack