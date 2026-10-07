#include "surgitrack/SafetyGeofence.hpp"

#include <limits>
#include <sstream>
#include <stdexcept>

namespace surgitrack {

SafetyGeofence::SafetyGeofence(double warning_buffer_mm)
    : warning_buffer_mm_(warning_buffer_mm) {
    if (warning_buffer_mm < 0.0) {
        throw std::invalid_argument("Warning buffer cannot be negative.");
    }
}

void SafetyGeofence::addCriticalStructure(
    const std::string& name,
    const Eigen::Vector3d& center,
    double radius_mm) {
    if (name.empty()) {
        throw std::invalid_argument("Critical structure name cannot be empty.");
    }
    if (radius_mm < 0.0) {
        throw std::invalid_argument("Safety radius cannot be negative.");
    }

    structures_.push_back({name, center, radius_mm});
}

SafetyStatus SafetyGeofence::evaluatePosition(
    const Eigen::Vector3d& tool_position,
    std::string& out_alert_msg) const {
    SafetyStatus worst_status = SafetyStatus::SAFE;
    double nearest_warning_distance = std::numeric_limits<double>::infinity();

    out_alert_msg = "SAFE: Tool within nominal workspace.";

    for (const auto& structure : structures_) {
        const double distance = (tool_position - structure.center).norm();

        if (distance <= structure.safety_radius_mm) {
            std::ostringstream message;
            message << "CRITICAL ALERT: Tool entered protected zone '"
                    << structure.name << "' (distance: " << distance
                    << " mm, radius: " << structure.safety_radius_mm << " mm).";
            out_alert_msg = message.str();
            return SafetyStatus::CRITICAL_VIOLATION;
        }

        if (distance <= structure.safety_radius_mm + warning_buffer_mm_
            && distance < nearest_warning_distance) {
            std::ostringstream message;
            message << "WARNING: Tool is approaching protected structure '"
                    << structure.name << "' (distance: " << distance << " mm).";
            out_alert_msg = message.str();
            nearest_warning_distance = distance;
            worst_status = SafetyStatus::WARNING_APPROACHING;
        }
    }

    return worst_status;
}

} // namespace surgitrack
