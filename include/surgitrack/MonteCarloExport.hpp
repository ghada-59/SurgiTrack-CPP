#ifndef SURGITRACK_MONTE_CARLO_EXPORT_HPP
#define SURGITRACK_MONTE_CARLO_EXPORT_HPP

#include "surgitrack/MonteCarloStudy.hpp"

#include <filesystem>

namespace surgitrack {

/**
 * @brief Write results.json, mc.csv, points.json and SVG figures.
 *
 * The CSV contains one row per trial. The JSON summary uses the same
 * samples as the tables printed by printMonteCarloStudy. Figures are
 * drawn from those summaries. Units are millimetres.
 */
void exportMonteCarloArtifacts(
    const MonteCarloStudy& study,
    const std::filesystem::path& directory);

} // namespace surgitrack

#endif
