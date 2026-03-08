#include "HttpServer.h"
#include <iostream>
#include <random>
#include <sstream>
#include <chrono>
#include <nlohmann/json.hpp>
#include "../db/DatabaseManager.h"

using json = nlohmann::json;

namespace {
std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    const char* hex = "0123456789abcdef";
    std::string id;
    id.reserve(36);
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) id += '-';
        else if (i == 14) id += '4';
        else if (i == 19) id += hex[(dis(gen) & 3) | 8];
        else id += hex[dis(gen)];
    }
    return id;
}
} // namespace

namespace api {

HttpServer::HttpServer(int port) : port_(port), pipeline_(44100) {
    setup_routes();
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::setup_routes() {
    // CORS middleware equivalent
    svr_.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "*");
        if (req.method == "OPTIONS") {
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Health / Landing
    svr_.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        json response = {
            {"service", "Adaptive Audio Balancing Platform (C++ Native)"},
            {"version", "0.1.0"},
            {"status", "online"}
        };
        res.set_content(response.dump(), "application/json");
    });

    // Audio optimize
    svr_.Post("/audio/optimize", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::vector<double> samples;
            if (body.contains("samples") && body["samples"].is_array()) {
                for (const auto& v : body["samples"]) samples.push_back(v.get<double>());
            }
            if (samples.empty()) samples = {0.1, -0.1, 0.2, -0.2};

            std::map<std::string, double> prefs;
            if (body.contains("preferences") && body["preferences"].is_object()) {
                for (auto it = body["preferences"].begin(); it != body["preferences"].end(); ++it)
                    prefs[it.key()] = it.value().get<double>();
            }
            if (prefs.empty()) prefs = {{"bass_boost", 0.5}};

            std::string platform = body.value("platform_hint", "local");
            std::string speaker_system_id = body.value("speaker_system_id", "");

            std::map<int, double> speaker_bands;
            std::map<int, std::string> speaker_issues;
            if (body.contains("speaker_calibration") && body["speaker_calibration"].is_object()) {
                auto cal = body["speaker_calibration"];
                if (cal.contains("bands") && cal["bands"].is_object()) {
                    for (auto it = cal["bands"].begin(); it != cal["bands"].end(); ++it) {
                        try { speaker_bands[std::stoi(it.key())] = it.value().get<double>(); } catch (...) {}
                    }
                }
                if (cal.contains("issues") && cal["issues"].is_object()) {
                    for (auto it = cal["issues"].begin(); it != cal["issues"].end(); ++it) {
                        try { speaker_issues[std::stoi(it.key())] = it.value().get<std::string>(); } catch (...) {}
                    }
                }
            }

            auto pipe_res = pipeline_.run(samples, prefs, std::nullopt, platform,
                speaker_bands, speaker_issues, speaker_system_id);

            json response = {
                {"final_eq", pipe_res.final_eq},
                {"headroom_db", pipe_res.headroom_db},
                {"peak_limit_db", pipe_res.peak_limit_db},
                {"track_profile", pipe_res.track_profile},
                {"warnings", pipe_res.warnings}
            };
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Speaker systems — create (persists to DB)
    svr_.Post("/speaker-systems/create", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string name = body.value("name", "Unnamed Speaker");
            std::string user_id = body.value("user_id", "default-user");
            std::string device_id = body.value("device_identifier", "");
            std::string channel = body.value("channel_config", "stereo");

            db::SpeakerSystem sys;
            sys.id = generate_uuid();
            sys.user_id = user_id;
            sys.name = name;
            sys.device_identifier = device_id.empty() ? std::nullopt : std::make_optional(device_id);
            sys.channel_config = channel;
            sys.is_active = false;

            if (db::DatabaseManager::getInstance().save_speaker_system(sys)) {
                res.set_content(json{{"status", "created"}, {"id", sys.id}}.dump(), "application/json");
            } else {
                res.status = 500;
                res.set_content(json{{"error", "Failed to save speaker system"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Speaker systems — calibrate (submit calibration ratings from Learn flow)
    svr_.Post("/speaker-systems/calibrate", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string speaker_system_id = body.value("speaker_system_id", "");
            if (speaker_system_id.empty()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing speaker_system_id"}}.dump(), "application/json");
                return;
            }
            if (!body.contains("readings") || !body["readings"].is_array()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing readings array"}}.dump(), "application/json");
                return;
            }
            bool ok = true;
            for (const auto& r : body["readings"]) {
                db::CalibrationResult cr;
                cr.id = generate_uuid();
                cr.speaker_system_id = speaker_system_id;
                cr.frequency_band = r.value("label", r.value("name", ""));
                cr.volume_rating = r.value("val", 50);
                cr.quality_rating = (r.value("issue", "none") == "none");
                if (!db::DatabaseManager::getInstance().save_calibration_result(cr)) ok = false;
            }
            res.set_content(json{{"status", ok ? "saved" : "partial_save"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Speaker systems — update (name only)
    svr_.Post("/speaker-systems/update", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string id = body.value("id", "");
            std::string name = body.value("name", "");
            if (id.empty()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing id"}}.dump(), "application/json");
                return;
            }
            if (name.empty()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing name"}}.dump(), "application/json");
                return;
            }
            if (db::DatabaseManager::getInstance().update_speaker_system_name(id, name)) {
                res.set_content(json{{"status", "updated"}}.dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content(json{{"error", "Speaker system not found"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Speaker systems — calibrate-replace (delete existing, insert new readings)
    svr_.Post("/speaker-systems/calibrate-replace", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string speaker_system_id = body.value("speaker_system_id", "");
            if (speaker_system_id.empty()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing speaker_system_id"}}.dump(), "application/json");
                return;
            }
            if (!body.contains("readings") || !body["readings"].is_array()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing readings array"}}.dump(), "application/json");
                return;
            }
            std::vector<db::CalibrationResult> results;
            for (const auto& r : body["readings"]) {
                db::CalibrationResult cr;
                cr.id = generate_uuid();
                cr.speaker_system_id = speaker_system_id;
                cr.frequency_band = r.value("label", r.value("name", ""));
                cr.volume_rating = r.value("val", 50);
                cr.quality_rating = (r.value("issue", "none") == "none");
                results.push_back(cr);
            }
            if (db::DatabaseManager::getInstance().replace_calibration_results(speaker_system_id, results)) {
                res.set_content(json{{"status", "replaced"}}.dump(), "application/json");
            } else {
                res.status = 500;
                res.set_content(json{{"error", "Failed to replace calibration"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Speaker systems — switch active
    svr_.Post("/speaker-systems/switch", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string user_id = body.value("user_id", "default-user");
            std::string speaker_system_id = body.value("speaker_system_id", "");
            if (db::DatabaseManager::getInstance().set_active_speaker(user_id, speaker_system_id)) {
                res.set_content(json{{"status", "switched"}}.dump(), "application/json");
            } else {
                res.status = 500;
                res.set_content(json{{"error", "Failed to switch speaker"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Speaker systems — get active
    svr_.Get("/speaker-systems/active", [](const httplib::Request& req, httplib::Response& res) {
        std::string user_id = req.has_param("user_id") ? req.get_param_value("user_id") : "default-user";
        std::string active_id = db::DatabaseManager::getInstance().get_active_speaker(user_id);
        res.set_content(json{{"speaker_system_id", active_id}}.dump(), "application/json");
    });

    // Speaker systems — calibration (get readings for a speaker)
    svr_.Get("/speaker-systems/calibration", [](const httplib::Request& req, httplib::Response& res) {
        std::string speaker_system_id = req.has_param("speaker_system_id") ? req.get_param_value("speaker_system_id") : "";
        if (speaker_system_id.empty()) {
            res.status = 400;
            res.set_content(json{{"error", "Missing speaker_system_id"}}.dump(), "application/json");
            return;
        }
        auto results = db::DatabaseManager::getInstance().get_calibration_results(speaker_system_id);
        json arr = json::array();
        for (const auto& r : results) {
            arr.push_back({{"label", r.frequency_band}, {"val", r.volume_rating}, {"issue", r.quality_rating ? "none" : "other"}});
        }
        res.set_content(arr.dump(), "application/json");
    });

    // Speaker systems — list
    svr_.Get("/speaker-systems/list", [](const httplib::Request& req, httplib::Response& res) {
        std::string user_id = req.has_param("user_id") ? req.get_param_value("user_id") : "default-user";
        auto systems = db::DatabaseManager::getInstance().get_speaker_systems(user_id);
        json arr = json::array();
        for (const auto& s : systems) {
            arr.push_back({{"id", s.id}, {"name", s.name}, {"device_identifier", s.device_identifier.value_or("")},
                          {"channel_config", s.channel_config}, {"is_active", s.is_active}, {"created_at", s.created_at}});
        }
        res.set_content(arr.dump(), "application/json");
    });

    // Speaker systems — delete
    svr_.Post("/speaker-systems/delete", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string id = body.value("id", "");
            if (id.empty()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing speaker_system_id"}}.dump(), "application/json");
                return;
            }
            if (db::DatabaseManager::getInstance().delete_speaker_system(id)) {
                res.set_content(json{{"status", "deleted"}}.dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content(json{{"error", "Speaker system not found"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Preferences — get
    svr_.Get("/preferences", [](const httplib::Request& req, httplib::Response& res) {
        std::string user_id = req.has_param("user_id") ? req.get_param_value("user_id") : "default-user";
        auto prefs = db::DatabaseManager::getInstance().get_user_preferences(user_id);
        json obj = json::object();
        for (const auto& p : prefs) obj[p.preference_key] = p.weight;
        res.set_content(obj.dump(), "application/json");
    });

    // Preferences — update
    svr_.Post("/preferences/update", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string user_id = body.value("user_id", "default-user");
            if (!body.contains("preferences") || !body["preferences"].is_object()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing 'preferences' object"}}.dump(), "application/json");
                return;
            }
            bool ok = true;
            for (auto it = body["preferences"].begin(); it != body["preferences"].end(); ++it) {
                db::UserPreference pref;
                pref.id = generate_uuid();
                pref.user_id = user_id;
                pref.preference_key = it.key();
                pref.weight = it.value().get<double>();
                if (!db::DatabaseManager::getInstance().save_user_preference(pref)) ok = false;
            }
            res.set_content(json{{"status", ok ? "updated" : "partial_update"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // EQ presets — list
    svr_.Get("/eq-presets/list", [](const httplib::Request& req, httplib::Response& res) {
        std::string user_id = req.has_param("user_id") ? req.get_param_value("user_id") : "default-user";
        auto presets = db::DatabaseManager::getInstance().get_eq_presets(user_id);
        json arr = json::array();
        for (const auto& p : presets) {
            json bands = json::object();
            try { bands = json::parse(p.bands_json); } catch (...) {}
            arr.push_back({{"id", p.id}, {"name", p.name}, {"speaker_system_id", p.speaker_system_id}, {"bands", bands}});
        }
        res.set_content(arr.dump(), "application/json");
    });

    // EQ presets — save
    svr_.Post("/eq-presets/save", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string user_id = body.value("user_id", "default-user");
            std::string name = body.value("name", "");
            std::string speaker_system_id = body.value("speaker_system_id", "");
            if (name.empty()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing preset name"}}.dump(), "application/json");
                return;
            }
            json bands = body.value("bands", json::object());
            std::string id = body.value("id", "eq_" + name);
            if (db::DatabaseManager::getInstance().save_eq_preset(id, user_id, name, speaker_system_id, bands.dump())) {
                res.set_content(json{{"status", "saved"}, {"id", id}}.dump(), "application/json");
            } else {
                res.status = 500;
                res.set_content(json{{"error", "Failed to save preset"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // EQ presets — delete
    svr_.Post("/eq-presets/delete", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = req.body.empty() ? json::object() : json::parse(req.body);
            std::string id = body.value("id", "");
            if (id.empty()) {
                res.status = 400;
                res.set_content(json{{"error", "Missing preset id"}}.dump(), "application/json");
                return;
            }
            if (db::DatabaseManager::getInstance().delete_eq_preset(id)) {
                res.set_content(json{{"status", "deleted"}}.dump(), "application/json");
            } else {
                res.status = 404;
                res.set_content(json{{"error", "Preset not found"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        }
    });

    // Devices — detect (stub; platform-specific implementation pending)
    svr_.Get("/devices/detect", [](const httplib::Request& req, httplib::Response& res) {
        json arr = json::array();
        arr.push_back({{"id", "default"}, {"name", "Default Output"}, {"type", "output"}});
        res.set_content(arr.dump(), "application/json");
    });
}

void HttpServer::start() {
    std::cout << "Starting C++ Native Server on 0.0.0.0:" << port_ << "\n";
    svr_.listen("0.0.0.0", port_);
}

void HttpServer::stop() {
    svr_.stop();
}

} // namespace api
