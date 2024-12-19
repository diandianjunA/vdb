#include "include/logger.h"
#include "include/index_factory.h"
#include "include/http_server.h"
#include "include/vector_index.h"
#include "include/vector_storage.h"
#include "include/vector_engine.h"
#include <unistd.h>

std::map<std::string, std::string> readConfigFile(const std::string& filename);

bool directoryExists(const std::string& path);

bool createDirectory(const std::string& path);

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
    set_log_level(spdlog::level::debug);

    GlobalLogger->info("Global logger initialized");

    std::string base_path = config["base_path"];
    if (!createDirectory(base_path)) {
        throw std::runtime_error("Failed to create directory: " + base_path);
    }

    // 初始化全局IndexFactory
    int dim = std::stoi(config["dim"]); // 向量维度
    int max_elements = std::stoi(config["max_elements"]); // 数据量
    IndexFactory* globalIndexFactory = getGlobalIndexFactory();
    globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
    globalIndexFactory->init(IndexFactory::IndexType::HNSW, dim, max_elements);
    globalIndexFactory->init(IndexFactory::IndexType::HNSWFLAT, dim);


    std::string db_path = base_path + "/db";
    std::string wal_path = base_path + "/wal";
    int node_id = std::stoi(config["node_id"]);
    std::string endpoint = config["endpoint"];
    int port = std::stoi(config["port"]);

    VectorIndex vector_index;
    VectorStorage vector_storage(db_path);
    WalManager wal_manager;

    VectorEngine vector_engine(db_path, wal_path, &vector_index, &vector_storage, &wal_manager);
    vector_engine.reloadDatabase();

    RaftStuff raft_stuff(node_id, endpoint, port, &vector_engine);

    // 创建并启动HTTP服务器
    std::string http_server_address = config["http_server_address"];
    int http_server_port = std::stoi(config["http_server_port"]);
    HttpServer server(http_server_address, http_server_port, &vector_engine, &raft_stuff);
    GlobalLogger->info("HttpServer created");
    server.start();

    return 0;
}

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

bool directoryExists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

bool createDirectory(const std::string& path) {
    if (directoryExists(path)) {
        return true;
    }

    // Recursively create parent directories
    size_t pos = path.find_last_of("/\\"); // Handle both Windows (\\) and Unix (/) separators
    if (pos != std::string::npos) {
        std::string parent = path.substr(0, pos);
        if (!createDirectory(parent)) {
            return false;
        }
    }

    // Create the current directory
    return mkdir(path.c_str(), 0755) == 0 || directoryExists(path);
}