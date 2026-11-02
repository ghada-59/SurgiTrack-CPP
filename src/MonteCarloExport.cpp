#include "surgitrack/MonteCarloExport.hpp"

#include "surgitrack/RegistrationMetrics.hpp"
#include "surgitrack/RigidRegistration.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace surgitrack {
namespace {

class JsonScope {
public:
    JsonScope(std::ostream& output, char open, char close)
        : output_(output), close_(close) {
        output_ << open << '\n';
    }

    ~JsonScope() {
        output_ << '\n' << indent_ << close_;
    }

    JsonScope(const JsonScope&) = delete;
    JsonScope& operator=(const JsonScope&) = delete;

    std::ostream& field(const char* key) {
        if (!first_) {
            output_ << ",\n";
        }
        first_ = false;
        output_ << indent_ << '"' << key << "\": ";
        return output_;
    }

    std::ostream& element() {
        if (!first_) {
            output_ << ",\n";
        }
        first_ = false;
        output_ << indent_;
        return output_;
    }

private:
    std::ostream& output_;
    char close_;
    bool first_ = true;
    const char* indent_ = "  ";
};

void writeNumber(std::ostream& output, double value) {
    if (!std::isfinite(value)) {
        output << "null";
        return;
    }
    std::ostringstream formatted;
    formatted << std::setprecision(17) << value;
    output << formatted.str();
}

void writeString(std::ostream& output, const std::string& value) {
    output << '"';
    for (const char character : value) {
        switch (character) {
            case '"':
                output << "\\\"";
                break;
            case '\\':
                output << "\\\\";
                break;
            case '\n':
                output << "\\n";
                break;
            default:
                output << character;
                break;
        }
    }
    output << '"';
}

void writeVector3(std::ostream& output, const Eigen::Vector3d& value) {
    output << '[';
    writeNumber(output, value.x());
    output << ',';
    writeNumber(output, value.y());
    output << ',';
    writeNumber(output, value.z());
    output << ']';
}

void writeMatrix3(std::ostream& output, const Eigen::Matrix3d& value) {
    output << '[';
    for (int row = 0; row < 3; ++row) {
        if (row > 0) {
            output << ',';
        }
        output << '[';
        for (int col = 0; col < 3; ++col) {
            if (col > 0) {
                output << ',';
            }
            writeNumber(output, value(row, col));
        }
        output << ']';
    }
    output << ']';
}

void writePoints(std::ostream& output, const Eigen::Matrix3Xd& points) {
    output << '[';
    for (Eigen::Index col = 0; col < points.cols(); ++col) {
        if (col > 0) {
            output << ',';
        }
        writeVector3(output, points.col(col));
    }
    output << ']';
}

std::string xmlEscape(const std::string& value) {
    std::string escaped;
    for (const char character : value) {
        switch (character) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            default:
                escaped.push_back(character);
                break;
        }
    }
    return escaped;
}

const MonteCarloConfiguration* findConfiguration(
    const MonteCarloStudy& study,
    int fiducial_count,
    double fle_rms_mm) {
    for (const auto& configuration : study.configurations) {
        if (configuration.fiducial_count == fiducial_count
            && std::abs(configuration.fle_rms_mm - fle_rms_mm) <= 1e-12) {
            return &configuration;
        }
    }
    return nullptr;
}

void writeResultsJson(const std::filesystem::path& path, const MonteCarloStudy& study) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not write " + path.string());
    }

    double max_abs_relative_tre2 = 0.0;
    for (const auto& configuration : study.configurations) {
        for (const auto& target : configuration.targets) {
            max_abs_relative_tre2 = std::max(
                max_abs_relative_tre2, std::abs(target.relative_error_tre2));
        }
    }

    JsonScope root(output, '{', '}');
    root.field("version") << "\"1.0\"";
    root.field("units") << "\"mm\"";
    root.field("project") << "\"SurgiTrack-CPP\"";
    root.field("academic") << "true";
    root.field("clinical") << "false";
    root.field("disclaimer")
        << "\"Academic simulation. Not a clinical, diagnostic, or surgical system.\"";
    root.field("seed") << study.seed;
    root.field("trials_per_configuration") << study.trials_per_configuration;
    root.field("openmp_threads") << study.thread_count;
    root.field("generated_at_utc");
    writeString(output, study.generated_at_utc);
    root.field("fle_definition")
        << "\"3D RMS. Per-axis sigma = FLE/sqrt(3). Noise is added only to the moving fiducials.\"";
    root.field("percentile") << "\"Hyndman-Fan type 7. median is p50.\"";
    root.field("formula")
        << "\"TRE^2 = (FLE^2 / N) * (1 + (1/3) * sum d_k^2 / f_k^2)\"";
    root.field("fre_formula") << "\"Sibson: expected FRE^2 = (1 - 2/N) * FLE^2\"";
    root.field("point_convention")
        << "\"C++ clouds are 3 x N, one point per column. JSON points are [x, y, z].\"";
    root.field("true_rotation");
    writeMatrix3(output, study.true_rotation);
    root.field("true_translation_mm");
    writeVector3(output, study.true_translation_mm);
    root.field("max_abs_relative_error_tre2");
    writeNumber(output, max_abs_relative_tre2);
    root.field("csv_rows")
        << static_cast<long long>(study.configurations.size())
            * static_cast<long long>(study.trials_per_configuration);

    root.field("fle_levels_mm") << '[';
    for (std::size_t index = 0; index < study.fle_rms_mm.size(); ++index) {
        if (index > 0) {
            output << ',';
        }
        writeNumber(output, study.fle_rms_mm[index]);
    }
    output << ']';

    root.field("fiducial_counts") << '[';
    for (std::size_t index = 0; index < study.fiducial_counts.size(); ++index) {
        if (index > 0) {
            output << ',';
        }
        output << study.fiducial_counts[index];
    }
    output << ']';

    root.field("configurations") << '[';
    bool first_configuration = true;
    for (const auto& configuration : study.configurations) {
        if (!first_configuration) {
            output << ',';
        }
        first_configuration = false;
        output << "\n  {\n    \"n_fiducials\": " << configuration.fiducial_count;
        output << ",\n    \"fle_rms_mm\": ";
        writeNumber(output, configuration.fle_rms_mm);
        output << ",\n    \"fre\": {";
        output << "\"mean_mm\": ";
        writeNumber(output, configuration.fre.mean_mm);
        output << ",\"median_mm\": ";
        writeNumber(output, configuration.fre.median_mm);
        output << ",\"p50_mm\": ";
        writeNumber(output, configuration.fre.p50_mm);
        output << ",\"p95_mm\": ";
        writeNumber(output, configuration.fre.p95_mm);
        output << ",\"p99_mm\": ";
        writeNumber(output, configuration.fre.p99_mm);
        output << ",\"rms_mm\": ";
        writeNumber(output, configuration.fre.rms_mm);
        output << ",\"mean_squared_mm2\": ";
        writeNumber(output, configuration.fre.mean_squared_mm2);
        output << ",\"sibson_fre2_mm2\": ";
        writeNumber(output, configuration.fre.sibson_fre2_mm2);
        output << ",\"relative_error_fre2\": ";
        writeNumber(output, configuration.fre.relative_error_fre2);
        output << "}";
        output << ",\n    \"targets\": [";
        for (std::size_t target_index = 0; target_index < configuration.targets.size();
             ++target_index) {
            const auto& target = configuration.targets[target_index];
            if (target_index > 0) {
                output << ',';
            }
            output << "\n      {\"name\": ";
            writeString(output, target.name);
            output << ",\"distance_from_centroid_mm\": ";
            writeNumber(output, target.distance_from_centroid_mm);
            output << ",\"position_mm\": ";
            writeVector3(output, target.position_mm);
            output << ",\"mean_tre_mm\": ";
            writeNumber(output, target.mean_tre_mm);
            output << ",\"median_tre_mm\": ";
            writeNumber(output, target.median_tre_mm);
            output << ",\"p50_tre_mm\": ";
            writeNumber(output, target.p50_tre_mm);
            output << ",\"p95_tre_mm\": ";
            writeNumber(output, target.p95_tre_mm);
            output << ",\"p99_tre_mm\": ";
            writeNumber(output, target.p99_tre_mm);
            output << ",\"rms_tre_mm\": ";
            writeNumber(output, target.rms_tre_mm);
            output << ",\"mean_tre2_mm2\": ";
            writeNumber(output, target.mean_tre2_mm2);
            output << ",\"fitzpatrick_tre2_mm2\": ";
            writeNumber(output, target.fitzpatrick_tre2_mm2);
            output << ",\"fitzpatrick_rms_tre_mm\": ";
            writeNumber(output, std::sqrt(std::max(0.0, target.fitzpatrick_tre2_mm2)));
            output << ",\"relative_error_tre2\": ";
            writeNumber(output, target.relative_error_tre2);
            if (target_index < configuration.tre_samples_mm.size()) {
                constexpr int kBins = 40;
                const double upper = std::max(target.p99_tre_mm * 1.2, 1e-9);
                const double width = upper / static_cast<double>(kBins);
                std::vector<int> counts(kBins, 0);
                for (const double sample : configuration.tre_samples_mm[target_index]) {
                    int bin = static_cast<int>(sample / width);
                    if (bin < 0) {
                        bin = 0;
                    }
                    if (bin >= kBins) {
                        bin = kBins - 1;
                    }
                    ++counts[static_cast<std::size_t>(bin)];
                }
                output << ",\"histogram\": {\"bin_width_mm\": ";
                writeNumber(output, width);
                output << ",\"bin_start_mm\": 0,\"counts\": [";
                for (int bin = 0; bin < kBins; ++bin) {
                    if (bin > 0) {
                        output << ',';
                    }
                    output << counts[static_cast<std::size_t>(bin)];
                }
                output << "]}";
            }
            output << '}';
        }
        output << "\n    ]";
        output << "\n  }";
    }
    output << "\n  ]\n";
}

void writeCsv(const std::filesystem::path& path, const MonteCarloStudy& study) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not write " + path.string());
    }
    if (study.configurations.empty()) {
        throw std::runtime_error("Monte Carlo study has no configurations to export.");
    }

    output << "n_fiducials,fle_rms_mm,trial,fre_rms_mm";
    for (const auto& target : study.configurations.front().targets) {
        output << ",tre_" << target.name << "_mm";
    }
    output << '\n';
    output << std::fixed << std::setprecision(10);

    for (const auto& configuration : study.configurations) {
        if (configuration.targets.size() != study.configurations.front().targets.size()) {
            throw std::runtime_error("Every configuration must report the same targets.");
        }
        for (int trial = 0; trial < study.trials_per_configuration; ++trial) {
            const auto trial_index = static_cast<std::size_t>(trial);
            output << configuration.fiducial_count << ','
                   << configuration.fle_rms_mm << ','
                   << trial << ','
                   << configuration.fre_samples_mm.at(trial_index);
            for (std::size_t target = 0; target < configuration.targets.size(); ++target) {
                output << ',' << configuration.tre_samples_mm.at(target).at(trial_index);
            }
            output << '\n';
        }
    }
}

void writePointsJson(const std::filesystem::path& path, const MonteCarloStudy& study) {
    const int demo_count = std::min(6, static_cast<int>(study.fiducials_mm.cols()));
    if (demo_count < 3) {
        throw std::runtime_error("Not enough fiducials for points.json.");
    }
    const Eigen::Matrix3Xd moving_mm = study.fiducials_mm.leftCols(demo_count);
    const Eigen::Matrix3Xd fixed_mm =
        (study.true_rotation * moving_mm).colwise() + study.true_translation_mm;
    RigidRegistrationOptions options;
    options.emit_warnings = false;
    const RigidRegistrationResult registration = registerRigid(moving_mm, fixed_mm, options);
    const Eigen::Matrix3Xd registered_mm =
        (registration.rotation * moving_mm).colwise() + registration.translation_mm;

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not write " + path.string());
    }

    output << "{\n";
    output << "  \"version\": \"1.0\",\n";
    output << "  \"units\": \"mm\",\n";
    output << "  \"academic\": true,\n";
    output << "  \"clinical\": false,\n";
    output << "  \"description\": \"Noiseless registration of the first "
           << demo_count << " reference fiducials.\",\n";
    output << "  \"point_convention\": "
              "\"C++ clouds are 3 x N, one point per column. JSON points are [x, y, z].\",\n";
    output << "  \"n_fiducials\": " << demo_count << ",\n";
    output << "  \"reference_fiducials_mm\": ";
    writePoints(output, study.fiducials_mm);
    output << ",\n  \"moving_mm\": ";
    writePoints(output, moving_mm);
    output << ",\n  \"fixed_mm\": ";
    writePoints(output, fixed_mm);
    output << ",\n  \"registered_mm\": ";
    writePoints(output, registered_mm);
    output << ",\n  \"rotation\": ";
    writeMatrix3(output, registration.rotation);
    output << ",\n  \"translation_mm\": ";
    writeVector3(output, registration.translation_mm);
    output << ",\n  \"true_rotation\": ";
    writeMatrix3(output, study.true_rotation);
    output << ",\n  \"true_translation_mm\": ";
    writeVector3(output, study.true_translation_mm);
    output << ",\n  \"fre_rms_mm\": ";
    writeNumber(output, registration.fre_rms_mm);
    output << ",\n  \"determinant\": ";
    writeNumber(output, registration.rotation.determinant());
    output << ",\n  \"residuals_mm\": [";
    for (Eigen::Index col = 0; col < demo_count; ++col) {
        if (col > 0) {
            output << ',';
        }
        writeNumber(output, (registered_mm.col(col) - fixed_mm.col(col)).norm());
    }
    output << "]\n}\n";
}

void writeDistanceFigure(const std::filesystem::path& path, const MonteCarloStudy& study) {
    const double fle_mm = 0.25;
    const int series_counts[] = {3, 6, 10};
    const char* series_colors[] = {"#B23A2E", "#35566F", "#D98E04"};

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not write " + path.string());
    }

    double max_distance = 1.0;
    double max_tre = 0.05;
    for (const int count : series_counts) {
        const auto* configuration = findConfiguration(study, count, fle_mm);
        if (configuration == nullptr) {
            continue;
        }
        for (const auto& target : configuration->targets) {
            max_distance = std::max(max_distance, target.distance_from_centroid_mm);
            max_tre = std::max(max_tre, target.mean_tre_mm);
            max_tre = std::max(max_tre, std::sqrt(std::max(0.0, target.fitzpatrick_tre2_mm2)));
        }
    }
    max_tre *= 1.15;

    const double left = 70.0;
    const double top = 48.0;
    const double width = 640.0;
    const double height = 360.0;
    auto xOf = [&](double distance) {
        return left + width * (distance / max_distance);
    };
    auto yOf = [&](double tre) {
        return top + height * (1.0 - tre / max_tre);
    };

    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"860\" height=\"520\" "
              "viewBox=\"0 0 860 520\">\n";
    output << "<rect width=\"860\" height=\"520\" fill=\"#F1F3F5\"/>\n";
    output << "<text x=\"70\" y=\"28\" fill=\"#1B2631\" font-family=\"sans-serif\" "
              "font-size=\"18\">Mean TRE versus distance from the fiducial centroid</text>\n";
    std::ostringstream subtitle;
    subtitle << std::fixed << std::setprecision(2)
             << "FLE " << fle_mm << " mm RMS, " << study.trials_per_configuration
             << " trials, seed " << study.seed
             << ". Solid: measured mean. Dashed: Fitzpatrick RMS.";
    output << "<text x=\"70\" y=\"46\" fill=\"#35566F\" font-family=\"sans-serif\" "
              "font-size=\"12\">"
           << xmlEscape(subtitle.str()) << "</text>\n";

    output << "<line x1=\"" << left << "\" y1=\"" << (top + height)
           << "\" x2=\"" << (left + width) << "\" y2=\"" << (top + height)
           << "\" stroke=\"#1B2631\"/>\n";
    output << "<line x1=\"" << left << "\" y1=\"" << top
           << "\" x2=\"" << left << "\" y2=\"" << (top + height)
           << "\" stroke=\"#1B2631\"/>\n";
    output << "<text x=\"" << (left + width / 2.0 - 80.0) << "\" y=\"500\" fill=\"#1B2631\" "
              "font-family=\"sans-serif\" font-size=\"13\">Distance from centroid (mm)</text>\n";
    output << "<text transform=\"translate(22 " << (top + height / 2.0 + 40.0)
           << ") rotate(-90)\" fill=\"#1B2631\" font-family=\"sans-serif\" font-size=\"13\">"
              "TRE (mm)</text>\n";

    output << std::fixed << std::setprecision(1);
    for (int tick = 0; tick <= 4; ++tick) {
        const double distance = max_distance * static_cast<double>(tick) / 4.0;
        const double x = xOf(distance);
        output << "<line x1=\"" << x << "\" y1=\"" << (top + height)
               << "\" x2=\"" << x << "\" y2=\"" << (top + height + 6)
               << "\" stroke=\"#1B2631\"/>\n";
        output << "<text x=\"" << (x - 12) << "\" y=\"" << (top + height + 22)
               << "\" fill=\"#1B2631\" font-family=\"sans-serif\" font-size=\"11\">"
               << distance << "</text>\n";
    }
    for (int tick = 0; tick <= 4; ++tick) {
        const double tre = max_tre * static_cast<double>(tick) / 4.0;
        const double y = yOf(tre);
        output << "<line x1=\"" << (left - 6) << "\" y1=\"" << y
               << "\" x2=\"" << left << "\" y2=\"" << y
               << "\" stroke=\"#1B2631\"/>\n";
        output << "<text x=\"16\" y=\"" << (y + 4)
               << "\" fill=\"#1B2631\" font-family=\"sans-serif\" font-size=\"11\">"
               << std::setprecision(3) << tre << "</text>\n";
    }

    int legend_y = 70;
    for (int series = 0; series < 3; ++series) {
        const auto* configuration = findConfiguration(study, series_counts[series], fle_mm);
        output << "<text x=\"740\" y=\"" << legend_y << "\" fill=\"" << series_colors[series]
               << "\" font-family=\"sans-serif\" font-size=\"13\">N="
               << series_counts[series];
        if (configuration == nullptr) {
            output << " (absent)";
        }
        output << "</text>\n";
        legend_y += 22;
        if (configuration == nullptr || configuration->targets.empty()) {
            continue;
        }
        output << "<polyline fill=\"none\" stroke=\"" << series_colors[series]
               << "\" stroke-width=\"2\" points=\"";
        for (const auto& target : configuration->targets) {
            output << xOf(target.distance_from_centroid_mm) << ','
                   << yOf(target.mean_tre_mm) << ' ';
        }
        output << "\"/>\n";
        output << "<polyline fill=\"none\" stroke=\"" << series_colors[series]
               << "\" stroke-width=\"1.5\" stroke-dasharray=\"5 4\" points=\"";
        for (const auto& target : configuration->targets) {
            output << xOf(target.distance_from_centroid_mm) << ','
                   << yOf(std::sqrt(std::max(0.0, target.fitzpatrick_tre2_mm2))) << ' ';
        }
        output << "\"/>\n";
        for (const auto& target : configuration->targets) {
            output << "<circle cx=\"" << xOf(target.distance_from_centroid_mm)
                   << "\" cy=\"" << yOf(target.mean_tre_mm)
                   << "\" r=\"3.5\" fill=\"" << series_colors[series] << "\"/>\n";
        }
    }
    output << "</svg>\n";
}

void writeHistogramFigure(const std::filesystem::path& path, const MonteCarloStudy& study) {
    const auto* configuration = findConfiguration(study, 6, 0.25);
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not write " + path.string());
    }
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"860\" height=\"520\" "
              "viewBox=\"0 0 860 520\">\n";
    output << "<rect width=\"860\" height=\"520\" fill=\"#F1F3F5\"/>\n";
    output << "<text x=\"70\" y=\"28\" fill=\"#1B2631\" font-family=\"sans-serif\" "
              "font-size=\"18\">TRE histogram at the fiducial centroid</text>\n";
    if (configuration == nullptr || configuration->targets.empty()
        || configuration->tre_samples_mm.empty()) {
        output << "<text x=\"70\" y=\"80\" fill=\"#B23A2E\" font-family=\"sans-serif\" "
                  "font-size=\"14\">Configuration N=6, FLE=0.25 mm was not in this run.</text>\n";
        output << "</svg>\n";
        return;
    }

    const auto& target = configuration->targets.front();
    constexpr int kBins = 40;
    const double upper = std::max(target.p99_tre_mm * 1.2, 1e-9);
    const double width = upper / static_cast<double>(kBins);
    std::vector<int> counts(kBins, 0);
    int peak = 1;
    for (const double sample : configuration->tre_samples_mm.front()) {
        int bin = static_cast<int>(sample / width);
        if (bin < 0) {
            bin = 0;
        }
        if (bin >= kBins) {
            bin = kBins - 1;
        }
        ++counts[static_cast<std::size_t>(bin)];
        peak = std::max(peak, counts[static_cast<std::size_t>(bin)]);
    }

    std::ostringstream subtitle;
    subtitle << std::fixed << std::setprecision(4)
             << "N=6, FLE 0.25 mm, " << study.trials_per_configuration
             << " trials. p50 " << target.p50_tre_mm
             << " mm, p95 " << target.p95_tre_mm
             << " mm, p99 " << target.p99_tre_mm << " mm.";
    output << "<text x=\"70\" y=\"46\" fill=\"#35566F\" font-family=\"sans-serif\" font-size=\"12\">"
           << xmlEscape(subtitle.str()) << "</text>\n";

    const double left = 70.0;
    const double top = 70.0;
    const double plot_width = 720.0;
    const double plot_height = 360.0;
    const double bar_width = plot_width / static_cast<double>(kBins);
    for (int bin = 0; bin < kBins; ++bin) {
        const double bar_height =
            plot_height * static_cast<double>(counts[static_cast<std::size_t>(bin)])
            / static_cast<double>(peak);
        const double x = left + bar_width * static_cast<double>(bin);
        const double y = top + plot_height - bar_height;
        output << "<rect x=\"" << x << "\" y=\"" << y
               << "\" width=\"" << (bar_width - 1.0) << "\" height=\"" << bar_height
               << "\" fill=\"#35566F\"/>\n";
    }
    auto marker = [&](double tre_mm, const char* color, const char* label) {
        const double x = left + plot_width * (tre_mm / upper);
        output << "<line x1=\"" << x << "\" y1=\"" << top << "\" x2=\"" << x
               << "\" y2=\"" << (top + plot_height) << "\" stroke=\"" << color
               << "\" stroke-dasharray=\"4 3\"/>\n";
        output << "<text x=\"" << (x + 4) << "\" y=\"" << (top + 16)
               << "\" fill=\"" << color << "\" font-family=\"sans-serif\" font-size=\"12\">"
               << label << "</text>\n";
    };
    marker(target.p50_tre_mm, "#1B2631", "p50");
    marker(target.p95_tre_mm, "#D98E04", "p95");
    marker(target.p99_tre_mm, "#B23A2E", "p99");
    output << "<text x=\"330\" y=\"500\" fill=\"#1B2631\" font-family=\"sans-serif\" "
              "font-size=\"13\">Centroid TRE (mm)</text>\n";
    output << "</svg>\n";
}

} // namespace

void exportMonteCarloArtifacts(
    const MonteCarloStudy& study,
    const std::filesystem::path& directory) {
    std::filesystem::create_directories(directory);
    writeResultsJson(directory / "results.json", study);
    writeCsv(directory / "mc.csv", study);
    writePointsJson(directory / "points.json", study);
    writeDistanceFigure(directory / "tre_vs_distance.svg", study);
    writeHistogramFigure(directory / "tre_histogram_n6_fle0p25.svg", study);
}

} // namespace surgitrack
