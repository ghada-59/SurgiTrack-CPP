# SurgiTrack-CPP — C++ 3D Medical Navigation & Safety Simulation

A portfolio-oriented C++17 project demonstrating 3D coordinate transformations and geometric safety checks for a simplified medical-navigation scenario.

> **Scope:** educational software simulation only. It is not a clinical, diagnostic, surgical, or real-time tracking system.

## What this project demonstrates

- C++17 object-oriented design
- 3D homogeneous-coordinate transformations with Eigen
- Translation and X/Y/Z rotations
- Transformation composition and inversion
- Distance-based safety geofencing around simplified critical structures
- Defensive parameter validation
- Unit testing with GoogleTest
- CMake-based project organization
- GitHub Actions continuous integration

## Project structure

```text
SurgiTrack-CPP/
├── include/surgitrack/
│   ├── CoordinateTransform.hpp
│   └── SafetyGeofence.hpp
├── src/
│   ├── CoordinateTransform.cpp
│   ├── SafetyGeofence.cpp
│   └── main.cpp
├── tests/
│   └── test_navigation.cpp
├── .github/workflows/
│   └── cpp-ci.yml
├── CMakeLists.txt
└── README.md
```

## How it works

The simulator generates a small trajectory in **camera space**. Each point is transformed into **patient space** using a 4×4 homogeneous transformation matrix. The transformed position is then evaluated against simplified spherical protected zones.

For transform composition, `A.combine(B)` computes `A × B`, so **B is applied first and A second** when transforming a point.

The safety logic uses three states:

| Status | Meaning |
|---|---|
| `SAFE` | Outside all protected zones |
| `WARNING_APPROACHING` | Inside the warning buffer around a protected zone |
| `CRITICAL_VIOLATION` | Inside a protected zone |

## Build and run

### Prerequisites

- CMake 3.16+
- A C++17-compatible compiler
- Internet access for the first CMake configuration, because Eigen and GoogleTest are fetched automatically

### Configure and build

```bash
cmake -S . -B build
cmake --build build
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

### Run the simulator

On Linux/macOS:

```bash
./build/SurgiTrackApp
```

On Windows with a Visual Studio generator:

```text
build\\Debug\\SurgiTrackApp.exe
```

## Testing

The test suite covers:

- identity transformation
- translation
- rotations around X, Y and Z
- transform composition order
- inverse round-trip
- safe, warning and critical geofence states
- critical-state precedence
- invalid safety parameters

## Engineering notes and limitations

This project intentionally keeps the model small and understandable. It does **not** implement:

- optical tracking hardware
- DICOM/image processing
- registration against medical images
- robot control
- real-time guarantees
- patient-specific anatomical models
- clinical validation

The protected structures are represented as simple spheres, and the trajectory is simulated with fixed coordinates. The project should therefore be viewed as a software-engineering and mathematical-geometry exercise inspired by medical-navigation use cases.

## Why it belongs in a biomedical software portfolio

The project connects biomedical engineering concepts with software engineering fundamentals: coordinate frames, 3D geometry, safety-oriented logic, modular C++ design, automated testing and reproducible builds.

> A small, testable C++ simulation for exploring geometry and safety logic in a medical-navigation context.

## Project status

Portfolio project — educational simulation.
