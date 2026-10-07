#include "surgitrack/SafetyGeofence.hpp"
#include <sstream>

namespace surgitrack {

SafetyGeofence::SafetyGeofence(double warning_buffer_mm) 
    : warning_buffer_mm_(warning_buffer_mm) {}

void SafetyGeofence::addCriticalStructure(const std::string& name, const Eigen::Vector3d& center, double radius_mm) {
    structures_.push_back({name, center, radius_mm});
}

SecurityStatus SafetyGeofence::evaluatePosition(const Eigen::Vector3d& tool_position, std::string& out_alert_msg) const {
    SecurityStatus worst_status = SecurityStatus::SAFE;
    out_alert_msg = "SAFE: Tool within nominal workspace.";

    for (const auto& s : structures_) {
        double dist = (tool_position - s.center).norm();
        
        if (dist <= s.safety_radius_mm) {
            std::ostringstream ss;
            ss << "CRITICAL ALERT: Surgical tool inside no-fly zone '" 
               << s.name << "' (Dist: " << dist << " mm <= Radius: " << s.safety_radius_mm << " mm)";
            out_alert_msg = ss.str();
            return SecurityStatus::CRITICAL_VIOLATION;
        } else if (dist <= s.safety_radius_mm + warning_buffer_mm_) {
            std::ostringstream ss;
            ss << "WARNING: Immediate proximity to critical structure '" 
               << s.name << "' (Dist: " << dist << " mm)";
            out_alert_msg = ss.str();
            worst_status = SecurityStatus::WARNING_APPROACHING;
        }
    }

    return worst_status;
}

} // namespace surgitrack