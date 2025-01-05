#include "include/proxy_http_server.h"
#include "include/logger.h"

std::map<std::string, std::string> readConfigFile(const std::string& filename) {
    std::ifstream file(filename);
    std::map<std::string, std::string> config;
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string key, value;
            std::getline(ss, key, '=');
            std::getline(ss, value);
            config[key] = value;
        }
        file.close();
    } else {
        GlobalLogger->error("Failed to open config file: {}", filename);
        throw std::runtime_error("Failed to open config file: " + filename);
    }
    return config;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    // 从命令行参数读取配置文件路径
    std::string config_file_path = argv[1];

    // 读取配置文件
    auto config = readConfigFile(config_file_path);

    init_global_logger();
    set_log_level(spdlog::level::warn);

    std::string master_host = config["master_host"]; // Master Server 地址
    int master_port = std::stoi(config["master_port"]); // Master Server 端口
    std::string instance_id = config["instance_id"]; // 代理服务器所属的实例 ID
    int proxy_port = std::stoi(config["proxy_port"]); // 代理服务器监听端口

    GlobalLogger->info("Starting ProxyServer...");
    ProxyHttpServer proxy(master_host, master_port, instance_id);
    GlobalLogger->info("Starting Proxy Server on port {}", proxy_port);
    proxy.start(proxy_port);

    return 0;
}
