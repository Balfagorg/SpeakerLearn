#pragma once
#include <vector>
#include <string>
#include <map>
#include "stages/Stages.h"
#include "../db/Models.h"

namespace core {

struct PipelineResult {
    std::map<std::string, double> final_eq;
    double peak_limit_db = -1.0;
    std::map<std::string, double> track_profile;
    double headroom_db = 0.0;
    std::vector<double> processed_samples;
    std::vector<std::string> warnings;
};

class AudioPipeline {
public:
    AudioPipeline(int sample_rate = 44100);
    
    PipelineResult run(
        const std::vector<double>& samples,
        const std::map<std::string, double>& user_preferences,
        const std::optional<db::SpeakerCapability>& speaker_capability,
        const std::string& platform_hint = "",
        const std::map<int, double>& speaker_bands = {},
        const std::map<int, std::string>& speaker_issues = {},
        const std::string& speaker_system_id = ""
    );

private:
    int sample_rate_;
    stages::SourceDetector source_detector_;
    stages::SpectrumAnalyzer spectrum_analyzer_;
    stages::TrackProfiler track_profiler_;
    stages::PreferenceEngine preference_engine_;
    stages::SpeakerModel speaker_model_;
    stages::EQOptimizer eq_optimizer_;
    stages::SafetyLimiter limiter_;
};

} // namespace core
