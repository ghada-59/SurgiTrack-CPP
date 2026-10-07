#ifndef SURGITRACK_SAFETY_GEOFENCE_HPP
#define SURGITRACK_SAFETY_GEOFENCE_HPP

#include <Eigen/Dense>

#include <string>
#include <vector>

namespace surgitrack {

struct CriticalStructure {
    std::string name;
    Eigen::Vector3d center;
    double safety_radius_mm;
};

enum class SafetyStatus {
    SAFE,
    WARNING_APPROACHING,
    CRITICAL_VIOLATION
};

class SafetyGeofence {
public:
    explicit SafetyGeofence(double warning_buffer_mm = 3.0);

    void addCriticalStructure(
        const std::string& name,
        const Eigen::Vector3d& center,
        double radius_mm);

    SafetyStatus evaluatePosition(
        const Eigen::Vector3d& tool_position,
        std::string& out_alert_msg) const;

private:
    double warning_buffer_mm_;
    std::vector<CriticalStructure> structures_;
};

} // namespace surgitrack

#endif
