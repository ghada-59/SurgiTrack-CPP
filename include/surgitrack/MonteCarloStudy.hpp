#ifndef SURGITRACK_MONTE_CARLO_STUDY_HPP
#define SURGITRACK_MONTE_CARLO_STUDY_HPP

#include <Eigen/Dense>

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace surgitrack {

/// Linear interpolation percentile (Hyndman-Fan type 7). probability is in [0, 1].
double linearPercentile(std::vector<double> samples, double probability);

struct MonteCarloTargetSummary {
    std::string name;
    double distance_from_centroid_mm = 0.0;
    Eigen::Vector3d position_mm = Eigen::Vector3d::Zero();
    double fitzpatrick_tre2_mm2 = 0.0;
    double mean_tre_mm = 0.0;
    double median_tre_mm = 0.0;
    double rms_tre_mm = 0.0;
    double mean_tre2_mm2 = 0.0;
    double p50_tre_mm = 0.0;
    double p95_tre_mm = 0.0;
    double p99_tre_mm = 0.0;
    /// (mean TRE² − formula) / formula. Signed.
    double relative_error_tre2 = 0.0;
};

struct MonteCarloFreSummary {
    double mean_mm = 0.0;
    double median_mm = 0.0;
    double rms_mm = 0.0;
    double mean_squared_mm2 = 0.0;
    double p50_mm = 0.0;
    double p95_mm = 0.0;
    double p99_mm = 0.0;
    /// Sibson: ⟨FRE²⟩ ≈ (1 − 2/N) ⟨FLE²⟩.
    double sibson_fre2_mm2 = 0.0;
    double relative_error_fre2 = 0.0;
};

struct MonteCarloConfiguration {
    int fiducial_count = 0;
    double fle_rms_mm = 0.0;
    MonteCarloFreSummary fre;
    std::vector<MonteCarloTargetSummary> targets;
    /// Trial-major samples, kept so a later export matches this run.
    std::vector<double> fre_samples_mm;
    /// tre_samples_mm[target][trial], same trial order as fre_samples_mm.
    std::vector<std::vector<double>> tre_samples_mm;
};

struct MonteCarloOptions {
    std::uint64_t seed = 20261102ULL;
    int trials = 10000;
    std::vector<double> fle_rms_mm{0.1, 0.25, 0.5, 1.0};
    std::vector<int> fiducial_counts{3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<double> target_distances_mm{0.0, 25.0, 50.0, 75.0};
};

struct MonteCarloStudy {
    std::uint64_t seed = 0;
    int trials_per_configuration = 0;
    int thread_count = 1;
    std::string generated_at_utc;
    std::vector<double> fle_rms_mm;
    std::vector<int> fiducial_counts;
    std::vector<double> target_distances_mm;
    Eigen::Matrix3d true_rotation = Eigen::Matrix3d::Identity();
    Eigen::Vector3d true_translation_mm = Eigen::Vector3d::Zero();
    /// Up to 10 labelled fiducials inside a 100 mm box. Columns are points, mm.
    Eigen::Matrix3Xd fiducials_mm;
    std::vector<MonteCarloConfiguration> configurations;
};

/**
 * @brief Repeated rigid registrations with isotropic Gaussian FLE.
 *
 * Fiducials are a fixed, non-collinear subset of a 100 mm layout.
 * A known rigid pose maps them into the fixed frame. Noise with the
 * requested 3D RMS is added only to the moving cloud. Each trial uses
 * its own std::mt19937_64 seed derived from the master seed, so threads
 * never share a generator and the sample does not depend on the thread count.
 */
MonteCarloStudy runMonteCarloStudy(const MonteCarloOptions& options);

void printMonteCarloStudy(std::ostream& output, const MonteCarloStudy& study);

} // namespace surgitrack

#endif
