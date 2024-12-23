#include "include/proxy_http_server.h"
#include "include/logger.h"

int main() {
    init_global_logger();
    set_log_level(spdlog::level::debug);

    std::string master_host = "127.0.0.1"; // Master Server 地址
    int master_port = 6060;                // Master Server 端口
    std::string instance_id = "instance1"; // 代理服务器所属的实例 ID
    int proxy_port = 6061;                   // 代理服务器监听端口

    GlobalLogger->info("Starting ProxyServer...");
    ProxyHttpServer proxy(master_host, master_port, instance_id);
    GlobalLogger->info("Starting Proxy Server on port {}", proxy_port);
    GlobalLogger->info("Proxy server created");
    proxy.start(proxy_port);

    return 0;
}
