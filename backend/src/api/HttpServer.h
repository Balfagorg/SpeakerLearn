#pragma once
#include <httplib.h>
#include <memory>
#include "../core/AudioPipeline.h"

namespace api {

class HttpServer {
public:
    HttpServer(int port = 8000);
    ~HttpServer();

    void start();
    void stop();

private:
    void setup_routes();
    httplib::Server svr_;
    int port_;
    core::AudioPipeline pipeline_;
};

} // namespace api
