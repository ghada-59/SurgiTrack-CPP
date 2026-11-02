#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "surgitrack/MonteCarloStudy.hpp"
#include "surgitrack/RegistrationMetrics.hpp"

using namespace surgitrack;

namespace {

Eigen::Matrix3Xd squareFiducialsMm() {
    Eigen::Matrix3Xd points(3, 4);
    points.col(0) << 1.0, 0.0, 0.0;
    points.col(1) << -1.0, 0.0, 0.0;
    points.col(2) << 0.0, 1.0, 0.0;
    points.col(3) << 0.0, -1.0, 0.0;
    return points;
}

} // namespace

TEST(RegistrationMetricsTest, FreIsRootMeanSquareResidual) {
    Eigen::Matrix3Xd estimated(3, 2);
    estimated.col(0) << 0.0, 0.0, 0.0;
    estimated.col(1) << 0.0, 0.0, 0.0;
    Eigen::Matrix3Xd reference(3, 2);
    reference.col(0) << 3.0, 0.0, 0.0;
    reference.col(1) << 0.0, 4.0, 0.0;

    EXPECT_NEAR(freRmsMm(estimated, reference), std::sqrt(12.5), 1e-12);
}

TEST(RegistrationMetricsTest, TreIsEuclideanDistance) {
    EXPECT_NEAR(
        treMm(Eigen::Vector3d(1.0, 2.0, 2.0), Eigen::Vector3d(0.0, 0.0, 0.0)),
        3.0,
        1e-12);
}

TEST(RegistrationMetricsTest, FitzpatrickAtCentroidIsFleSquaredOverN) {
    const Eigen::Matrix3Xd fiducials = squareFiducialsMm();
    const FiducialMoments moments = fiducialMomentsMm(fiducials);
    const double tre2 = fitzpatrickTreSquaredMm2(1.0, 4, moments, moments.centroid_mm);
    EXPECT_NEAR(tre2, 0.25, 1e-12);
}

TEST(RegistrationMetricsTest, FitzpatrickMatchesHandCalculationOffAxis) {
    const Eigen::Matrix3Xd fiducials = squareFiducialsMm();
    const FiducialMoments moments = fiducialMomentsMm(fiducials);
    const Eigen::Vector3d target(0.0, 0.0, 10.0);
    // f² = (0.5, 0.5, 1), d² = (100, 100, 0), sum = 400.
    // TRE² = (1² / 4) * (1 + 400/3) = 403/12.
    const double tre2 = fitzpatrickTreSquaredMm2(1.0, 4, moments, target);
    EXPECT_NEAR(tre2, 403.0 / 12.0, 1e-9);
}

TEST(RegistrationMetricsTest, LinearPercentileInterpolates) {
    const std::vector<double> samples{1.0, 2.0, 3.0, 4.0};
    EXPECT_NEAR(linearPercentile(samples, 0.0), 1.0, 1e-12);
    EXPECT_NEAR(linearPercentile(samples, 0.5), 2.5, 1e-12);
    EXPECT_NEAR(linearPercentile(samples, 1.0), 4.0, 1e-12);
}

TEST(MonteCarloStudyTest, ZeroNoiseRecoversTargets) {
    MonteCarloOptions options;
    options.trials = 1;
    options.seed = 7;
    options.fle_rms_mm = {0.0};
    options.fiducial_counts = {6};
    options.target_distances_mm = {0.0, 25.0};

    const MonteCarloStudy study = runMonteCarloStudy(options);
    ASSERT_EQ(study.configurations.size(), 1U);
    EXPECT_NEAR(study.configurations[0].fre.mean_mm, 0.0, 1e-9);
    ASSERT_EQ(study.configurations[0].targets.size(), 2U);
    EXPECT_NEAR(study.configurations[0].targets[0].mean_tre_mm, 0.0, 1e-9);
    EXPECT_NEAR(study.configurations[0].targets[1].mean_tre_mm, 0.0, 1e-9);
    EXPECT_NEAR(
        study.configurations[0].targets[0].fitzpatrick_tre2_mm2,
        0.0,
        1e-12);
}

TEST(MonteCarloStudyTest, RepeatedSeedMatches) {
    MonteCarloOptions options;
    options.trials = 40;
    options.seed = 99;
    options.fle_rms_mm = {0.25};
    options.fiducial_counts = {4, 8};
    options.target_distances_mm = {0.0, 50.0};

    const MonteCarloStudy first = runMonteCarloStudy(options);
    const MonteCarloStudy second = runMonteCarloStudy(options);
    ASSERT_EQ(first.configurations.size(), second.configurations.size());
    for (std::size_t index = 0; index < first.configurations.size(); ++index) {
        EXPECT_NEAR(
            first.configurations[index].fre.mean_mm,
            second.configurations[index].fre.mean_mm,
            1e-12);
        ASSERT_EQ(
            first.configurations[index].targets.size(),
            second.configurations[index].targets.size());
        for (std::size_t target = 0; target < first.configurations[index].targets.size();
             ++target) {
            EXPECT_NEAR(
                first.configurations[index].targets[target].mean_tre_mm,
                second.configurations[index].targets[target].mean_tre_mm,
                1e-12);
            EXPECT_NEAR(
                first.configurations[index].fre_samples_mm[target],
                second.configurations[index].fre_samples_mm[target],
                1e-12);
        }
    }
}
