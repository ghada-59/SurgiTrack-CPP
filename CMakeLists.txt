cmake_minimum_required(VERSION 3.16)
project(SurgiTrack_CPP VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(eigen)

add_library(surgitrack
    src/CoordinateTransform.cpp
    src/SafetyGeofence.cpp
)

target_include_directories(surgitrack PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(surgitrack PUBLIC Eigen3::Eigen)
target_compile_features(surgitrack PUBLIC cxx_std_17)

add_executable(SurgiTrackApp src/main.cpp)
target_link_libraries(SurgiTrackApp PRIVATE surgitrack)

if(BUILD_TESTING)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(SurgiTrackTests tests/test_navigation.cpp)
    target_link_libraries(SurgiTrackTests PRIVATE surgitrack GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(SurgiTrackTests)
endif()
