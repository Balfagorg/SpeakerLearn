#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include <sqlite3.h>
#include "Models.h"

namespace db {

class DatabaseManager {
public:
    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    // Delete copy/move constructors
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool init_db(const std::string& db_path = "adaptive_audio.db");
    void close_db();

    // Commands
    bool execute_sql(const std::string& sql);

    // CRUD for SpeakerSystems
    bool save_speaker_system(const SpeakerSystem& sys);
    std::vector<SpeakerSystem> get_speaker_systems(const std::string& user_id);

    // CRUD for SpeakerCapability
    bool save_speaker_capability(const SpeakerCapability& cap);
    std::optional<SpeakerCapability> get_speaker_capability(const std::string& speaker_system_id);

    // CRUD for UserPreferences
    bool save_user_preference(const UserPreference& pref);
    std::vector<UserPreference> get_user_preferences(const std::string& user_id);

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    sqlite3* db_ = nullptr;
    std::mutex db_mutex_;

    void seed_default_source_profiles();
};

} // namespace db
