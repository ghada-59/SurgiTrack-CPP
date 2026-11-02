#include "surgitrack/MonteCarloStudy.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void printUsage(std::ostream& output) {
    output << "Usage: SurgiTrackMonteCarlo [--trials N] [--seed S]\n"
              "Academic rigid-registration simulation. Units are millimetres.\n";
}

} // namespace

int main(int argc, char** argv) {
    surgitrack::MonteCarloOptions options;
    try {
        for (int index = 1; index < argc; ++index) {
            const std::string argument = argv[index];
            if (argument == "--help") {
                printUsage(std::cout);
                return 0;
            }
            if (index + 1 >= argc) {
                std::cerr << "Missing value for " << argument << "\n";
                printUsage(std::cerr);
                return 2;
            }
            if (argument == "--trials") {
                options.trials = std::stoi(argv[++index]);
            } else if (argument == "--seed") {
                options.seed = std::stoull(argv[++index]);
            } else {
                std::cerr << "Unknown argument: " << argument << "\n";
                printUsage(std::cerr);
                return 2;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        printUsage(std::cerr);
        return 2;
    }

    try {
        const surgitrack::MonteCarloStudy study = surgitrack::runMonteCarloStudy(options);
        surgitrack::printMonteCarloStudy(std::cout, study);
    } catch (const std::exception& error) {
        std::cerr << "Monte Carlo failed: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
