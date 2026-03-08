#include "AudioPipeline.h"

namespace core {

AudioPipeline::AudioPipeline(int sample_rate) 
    : sample_rate_(sample_rate),
      spectrum_analyzer_(sample_rate),
      limiter_(sample_rate)
{}

PipelineResult AudioPipeline::run(
    const std::vector<double>& samples,
    const std::map<std::string, double>& user_preferences,
    const std::optional<db::SpeakerCapability>& speaker_capability,
    const std::string& platform_hint,
    const std::map<int, double>& speaker_bands,
    const std::map<int, std::string>& speaker_issues,
    const std::string& speaker_system_id)
{
    PipelineResult result;

    // 1 - Source detection (fetches source_profiles from DB)
    auto source_info = source_detector_.detect(platform_hint);

    // 2 - Spectrum analyzer
    auto spectrum = spectrum_analyzer_.analyze(samples);

    // 3 - Track profiler
    auto track_profile = track_profiler_.profile(spectrum);

    // 4 - User preferences -> Target EQ
    auto target_curve = preference_engine_.build_target_curve(user_preferences);

    // 5 - Speaker constraints
    auto constraints = speaker_model_.load_constraints(speaker_capability);

    // 6 - EQ optimizer (degradation compensation)
    result.final_eq = eq_optimizer_.compute(target_curve, source_info, track_profile, constraints,
        speaker_bands, speaker_issues);

    // 7 - DSP Engine 
    // In actual implementation, we'd call the dsp_engine via FFI/C++ linkage here to process `samples` with `result.final_eq`.
    // For now we simulate with a pass-through
    std::vector<double> dsp_processed = samples;

    // 8 - Limiter
    result.processed_samples = limiter_.limit(dsp_processed);
    result.headroom_db = limiter_.check_headroom(result.processed_samples);
    result.peak_limit_db = limiter_.get_ceiling_db();

    if (result.headroom_db < 0) {
        result.warnings.push_back("Signal exceeded limiter ceiling.");
    }

    result.track_profile = track_profile.profile_data;
    return result;
}

} // namespace core
