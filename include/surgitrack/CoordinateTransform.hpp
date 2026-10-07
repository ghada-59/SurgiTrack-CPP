#ifndef SURGITRACK_COORDINATE_TRANSFORM_HPP
#define SURGITRACK_COORDINATE_TRANSFORM_HPP

#include <Eigen/Dense>

namespace surgitrack {

/**
 * @brief Represents a 3D homogeneous transformation matrix.
 *
 * The class supports translations, rotations, point transformation,
 * composition and inversion for a small medical-navigation simulation.
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

    /**
     * @brief Compose this transform with another transform.
     *
     * The returned matrix is this->matrix() * other.matrix().
     * When applied to a point, "other" is therefore applied first,
     * followed by "this".
     */
    CoordinateTransform combine(const CoordinateTransform& other) const;

    CoordinateTransform inverse() const;

    const Eigen::Matrix4d& getMatrix() const { return matrix_; }

private:
    Eigen::Matrix4d matrix_;
};

} // namespace surgitrack

#endif
