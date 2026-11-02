#include "surgitrack/MonteCarloExport.hpp"
#include "surgitrack/MonteCarloStudy.hpp"

#include <CLI/CLI.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    surgitrack::MonteCarloOptions options;
    std::string output_directory;

    CLI::App app{"Academic rigid-registration simulation. Units are millimetres."};
    app.add_option("--trials", options.trials, "Trials per configuration")
        ->check(CLI::PositiveNumber);
    app.add_option("--seed", options.seed, "Master seed for the per-trial generators");
    app.add_option(
        "--output",
        output_directory,
        "Directory for results.json, mc.csv, points.json and the SVG figures");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& error) {
        return app.exit(error);
    }

    try {
        const surgitrack::MonteCarloStudy study = surgitrack::runMonteCarloStudy(options);
        surgitrack::printMonteCarloStudy(std::cout, study);
        if (!output_directory.empty()) {
            surgitrack::exportMonteCarloArtifacts(study, output_directory);
            std::cout << "wrote artifacts to " << output_directory << "\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "Monte Carlo failed: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
