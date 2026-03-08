#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json& j, const std::optional<T>& opt) {
            if (opt == std::nullopt) {
                j = nullptr;
            } else {
                j = *opt;
            }
        }

        static void from_json(const json& j, std::optional<T>& opt) {
            if (j.is_null()) {
                opt = std::nullopt;
            } else {
                opt = j.template get<T>();
            }
        }
    };
}

namespace db {

struct User {
    std::string id;
    std::string username;
    std::string created_at;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(User, id, username, created_at)
};

struct SpeakerSystem {
    std::string id;
    std::string user_id;
    std::string name;
    std::optional<std::string> device_identifier;
    std::string channel_config;
    bool is_active;
    std::string created_at;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpeakerSystem, id, user_id, name, device_identifier, channel_config, is_active, created_at)
};

struct SpeakerCapability {
    std::string id;
    std::string speaker_system_id;
    double sub_bass_capability;
    double bass_capability;
    double low_mid_capability;
    double mid_capability;
    double high_capability;
    double air_capability;
    double max_safe_spl;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SpeakerCapability, id, speaker_system_id, sub_bass_capability, bass_capability, low_mid_capability, mid_capability, high_capability, air_capability, max_safe_spl)
};

struct CalibrationResult {
    std::string id;
    std::string speaker_system_id;
    std::string frequency_band;
    int volume_rating;
    bool quality_rating;
    std::string issue;  // e.g. "muffled", "harsh", "none" — for issueToCompensation
    std::string tested_at;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CalibrationResult, id, speaker_system_id, frequency_band, volume_rating, quality_rating, issue, tested_at)
};

struct UserPreference {
    std::string id;
    std::string user_id;
    std::string preference_key;
    double weight;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(UserPreference, id, user_id, preference_key, weight)
};

struct EqPreset {
    std::string id;
    std::string name;
    std::string speaker_system_id;
    std::string bands_json;
};

struct SourceProfile {
    std::string id;
    std::string platform_name;
    double sub_bass_offset;
    double bass_offset;
    double low_mid_offset;
    double mid_offset;
    double high_offset;
    double air_offset;
    std::optional<std::string> description;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SourceProfile, id, platform_name, sub_bass_offset, bass_offset, low_mid_offset, mid_offset, high_offset, air_offset, description)
};

} // namespace db
