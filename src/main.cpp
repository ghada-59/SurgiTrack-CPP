#include <iomanip>
#include <iostream>
#include <vector>

#include "surgitrack/CoordinateTransform.hpp"
#include "surgitrack/SafetyGeofence.hpp"

using namespace surgitrack;

namespace {

const char* toString(SafetyStatus status) {
    switch (status) {
        case SafetyStatus::SAFE:
            return "SAFE";
        case SafetyStatus::WARNING_APPROACHING:
            return "WARNING";
        case SafetyStatus::CRITICAL_VIOLATION:
            return "CRITICAL";
    }
    return "UNKNOWN";
}

} // namespace

int main() {
    std::cout << "========================================================
";
    std::cout << "   SurgiTrack-CPP: 3D Medical Navigation Simulation    
";
    std::cout << "   Coordinate Transformation & Safety Geofencing       
";
    std::cout << "========================================================

";

    SafetyGeofence geofence(3.0);
    geofence.addCriticalStructure(
        "Femoral Artery", Eigen::Vector3d(50.0, 100.0, 10.0), 5.0);
    geofence.addCriticalStructure(
        "Sciatic Nerve", Eigen::Vector3d(20.0, 80.0, -15.0), 4.0);

    // Composition: rotation is applied first, then translation.
    const auto cameraToPatient =
        CoordinateTransform::createTranslation(10.0, 20.0, 5.0)
            .combine(CoordinateTransform::createRotationZ(0.1));

    const std::vector<Eigen::Vector3d> camera_positions = {
        {0.0, 0.0, 0.0},
        {25.0, 50.0, 2.0},
        {40.0, 75.0, 4.5},
        {44.0, 75.0, 4.5},
        {48.0, 75.0, 5.0}
    };

    std::cout << std::fixed << std::setprecision(2);

    for (std::size_t i = 0; i < camera_positions.size(); ++i) {
        const Eigen::Vector3d patient_position =
            cameraToPatient.transformPoint(camera_positions[i]);

        std::string alert;
        const SafetyStatus status =
            geofence.evaluatePosition(patient_position, alert);

        std::cout << "[Step " << i + 1 << "] "
                  << "Camera: (" << camera_positions[i].x() << ", "
                  << camera_positions[i].y() << ", "
                  << camera_positions[i].z() << ") mm"
                  << " -> Patient: (" << patient_position.x() << ", "
                  << patient_position.y() << ", "
                  << patient_position.z() << ") mm
";

        std::cout << "         Status: " << toString(status) << "
";
        std::cout << "         " << alert << "
";
        std::cout << "--------------------------------------------------------
";
    }

    return 0;
}
