#pragma once
#include <vector>
#include <string>
#include <map>
#include <optional>
#include "../../db/Models.h"

namespace core::stages {

struct SourceInfo {
    std::string platform_name;
    std::map<std::string, double> offsets;
};

class SourceDetector {
public:
    SourceInfo detect(const std::string& platform_hint);
};

struct Spectrum {
    std::map<std::string, double> band_energies;
};

class SpectrumAnalyzer {
public:
    SpectrumAnalyzer(int sample_rate) : sample_rate_(sample_rate) {}
    Spectrum analyze(const std::vector<double>& audio);
private:
    int sample_rate_;
};

struct TrackProfile {
    std::map<std::string, double> profile_data;
};

class TrackProfiler {
public:
    TrackProfile profile(const Spectrum& spectrum);
    TrackProfile profile_from_samples(const std::vector<double>& samples);
};

struct TargetEQCurve {
    std::map<std::string, double> curve;
};

class PreferenceEngine {
public:
    TargetEQCurve build_target_curve(const std::map<std::string, double>& user_preferences);
};

struct SpeakerConstraints {
    std::map<std::string, double> limitations;
};

class SpeakerModel {
public:
    SpeakerConstraints load_constraints(const std::optional<db::SpeakerCapability>& cap);
};

class EQOptimizer {
public:
    std::map<std::string, double> compute(
        const TargetEQCurve& user_curve,
        const SourceInfo& source_info,
        const TrackProfile& track_profile,
        const SpeakerConstraints& constraints,
        const std::map<int, double>& speaker_bands = {},
        const std::map<int, std::string>& speaker_issues = {}
    );
};

class SafetyLimiter {
public:
    SafetyLimiter(int sample_rate) : sample_rate_(sample_rate) {}
    std::vector<double> limit(const std::vector<double>& audio);
    double check_headroom(const std::vector<double>& audio);
    double get_ceiling_db() const { return -1.0; } // placeholder
private:
    int sample_rate_;
};

} // namespace core::stages
