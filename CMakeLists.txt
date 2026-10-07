cmake_minimum_required(VERSION 3.14)
project(SurgiTrack_CPP VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Téléchargement automatique d'Eigen3 (Header-only)
FetchContent_Declare(
  eigen
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG        3.4.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(eigen)

# Inclusion des fichiers header d'Eigen
include_directories(${eigen_SOURCE_DIR})
include_directories(include)

# 2. Fichiers sources du projet
set(SOURCES
    src/CoordinateTransform.cpp
    src/SafetyGeofence.cpp
)

# 3. Exécutable principal (Simulateur)
add_executable(SurgiTrackApp src/main.cpp ${SOURCES})

# 4. Configuration de GoogleTest
enable_testing()
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(SurgiTrackTests tests/test_navigation.cpp ${SOURCES})
target_link_libraries(SurgiTrackTests PRIVATE gtest_main)

include(GoogleTest)
gtest_discover_tests(SurgiTrackTests)