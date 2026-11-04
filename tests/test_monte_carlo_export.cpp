#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include "surgitrack/MonteCarloExport.hpp"
#include "surgitrack/MonteCarloStudy.hpp"

using namespace surgitrack;

namespace {

struct TemporaryDirectoryCleanup {
    std::filesystem::path path;

    ~TemporaryDirectoryCleanup() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

}  // namespace

TEST(MonteCarloExportTest, WritesJsonCsvAndPoints) {
    MonteCarloOptions options;
    options.trials = 3;
    options.seed = 11;
    options.fle_rms_mm = {0.1};
    options.fiducial_counts = {4};
    options.target_distances_mm = {0.0, 25.0};

    const MonteCarloStudy study = runMonteCarloStudy(options);

    // Reserve a unique directory atomically so parallel test processes cannot
    // remove or overwrite one another's exported files.
    const auto unique_id =
        std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path directory;
    for (unsigned int attempt = 0; attempt < 1000; ++attempt) {
        const std::filesystem::path candidate =
            std::filesystem::temp_directory_path() /
            ("surgitrack-export-test-" + std::to_string(unique_id) + "-"
             + std::to_string(attempt));
        std::error_code error;
        if (std::filesystem::create_directory(candidate, error)) {
            directory = candidate;
            break;
        }
        ASSERT_FALSE(static_cast<bool>(error))
            << "Could not create temporary test directory: " << error.message();
    }
    ASSERT_FALSE(directory.empty())
        << "Could not reserve a unique temporary test directory.";
    const TemporaryDirectoryCleanup cleanup{directory};

    exportMonteCarloArtifacts(study, directory);

    const auto read_all = [](const std::filesystem::path& path) {
        std::ifstream input(path);
        std::ostringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    };

    const std::string results = read_all(directory / "results.json");
    EXPECT_NE(results.find("\\"version\\": \\"1.0\\""), std::string::npos);
    EXPECT_NE(results.find("\\"units\\": \\"mm\\""), std::string::npos);
    EXPECT_NE(results.find("\\"clinical\\": false"), std::string::npos);
    EXPECT_NE(results.find("\\"trials_per_configuration\\": 3"), std::string::npos);
    EXPECT_NE(results.find("\\"p5_tre_mm\\""), std::string::npos);
    EXPECT_NE(results.find("\\"p95_tre_mm\\""), std::string::npos);

    {
        std::ifstream csv(directory / "mc.csv");
        std::string header;
        ASSERT_TRUE(std::getline(csv, header));
        EXPECT_NE(header.find("fre_rms_mm"), std::string::npos);
        EXPECT_NE(header.find("tre_centroid_mm"), std::string::npos);
        EXPECT_NE(header.find("tre_d25_mm"), std::string::npos);
        int rows = 0;
        std::string line;
        while (std::getline(csv, line)) {
            if (!line.empty()) {
                ++rows;
            }
        }
        EXPECT_EQ(rows, 3);
    }  // The CSV stream is closed before TemporaryDirectoryCleanup runs.

    const std::string points = read_all(directory / "points.json");
    EXPECT_NE(points.find("\\"units\\": \\"mm\\""), std::string::npos);
    EXPECT_NE(points.find("\\"moving_mm\\""), std::string::npos);
    EXPECT_NE(points.find("\\"registered_mm\\""), std::string::npos);

    EXPECT_TRUE(std::filesystem::exists(directory / "tre_vs_distance.svg"));
    EXPECT_TRUE(
        std::filesystem::exists(directory / "tre_histogram_n6_fle0p25.svg"));
}
