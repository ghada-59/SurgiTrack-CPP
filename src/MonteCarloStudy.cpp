#include "surgitrack/MonteCarloStudy.hpp"

#include "surgitrack/RegistrationMetrics.hpp"
#include "surgitrack/RigidRegistration.hpp"

#include <Eigen/Geometry>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <limits>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace surgitrack {
namespace {

constexpr double kPi = 3.14159265358979323846;

std::uint64_t mixSeed(
    std::uint64_t seed,
    int fiducial_count,
    int fle_index,
    int trial) {
    auto mix = [](std::uint64_t value) {
        value += 0x9E3779B97F4A7C15ULL;
        value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31);
    };
    std::uint64_t value = seed;
    value ^= mix(static_cast<std::uint64_t>(fiducial_count) + 1ULL);
    value ^= mix(static_cast<std::uint64_t>(fle_index) + 0x100ULL);
    value ^= mix(static_cast<std::uint64_t>(trial) + 0x10000ULL);
    return mix(value);
}

double uniform01(std::mt19937_64& generator) {
    // 53 mantissa bits. Avoids std::uniform_real_distribution, whose
    // algorithm is not fixed by the standard.
    constexpr double kScale = 1.0 / 9007199254740992.0;
    return static_cast<double>(generator() >> 11) * kScale;
}

double standardNormal(std::mt19937_64& generator) {
    double u1 = uniform01(generator);
    while (!(u1 > 0.0)) {
        u1 = uniform01(generator);
    }
    const double u2 = uniform01(generator);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * kPi * u2);
}

std::string utcTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm calendar{};
#if defined(_WIN32)
    if (gmtime_s(&calendar, &now) != 0) {
        throw std::runtime_error("Could not format the UTC timestamp.");
    }
#else
    if (gmtime_r(&now, &calendar) == nullptr) {
        throw std::runtime_error("Could not format the UTC timestamp.");
    }
#endif
    char buffer[32];
    if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &calendar) == 0) {
        throw std::runtime_error("Could not format the UTC timestamp.");
    }
    return buffer;
}

Eigen::Matrix3Xd referenceFiducialsMm() {
    // Ten labelled points in a 100 mm box (coordinates in [-50, 50]).
    // The first three form a non-degenerate triangle; later prefixes stay
    // non-collinear. Comma initialisation fills left to right, top to bottom.
    Eigen::Matrix3Xd fiducials(3, 10);
    fiducials <<
        50.0, -50.0, -50.0, 50.0, 50.0, -50.0, 50.0, -50.0, 0.0, 0.0,
        50.0, -50.0, 50.0, -50.0, -50.0, 50.0, 50.0, -50.0, 0.0, 50.0,
        50.0, 50.0, -50.0, -50.0, 50.0, 50.0, -50.0, -50.0, 50.0, 0.0;
    return fiducials;
}

Eigen::Matrix3d referenceRotation() {
    return (Eigen::AngleAxisd(0.40, Eigen::Vector3d::UnitZ())
            * Eigen::AngleAxisd(-0.25, Eigen::Vector3d::UnitY())
            * Eigen::AngleAxisd(0.10, Eigen::Vector3d::UnitX()))
        .toRotationMatrix();
}

std::string targetName(double distance_mm) {
    if (!(distance_mm > 0.0)) {
        return "centroid";
    }
    std::ostringstream name;
    name << "d" << std::fixed << std::setprecision(0) << distance_mm;
    return name.str();
}

double relativeError(double measured, double reference) {
    if (!(std::abs(reference) > 0.0)) {
        return std::abs(measured) > 0.0
            ? std::numeric_limits<double>::infinity()
            : 0.0;
    }
    return (measured - reference) / reference;
}

struct DistributionSummary {
    double mean = 0.0;
    double mean_of_squares = 0.0;
    double rms = 0.0;
    double p5 = 0.0;
    double p50 = 0.0;
    double p95 = 0.0;
    double p99 = 0.0;
};

DistributionSummary summarize(const std::vector<double>& samples) {
    if (samples.empty()) {
        throw std::invalid_argument("Cannot summarise an empty sample.");
    }
    DistributionSummary summary;
    const double count = static_cast<double>(samples.size());
    summary.mean = std::accumulate(samples.begin(), samples.end(), 0.0) / count;
    const double sum_of_squares = std::accumulate(
        samples.begin(),
        samples.end(),
        0.0,
        [](double total, double value) { return total + value * value; });
    summary.mean_of_squares = sum_of_squares / count;
    summary.rms = std::sqrt(summary.mean_of_squares);
    summary.p5 = linearPercentile(samples, 0.05);
    summary.p50 = linearPercentile(samples, 0.50);
    summary.p95 = linearPercentile(samples, 0.95);
    summary.p99 = linearPercentile(samples, 0.99);
    return summary;
}

void validateOptions(const MonteCarloOptions& options, int available_fiducials) {
    if (options.trials < 1) {
        throw std::invalid_argument("Monte Carlo trials must be positive.");
    }
    if (options.fle_rms_mm.empty() || options.fiducial_counts.empty()
        || options.target_distances_mm.empty()) {
        throw std::invalid_argument("Monte Carlo options are empty.");
    }
    for (const double fle_mm : options.fle_rms_mm) {
        if (!std::isfinite(fle_mm) || fle_mm < 0.0) {
            throw std::invalid_argument("FLE must be finite and non-negative.");
        }
    }
    for (const int count : options.fiducial_counts) {
        if (count < 3 || count > available_fiducials) {
            throw std::invalid_argument(
                "Fiducial count must be from 3 to the size of the reference cloud.");
        }
    }
    for (const double distance_mm : options.target_distances_mm) {
        if (!std::isfinite(distance_mm) || distance_mm < 0.0) {
            throw std::invalid_argument(
                "Target distance must be finite and non-negative.");
        }
    }
}

} // namespace

double linearPercentile(std::vector<double> samples, double probability) {
    if (samples.empty()) {
        throw std::invalid_argument("Percentile requires at least one sample.");
    }
    if (!(probability >= 0.0 && probability <= 1.0) || !std::isfinite(probability)) {
        throw std::invalid_argument("Percentile probability must be in [0, 1].");
    }
    for (const double sample : samples) {
        if (!std::isfinite(sample)) {
            throw std::invalid_argument("Percentile samples must be finite.");
        }
    }
    std::sort(samples.begin(), samples.end());
    if (samples.size() == 1) {
        return samples.front();
    }
    const double position = probability * static_cast<double>(samples.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(position));
    const auto upper = static_cast<std::size_t>(std::ceil(position));
    const double weight = position - static_cast<double>(lower);
    return samples[lower] * (1.0 - weight) + samples[upper] * weight;
}

MonteCarloStudy runMonteCarloStudy(const MonteCarloOptions& options) {
    const Eigen::Matrix3Xd fiducials_mm = referenceFiducialsMm();
    validateOptions(options, static_cast<int>(fiducials_mm.cols()));

    const Eigen::Matrix3d true_rotation = referenceRotation();
    const Eigen::Vector3d true_translation_mm(15.0, -8.0, 6.0);
    const Eigen::Vector3d target_direction =
        Eigen::Vector3d(1.0, 1.0, 1.0).normalized();

    MonteCarloStudy study;
    study.seed = options.seed;
    study.trials_per_configuration = options.trials;
    study.generated_at_utc = utcTimestamp();
    study.fle_rms_mm = options.fle_rms_mm;
    study.fiducial_counts = options.fiducial_counts;
    study.target_distances_mm = options.target_distances_mm;
    study.true_rotation = true_rotation;
    study.true_translation_mm = true_translation_mm;
    study.fiducials_mm = fiducials_mm;
#if defined(_OPENMP)
    study.thread_count = omp_get_max_threads();
#else
    study.thread_count = 1;
#endif

    study.configurations.reserve(
        options.fiducial_counts.size() * options.fle_rms_mm.size());

    for (const int fiducial_count : options.fiducial_counts) {
        const Eigen::Matrix3Xd subset_mm = fiducials_mm.leftCols(fiducial_count);
        const FiducialMoments moments = fiducialMomentsMm(subset_mm);
        const Eigen::Matrix3Xd fixed_mm =
            (true_rotation * subset_mm).colwise() + true_translation_mm;

        // Reject a layout that the registrar itself will not accept.
        RigidRegistrationOptions noiseless_options;
        noiseless_options.emit_warnings = false;
        static_cast<void>(registerRigid(subset_mm, fixed_mm, noiseless_options));

        std::vector<Eigen::Vector3d> targets_mm;
        targets_mm.reserve(options.target_distances_mm.size());
        for (const double distance_mm : options.target_distances_mm) {
            targets_mm.push_back(moments.centroid_mm + distance_mm * target_direction);
        }

        for (std::size_t fle_index = 0; fle_index < options.fle_rms_mm.size(); ++fle_index) {
            const double fle_mm = options.fle_rms_mm[fle_index];
            MonteCarloConfiguration configuration;
            configuration.fiducial_count = fiducial_count;
            configuration.fle_rms_mm = fle_mm;
            configuration.fre_samples_mm.assign(static_cast<std::size_t>(options.trials), 0.0);
            configuration.tre_samples_mm.assign(
                targets_mm.size(),
                std::vector<double>(static_cast<std::size_t>(options.trials), 0.0));

            const double per_axis_sigma_mm = fle_mm / std::sqrt(3.0);
            const int trials = options.trials;
            const int fle_index_int = static_cast<int>(fle_index);

#if defined(_OPENMP)
#pragma omp parallel for schedule(static)
#endif
            for (int trial = 0; trial < trials; ++trial) {
                std::mt19937_64 generator(mixSeed(
                    options.seed, fiducial_count, fle_index_int, trial));
                Eigen::Matrix3Xd moving_mm = subset_mm;
                for (Eigen::Index col = 0; col < moving_mm.cols(); ++col) {
                    for (Eigen::Index row = 0; row < moving_mm.rows(); ++row) {
                        moving_mm(row, col) +=
                            per_axis_sigma_mm * standardNormal(generator);
                    }
                }

                RigidRegistrationOptions registration_options;
                registration_options.emit_warnings = false;
                // Pair distances move by about FLE. The tolerance stays far
                // above that jitter and still rejects a swapped label.
                registration_options.pairing_tolerance_mm = std::max(1e-3, 15.0 * fle_mm);
                const RigidRegistrationResult registration =
                    registerRigid(moving_mm, fixed_mm, registration_options);

                configuration.fre_samples_mm[static_cast<std::size_t>(trial)] =
                    registration.fre_rms_mm;
                for (std::size_t target_index = 0; target_index < targets_mm.size();
                     ++target_index) {
                    const Eigen::Vector3d truth_mm =
                        true_rotation * targets_mm[target_index] + true_translation_mm;
                    const Eigen::Vector3d estimated_mm =
                        registration.rotation * targets_mm[target_index]
                        + registration.translation_mm;
                    configuration.tre_samples_mm[target_index][static_cast<std::size_t>(trial)] =
                        treMm(estimated_mm, truth_mm);
                }
            }

            const DistributionSummary fre = summarize(configuration.fre_samples_mm);
            configuration.fre.mean_mm = fre.mean;
            configuration.fre.median_mm = fre.p50;
            configuration.fre.rms_mm = fre.rms;
            configuration.fre.mean_squared_mm2 = fre.mean_of_squares;
            configuration.fre.p5_mm = fre.p5;
            configuration.fre.p50_mm = fre.p50;
            configuration.fre.p95_mm = fre.p95;
            configuration.fre.p99_mm = fre.p99;
            configuration.fre.sibson_fre2_mm2 =
                (1.0 - 2.0 / static_cast<double>(fiducial_count)) * fle_mm * fle_mm;
            configuration.fre.relative_error_fre2 = relativeError(
                configuration.fre.mean_squared_mm2,
                configuration.fre.sibson_fre2_mm2);

            configuration.targets.reserve(targets_mm.size());
            for (std::size_t target_index = 0; target_index < targets_mm.size();
                 ++target_index) {
                const DistributionSummary tre =
                    summarize(configuration.tre_samples_mm[target_index]);
                MonteCarloTargetSummary target;
                target.name = targetName(options.target_distances_mm[target_index]);
                target.distance_from_centroid_mm = options.target_distances_mm[target_index];
                target.position_mm = targets_mm[target_index];
                target.fitzpatrick_tre2_mm2 = fitzpatrickTreSquaredMm2(
                    fle_mm, fiducial_count, moments, targets_mm[target_index]);
                target.mean_tre_mm = tre.mean;
                target.median_tre_mm = tre.p50;
                target.rms_tre_mm = tre.rms;
                target.mean_tre2_mm2 = tre.mean_of_squares;
                target.p5_tre_mm = tre.p5;
                target.p50_tre_mm = tre.p50;
                target.p95_tre_mm = tre.p95;
                target.p99_tre_mm = tre.p99;
                target.relative_error_tre2 = relativeError(
                    target.mean_tre2_mm2, target.fitzpatrick_tre2_mm2);
                configuration.targets.push_back(target);
            }

            study.configurations.push_back(std::move(configuration));
        }
    }

    return study;
}

void printMonteCarloStudy(std::ostream& output, const MonteCarloStudy& study) {
    output << std::fixed;
    output << "SurgiTrack-CPP Monte Carlo (academic simulation, not clinical)\n";
    output << "units: mm\n";
    output << "seed: " << study.seed << "\n";
    output << "trials_per_configuration: " << study.trials_per_configuration << "\n";
    output << "openmp_threads: " << study.thread_count << "\n";
    output << "generated_at_utc: " << study.generated_at_utc << "\n";
    output << "fle_definition: 3D RMS; per-axis sigma = FLE/sqrt(3); "
              "noise on moving fiducials only\n";
    output << "percentile: Hyndman-Fan type 7; median is p50\n";
    output << "formula: TRE^2 = (FLE^2 / N) * (1 + (1/3) * sum d_k^2 / f_k^2)\n";

    output << "\n# TRE\n";
    output << "n_fiducials fle_mm target distance_mm mean_mm median_mm "
              "p5_mm p50_mm p95_mm p99_mm rms_mm mean_tre2_mm2 "
              "fitzpatrick_tre2_mm2 relative_error_tre2\n";

    double max_abs_relative_tre2 = 0.0;
    for (const auto& configuration : study.configurations) {
        for (const auto& target : configuration.targets) {
            output << configuration.fiducial_count << " "
                   << std::setprecision(6) << configuration.fle_rms_mm << " "
                   << target.name << " "
                   << target.distance_from_centroid_mm << " "
                   << target.mean_tre_mm << " "
                   << target.median_tre_mm << " "
                   << target.p5_tre_mm << " "
                   << target.p50_tre_mm << " "
                   << target.p95_tre_mm << " "
                   << target.p99_tre_mm << " "
                   << target.rms_tre_mm << " "
                   << std::setprecision(8) << target.mean_tre2_mm2 << " "
                   << target.fitzpatrick_tre2_mm2 << " "
                   << target.relative_error_tre2 << "\n";
            max_abs_relative_tre2 = std::max(
                max_abs_relative_tre2, std::abs(target.relative_error_tre2));
        }
    }

    output << "\n# FRE\n";
    output << "n_fiducials fle_mm mean_mm median_mm p5_mm p50_mm p95_mm p99_mm "
              "rms_mm mean_fre2_mm2 sibson_fre2_mm2 relative_error_fre2\n";
    for (const auto& configuration : study.configurations) {
        output << configuration.fiducial_count << " "
               << std::setprecision(6) << configuration.fle_rms_mm << " "
               << configuration.fre.mean_mm << " "
               << configuration.fre.median_mm << " "
               << configuration.fre.p5_mm << " "
               << configuration.fre.p50_mm << " "
               << configuration.fre.p95_mm << " "
               << configuration.fre.p99_mm << " "
               << configuration.fre.rms_mm << " "
               << std::setprecision(8) << configuration.fre.mean_squared_mm2 << " "
               << configuration.fre.sibson_fre2_mm2 << " "
               << configuration.fre.relative_error_fre2 << "\n";
    }

    output << std::setprecision(8);
    output << "\nconfigurations: " << study.configurations.size() << "\n";
    output << "max_abs_relative_error_tre2: " << max_abs_relative_tre2 << "\n";
}

} // namespace surgitrack
