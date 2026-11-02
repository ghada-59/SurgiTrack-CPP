cmake_minimum_required(VERSION 3.16)
project(SurgiTrack_CPP VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

# Preserve this project's test setting, but disable dependency tests while
# configuring Eigen so CTest only discovers SurgiTrack's own tests.
set(SURGITRACK_BUILD_TESTING ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(eigen)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CLI11)

# Restore the test setting for SurgiTrack's own test suite.
set(BUILD_TESTING ${SURGITRACK_BUILD_TESTING} CACHE BOOL "Build tests" FORCE)

add_library(surgitrack
    src/CoordinateTransform.cpp
    src/SafetyGeofence.cpp
    src/RigidRegistration.cpp
    src/RegistrationMetrics.cpp
    src/MonteCarloStudy.cpp
    src/MonteCarloExport.cpp
)

target_include_directories(surgitrack PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(surgitrack PUBLIC Eigen3::Eigen nlohmann_json::nlohmann_json)
target_compile_features(surgitrack PUBLIC cxx_std_17)

# Enable useful compiler diagnostics on project code without forcing warnings
# on the third-party Eigen and GoogleTest targets.
function(surgitrack_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

surgitrack_enable_warnings(surgitrack)

find_package(OpenMP COMPONENTS CXX)
if(OpenMP_CXX_FOUND)
    target_link_libraries(surgitrack PUBLIC OpenMP::OpenMP_CXX)
endif()

add_executable(SurgiTrackApp src/main.cpp)
target_link_libraries(SurgiTrackApp PRIVATE surgitrack)
surgitrack_enable_warnings(SurgiTrackApp)

add_executable(SurgiTrackMonteCarlo src/MonteCarloMain.cpp)
target_link_libraries(SurgiTrackMonteCarlo PRIVATE surgitrack CLI11::CLI11)
surgitrack_enable_warnings(SurgiTrackMonteCarlo)

if(BUILD_TESTING)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(SurgiTrackTests
        tests/test_navigation.cpp
        tests/test_rigid_registration.cpp
        tests/test_registration_metrics.cpp
        tests/test_monte_carlo_export.cpp
    )
    target_link_libraries(SurgiTrackTests PRIVATE surgitrack GTest::gtest_main)
    surgitrack_enable_warnings(SurgiTrackTests)

    include(GoogleTest)
    gtest_discover_tests(SurgiTrackTests)
endif()
