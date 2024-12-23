#include "include/master_server.h"
#include "include/logger.h"

int main() {
    std::string etcdEndpoints = "http://127.0.0.1:2379";
    int httpPort = 6060;

    init_global_logger();
    set_log_level(spdlog::level::debug);

    GlobalLogger->info("Global logger initialized");

    try {
        MasterServer masterServer(etcdEndpoints, httpPort);
        GlobalLogger->info("Starting Master Server on port {}", httpPort);
        GlobalLogger->info("Master Server created");
        masterServer.run();
    } catch (const std::exception& e) {
        GlobalLogger->error("Exception occurred in MasterServer: {}", e.what());
        return -1;
    }
}