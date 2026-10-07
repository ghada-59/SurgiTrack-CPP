#ifndef SURGITRACK_COORDINATE_TRANSFORM_HPP
#define SURGITRACK_COORDINATE_TRANSFORM_HPP

#include <Eigen/Dense>

namespace surgitrack {

/**
 * @brief Handles 3D rigid body transformations (Camera Space -> Patient Space -> Scanner Space)
 */
class CoordinateTransform {
public:
    CoordinateTransform();
    explicit CoordinateTransform(const Eigen::Matrix4d& matrix);

    static CoordinateTransform createTranslation(double tx, double ty, double tz);
    static CoordinateTransform createRotationX(double angle_rad);
    static CoordinateTransform createRotationY(double angle_rad);
    static CoordinateTransform createRotationZ(double angle_rad);

    Eigen::Vector3d transformPoint(const Eigen::Vector3d& point) const;
    CoordinateTransform combine(const CoordinateTransform& other) const;
    CoordinateTransform inverse() const;

    const Eigen::Matrix4d& getMatrix() const { return matrix_; }

private:
    Eigen::Matrix4d matrix_;
};

} // namespace surgitrack

#endif