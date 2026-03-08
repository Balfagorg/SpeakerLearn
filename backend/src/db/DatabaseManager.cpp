#include "DatabaseManager.h"
#include <iostream>

namespace db {

DatabaseManager::~DatabaseManager() {
    close_db();
}

bool DatabaseManager::init_db(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_ != nullptr) return true;

    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    const std::string create_tables_sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id VARCHAR(36) PRIMARY KEY,
            username VARCHAR(128) UNIQUE NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        
        CREATE TABLE IF NOT EXISTS speaker_systems (
            id VARCHAR(36) PRIMARY KEY,
            user_id VARCHAR(36) NOT NULL,
            name VARCHAR(128) NOT NULL,
            device_identifier VARCHAR(256),
            channel_config VARCHAR(32) DEFAULT 'stereo',
            is_active BOOLEAN DEFAULT 0,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        );

        CREATE TABLE IF NOT EXISTS speaker_capabilities (
            id VARCHAR(36) PRIMARY KEY,
            speaker_system_id VARCHAR(36) UNIQUE NOT NULL,
            sub_bass_capability FLOAT DEFAULT 0.5,
            bass_capability FLOAT DEFAULT 0.5,
            low_mid_capability FLOAT DEFAULT 0.7,
            mid_capability FLOAT DEFAULT 0.8,
            high_capability FLOAT DEFAULT 0.7,
            air_capability FLOAT DEFAULT 0.5,
            max_safe_spl FLOAT DEFAULT 85.0,
            FOREIGN KEY(speaker_system_id) REFERENCES speaker_systems(id)
        );

        CREATE TABLE IF NOT EXISTS calibration_results (
            id VARCHAR(36) PRIMARY KEY,
            speaker_system_id VARCHAR(36) NOT NULL,
            frequency_band VARCHAR(32) NOT NULL,
            volume_rating INTEGER NOT NULL,
            quality_rating BOOLEAN NOT NULL,
            tested_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(speaker_system_id) REFERENCES speaker_systems(id)
        );

        CREATE TABLE IF NOT EXISTS user_preferences (
            id VARCHAR(36) PRIMARY KEY,
            user_id VARCHAR(36) NOT NULL,
            preference_key VARCHAR(64) NOT NULL,
            weight FLOAT DEFAULT 0.5,
            UNIQUE(user_id, preference_key),
            FOREIGN KEY(user_id) REFERENCES users(id)
        );

        CREATE TABLE IF NOT EXISTS source_profiles (
            id VARCHAR(36) PRIMARY KEY,
            platform_name VARCHAR(64) UNIQUE NOT NULL,
            sub_bass_offset FLOAT DEFAULT 0.0,
            bass_offset FLOAT DEFAULT 0.0,
            low_mid_offset FLOAT DEFAULT 0.0,
            mid_offset FLOAT DEFAULT 0.0,
            high_offset FLOAT DEFAULT 0.0,
            air_offset FLOAT DEFAULT 0.0,
            description TEXT
        );

        CREATE TABLE IF NOT EXISTS eq_presets (
            id VARCHAR(128) PRIMARY KEY,
            user_id VARCHAR(36) NOT NULL,
            name VARCHAR(128) NOT NULL,
            speaker_system_id VARCHAR(36),
            bands_json TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";

    if (!execute_sql(create_tables_sql)) {
        return false;
    }

    // Enable WAL mode for better concurrency
    execute_sql("PRAGMA journal_mode=WAL;");

    seed_default_source_profiles();
    return true;
}

void DatabaseManager::close_db() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DatabaseManager::execute_sql(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

void DatabaseManager::seed_default_source_profiles() {
    const char* count_sql = "SELECT COUNT(*) FROM source_profiles;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, count_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            if (count > 0) {
                sqlite3_finalize(stmt);
                return; // Already seeded
            }
        }
    }
    sqlite3_finalize(stmt);

    const std::string seed_sql = R"(
        INSERT INTO source_profiles (id, platform_name, sub_bass_offset, bass_offset, low_mid_offset, mid_offset, high_offset, air_offset, description) VALUES
        ('1', 'spotify', 1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 'Strong loudness normalisation — slight bass boost, treble roll-off.'),
        ('2', 'apple_music', 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 'Wider stereo mix — elevated mids and highs.'),
        ('3', 'youtube', -0.5, 0.0, 0.0, 0.5, -1.5, 0.0, 'Aggressive compression — reduced bass and high-end.'),
        ('4', 'local_flac', 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 'Neutral profile — no correction needed.');
    )";
    execute_sql(seed_sql);
}

bool DatabaseManager::save_speaker_system(const SpeakerSystem& sys) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT OR REPLACE INTO speaker_systems (id, user_id, name, device_identifier, channel_config, is_active, created_at) VALUES (?, ?, ?, ?, ?, ?, COALESCE(?, CURRENT_TIMESTAMP));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, sys.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sys.user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, sys.name.c_str(), -1, SQLITE_TRANSIENT);
    if (sys.device_identifier) {
        sqlite3_bind_text(stmt, 4, sys.device_identifier->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 4);
    }
    sqlite3_bind_text(stmt, 5, sys.channel_config.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, sys.is_active ? 1 : 0);
    if (!sys.created_at.empty()) {
        sqlite3_bind_text(stmt, 7, sys.created_at.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 7);
    }

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<SpeakerSystem> DatabaseManager::get_speaker_systems(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<SpeakerSystem> systems;
    const char* sql = "SELECT id, user_id, name, device_identifier, channel_config, is_active, created_at FROM speaker_systems WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return systems;

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SpeakerSystem sys;
        sys.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        sys.user_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        sys.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* dev_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (dev_id) sys.device_identifier = dev_id;
        sys.channel_config = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        sys.is_active = sqlite3_column_int(stmt, 5) != 0;
        sys.created_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        systems.push_back(sys);
    }
    sqlite3_finalize(stmt);
    return systems;
}

bool DatabaseManager::save_speaker_capability(const SpeakerCapability& cap) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT OR REPLACE INTO speaker_capabilities (id, speaker_system_id, sub_bass_capability, bass_capability, low_mid_capability, mid_capability, high_capability, air_capability, max_safe_spl) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, cap.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, cap.speaker_system_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, cap.sub_bass_capability);
    sqlite3_bind_double(stmt, 4, cap.bass_capability);
    sqlite3_bind_double(stmt, 5, cap.low_mid_capability);
    sqlite3_bind_double(stmt, 6, cap.mid_capability);
    sqlite3_bind_double(stmt, 7, cap.high_capability);
    sqlite3_bind_double(stmt, 8, cap.air_capability);
    sqlite3_bind_double(stmt, 9, cap.max_safe_spl);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::optional<SpeakerCapability> DatabaseManager::get_speaker_capability(const std::string& speaker_system_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT id, speaker_system_id, sub_bass_capability, bass_capability, low_mid_capability, mid_capability, high_capability, air_capability, max_safe_spl FROM speaker_capabilities WHERE speaker_system_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, speaker_system_id.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<SpeakerCapability> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SpeakerCapability cap;
        cap.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        cap.speaker_system_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cap.sub_bass_capability = sqlite3_column_double(stmt, 2);
        cap.bass_capability = sqlite3_column_double(stmt, 3);
        cap.low_mid_capability = sqlite3_column_double(stmt, 4);
        cap.mid_capability = sqlite3_column_double(stmt, 5);
        cap.high_capability = sqlite3_column_double(stmt, 6);
        cap.air_capability = sqlite3_column_double(stmt, 7);
        cap.max_safe_spl = sqlite3_column_double(stmt, 8);
        result = cap;
    }
    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::save_user_preference(const UserPreference& pref) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT OR REPLACE INTO user_preferences (id, user_id, preference_key, weight) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, pref.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pref.user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, pref.preference_key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, pref.weight);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<UserPreference> DatabaseManager::get_user_preferences(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<UserPreference> prefs;
    const char* sql = "SELECT id, user_id, preference_key, weight FROM user_preferences WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return prefs;

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserPreference pref;
        pref.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        pref.user_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        pref.preference_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        pref.weight = sqlite3_column_double(stmt, 3);
        prefs.push_back(pref);
    }
    sqlite3_finalize(stmt);
    return prefs;
}

bool DatabaseManager::save_calibration_result(const CalibrationResult& result) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT INTO calibration_results (id, speaker_system_id, frequency_band, volume_rating, quality_rating, tested_at) VALUES (?, ?, ?, ?, ?, COALESCE(?, CURRENT_TIMESTAMP));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, result.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, result.speaker_system_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, result.frequency_band.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, result.volume_rating);
    sqlite3_bind_int(stmt, 5, result.quality_rating ? 1 : 0);
    if (!result.tested_at.empty()) {
        sqlite3_bind_text(stmt, 6, result.tested_at.c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 6);
    }

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::set_active_speaker(const std::string& user_id, const std::string& speaker_system_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* clear_sql = "UPDATE speaker_systems SET is_active = 0 WHERE user_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, clear_sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (speaker_system_id.empty()) return true;

    const char* set_sql = "UPDATE speaker_systems SET is_active = 1 WHERE id = ? AND user_id = ?;";
    if (sqlite3_prepare_v2(db_, set_sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, speaker_system_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DatabaseManager::save_eq_preset(const std::string& id, const std::string& user_id, const std::string& name,
                                     const std::string& speaker_system_id, const std::string& bands_json) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "INSERT OR REPLACE INTO eq_presets (id, user_id, name, speaker_system_id, bands_json) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, speaker_system_id.empty() ? nullptr : speaker_system_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, bands_json.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<EqPreset> DatabaseManager::get_eq_presets(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<EqPreset> presets;
    const char* sql = "SELECT id, name, speaker_system_id, bands_json FROM eq_presets WHERE user_id = ? ORDER BY created_at DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return presets;

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EqPreset p;
        p.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* spk = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (spk) p.speaker_system_id = spk;
        p.bands_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        presets.push_back(p);
    }
    sqlite3_finalize(stmt);
    return presets;
}

bool DatabaseManager::delete_eq_preset(const std::string& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "DELETE FROM eq_presets WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::optional<std::string> DatabaseManager::get_eq_preset_bands(const std::string& id) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    const char* sql = "SELECT bands_json FROM eq_presets WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<std::string> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return result;
}

} // namespace db
