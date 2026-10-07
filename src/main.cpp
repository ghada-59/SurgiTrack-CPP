#include <iostream>
#include <vector>
#include <iomanip>
#include "surgitrack/CoordinateTransform.hpp"
#include "surgitrack/SafetyGeofence.hpp"

using namespace surgitrack;

int main() {
    std::cout << "========================================================\n";
    std::cout << "   SurgiTrack-CPP: Medical Navigation & Safety Suite   \n";
    std::cout << "   Sub-Millimeter Surgical Tracking Simulator           \n";
    std::cout << "========================================================\n\n";

    // Define critical anatomical structures
    SafetyGeofence geofence(3.0); // 3 mm warning buffer
    geofence.addCriticalStructure("Femoral Artery", Eigen::Vector3d(50.0, 100.0, 10.0), 5.0);
    geofence.addCriticalStructure("Sciatic Nerve", Eigen::Vector3d(20.0, 80.0, -15.0), 4.0);

    // Frame transformation: Tracking Camera -> Patient Space
    CoordinateTransform cameraToPatient = CoordinateTransform::combine(
        CoordinateTransform::createTranslation(10.0, 20.0, 5.0),
        CoordinateTransform::createRotationZ(0.1)
    );

    // Simulated optical tracking trajectory (Camera Space coordinates in mm)
    std::vector<Eigen::Vector3d> camera_positions = {
        Eigen::Vector3d(0.0, 0.0, 0.0),
        Eigen::Vector3d(25.0, 50.0, 2.0),
        Eigen::Vector3d(35.0, 75.0, 4.0),
        Eigen::Vector3d(37.5, 77.0, 4.5) // Hazardous trajectory path
    };

    std::cout << std::fixed << std::setprecision(2);

    for (size_t i = 0; i < camera_positions.size(); ++i) {
        Eigen::Vector3d pt_patient = cameraToPatient.transformPoint(camera_positions[i]);
        std::string alert;
        SecurityStatus status = geofence.evaluatePosition(pt_patient, alert);

        std::cout << "[Step " << i + 1 << "] Cam: (" 
                  << camera_positions[i].x() << ", " << camera_positions[i].y() << ", " << camera_positions[i].z() << ") mm"
                  << " -> Patient: (" 
                  << pt_patient.x() << ", " << pt_patient.y() << ", " << pt_patient.z() << ") mm\n";
        
        std::cout << "         " << alert << "\n";
        std::cout << "--------------------------------------------------------\n";
    }

    return 0;
}