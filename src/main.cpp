#include <iostream>
#include "db/DatabaseManager.h"
#include "api/HttpServer.h"

int main(int argc, char** argv) {
    std::cout << "Initializing database...\n";
    if (!db::DatabaseManager::getInstance().init_db("adaptive_audio_cpp.db")) {
        std::cerr << "Failed to initialize SQLite database. Exiting.\n";
        return 1;
    }

    // Run on 8001 to avoid conflicting with the existing python process on 8000
    api::HttpServer server(8001); 
    server.start();

    return 0;
}
