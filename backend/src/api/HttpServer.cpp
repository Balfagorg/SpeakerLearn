#include "HttpServer.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include "../db/DatabaseManager.h"

using json = nlohmann::json;

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
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
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

    // Audio optimize stub
    svr_.Post("/audio/optimize", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            // In a full implementation, we extract samples, preferences, platform from JSON.
            // For now, call pipeline with dummy data to satisfy linking and verify the flow.
            std::vector<double> dummy_samples = {0.1, -0.1, 0.2, -0.2};
            std::map<std::string, double> dummy_prefs = {{"bass_boost", 0.5}};
            
            auto pipe_res = pipeline_.run(dummy_samples, dummy_prefs, std::nullopt, "spotify");
            
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

    // Speaker systems stub
    svr_.Post("/speaker-systems/create", [](const httplib::Request& req, httplib::Response& res) {
        // Just return a success json for now
        json response = {{"status", "created"}, {"id", "dummy-id-123"}};
        res.set_content(response.dump(), "application/json");
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
