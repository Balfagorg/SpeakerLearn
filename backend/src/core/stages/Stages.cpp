#include "Stages.h"
#include "../../db/DatabaseManager.h"

namespace core::stages {

SourceInfo SourceDetector::detect(const std::string& platform_hint) {
    SourceInfo info;
    std::string key = platform_hint.empty() ? "local_flac" : platform_hint;
    if (key == "local") key = "local_flac";
    auto profile = db::DatabaseManager::getInstance().get_source_profile(key);
    if (profile) {
        info.platform_name = profile->platform_name;
        info.offsets["sub_bass"] = profile->sub_bass_offset;
        info.offsets["bass"] = profile->bass_offset;
        info.offsets["low_mid"] = profile->low_mid_offset;
        info.offsets["mid"] = profile->mid_offset;
        info.offsets["high"] = profile->high_offset;
        info.offsets["air"] = profile->air_offset;
    } else {
        info.platform_name = key;
        info.offsets = {{"sub_bass", 0.0}, {"bass", 0.0}, {"low_mid", 0.0}, {"mid", 0.0}, {"high", 0.0}, {"air", 0.0}};
    }
    return info;
}

Spectrum SpectrumAnalyzer::analyze(const std::vector<double>& audio) {
    Spectrum spec;
    spec.band_energies = {{"sub_bass", 0.1}, {"bass", 0.2}, {"low_mid", 0.3}, {"mid", 0.4}, {"high", 0.2}, {"air", 0.1}};
    return spec;
}

TrackProfile TrackProfiler::profile(const Spectrum& spectrum) {
    TrackProfile tp;
    tp.profile_data = {{"avg_loudness", -14.0}, {"dynamic_range", 8.0}};
    return tp;
}

TargetEQCurve PreferenceEngine::build_target_curve(const std::map<std::string, double>& user_preferences) {
    TargetEQCurve eq;
    eq.curve = {{"sub_bass", 0.0}, {"bass", 0.0}, {"low_mid", 0.0}, {"mid", 0.0}, {"high", 0.0}, {"air", 0.0}};
    // In full implementation, we multiply preference weights by influence curves
    return eq;
}

SpeakerConstraints SpeakerModel::load_constraints(const std::optional<db::SpeakerCapability>& cap) {
    SpeakerConstraints sc;
    sc.limitations = {{"sub_bass", 0.5}, {"bass", 0.5}, {"low_mid", 0.7}, {"mid", 0.8}, {"high", 0.7}, {"air", 0.5}};
    if (cap) {
        sc.limitations["sub_bass"] = cap->sub_bass_capability;
        sc.limitations["bass"] = cap->bass_capability;
        sc.limitations["low_mid"] = cap->low_mid_capability;
        sc.limitations["mid"] = cap->mid_capability;
        sc.limitations["high"] = cap->high_capability;
        sc.limitations["air"] = cap->air_capability;
    }
    return sc;
}

static const std::vector<int> BANDS_7_HZ = {60, 170, 350, 1000, 3500, 8000, 16000};

static double issueToCompensation(const std::string& issue, int hz) {
    if (issue.empty() || issue == "none") return 0.0;
    if (issue == "muffled") return (hz >= 1000 && hz <= 3500) ? 1.5 : 0.0;
    if (issue == "harsh") return (hz >= 3500 && hz <= 8000) ? -1.5 : 0.0;
    if (issue == "tinny") return (hz >= 8000) ? -1.0 : (hz >= 60 && hz <= 350) ? 0.5 : 0.0;
    if (issue == "crackle") return -1.0;
    if (issue == "rolloff") return (hz <= 170) ? 2.0 : 0.0;
    if (issue == "inaudible") return 2.0;
    if (issue == "rattle" || issue == "feedback" || issue == "boxiness" || issue == "inconsistent") return -0.5;
    return 0.0;
}

std::map<std::string, double> EQOptimizer::compute(
    const TargetEQCurve& user_curve,
    const SourceInfo& source_info,
    const TrackProfile& track_profile,
    const SpeakerConstraints& constraints,
    const std::map<int, double>& speaker_bands,
    const std::map<int, std::string>& speaker_issues)
{
    std::map<std::string, double> final_eq;
    const auto& off = source_info.offsets;
    for (int hz : BANDS_7_HZ) {
        double v = 0.0;
        if (speaker_bands.count(hz)) v = speaker_bands.at(hz);
        double inv_src = 0.0;
        if (hz <= 60) inv_src = -off.count("sub_bass") ? off.at("sub_bass") : 0;
        else if (hz <= 170) inv_src = -0.5 * (off.count("bass") ? off.at("bass") : 0) - 0.5 * (off.count("low_mid") ? off.at("low_mid") : 0);
        else if (hz <= 350) inv_src = -(off.count("low_mid") ? off.at("low_mid") : 0);
        else if (hz <= 1000) inv_src = -(off.count("mid") ? off.at("mid") : 0);
        else if (hz <= 3500) inv_src = -0.5 * (off.count("mid") ? off.at("mid") : 0) - 0.5 * (off.count("high") ? off.at("high") : 0);
        else if (hz <= 8000) inv_src = -(off.count("high") ? off.at("high") : 0);
        else inv_src = -(off.count("air") ? off.at("air") : 0);
        v += inv_src;
        if (speaker_issues.count(hz)) v += issueToCompensation(speaker_issues.at(hz), hz);
        double spec = 0.0;
        if (track_profile.profile_data.count("avg_loudness")) spec = track_profile.profile_data.at("avg_loudness") < -18 ? 0.5 : 0;
        v += spec * 0.3;
        final_eq[std::to_string(hz)] = std::max(-16.0, std::min(16.0, v));
    }
    return final_eq;
}

std::vector<double> SafetyLimiter::limit(const std::vector<double>& audio) {
    // Pass-through for stub
    return audio;
}

double SafetyLimiter::check_headroom(const std::vector<double>& audio) {
    return 3.0; // 3dB headroom stub
}

} // namespace core::stages
