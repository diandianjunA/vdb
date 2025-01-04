#include "include/logger.h"
#include "include/index_factory.h"
#include "include/vdb_http_server.h"
#include "include/vector_index.h"
#include "include/vector_storage.h"
#include "include/vector_engine.h"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

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

void reset_directory(const fs::path& dir_path) {
    try {
        // 如果目标文件夹已存在
        if (fs::exists(dir_path)) {
            if (!fs::is_directory(dir_path)) {
                throw std::runtime_error(dir_path.string() + " 不是一个文件夹");
            }
            // 清空文件夹内容
            fs::remove_all(dir_path);
        }
        // 重新创建空文件夹
        fs::create_directories(dir_path);
    } catch (const std::exception& e) {
        std::cerr << "操作失败: " << e.what() << std::endl;
    }
}

void create_directory(const fs::path& dir_path) {
    try {
        // 如果目标文件夹已存在
        if (!fs::exists(dir_path)) {
            fs::create_directories(dir_path);
        }
    } catch (const std::exception& e) {
        std::cerr << "操作失败: " << e.what() << std::endl;
    }
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
    set_log_level(spdlog::level::debug);

    GlobalLogger->info("Global logger initialized");

    std::string base_path = config["base_path"];
    // create_directory(base_path);
    reset_directory(base_path);

    ServerType server_type;

    std::string server_type_str = config["server_type"];
    if (server_type_str == "vdb") {
        server_type = ServerType::VDB;
    } else if (server_type_str == "index") {
        server_type = ServerType::INDEX;
    } else if (server_type_str == "storage") {
        server_type = ServerType::STORAGE;
    } else {
        throw std::runtime_error("server_type is illegal");
        exit(1);
    }

    // 初始化全局IndexFactory
    int dim = std::stoi(config["dim"]); // 向量维度
    std::string db_path = base_path + "/db";
    std::string wal_path = base_path + "/wal";
    int node_id = std::stoi(config["node_id"]);
    std::string endpoint = config["endpoint"];
    int port = std::stoi(config["port"]);

    VectorIndex* vector_index;
    VectorStorage* vector_storage;

    if (server_type == ServerType::VDB || server_type == ServerType::INDEX) {
        IndexFactory* globalIndexFactory = getGlobalIndexFactory();
        std::string index_type = config["index_type"];
        if (index_type == "FLAT") {
            IndexFactory::IndexType type = IndexFactory::IndexType::FLAT;
            void* index = globalIndexFactory->init(type, dim);
            vector_index = new VectorIndex(index, type);
        } else if (index_type == "HNSWFLAT") {
            IndexFactory::IndexType type = IndexFactory::IndexType::HNSWFLAT;
            void* index = globalIndexFactory->init(type, dim);
            vector_index = new VectorIndex(index, type);
        } else if (index_type == "FLAT_GPU") {
            IndexFactory::IndexType type = IndexFactory::IndexType::FLAT_GPU;
            void* index = globalIndexFactory->init(type, dim);
            vector_index = new VectorIndex(index, type);
        } else if (index_type == "IVFPQ") {
            IndexFactory::IndexType type = IndexFactory::IndexType::IVFPQ;
            void* index = globalIndexFactory->init(type, dim);
            vector_index = new VectorIndex(index, type);
        } else if (index_type == "CAGRA") {
            IndexFactory::IndexType type = IndexFactory::IndexType::CAGRA;
            void* index = globalIndexFactory->init(type, dim);
            vector_index = new VectorIndex(index, type);
        }
    }
    if (server_type == ServerType::VDB || server_type == ServerType::STORAGE) {
        vector_storage = new VectorStorage(db_path);
    }

    VectorEngine vector_engine(db_path, wal_path, vector_index, vector_storage, server_type);
    vector_engine.reloadDatabase();
    RaftStuff raft_stuff(node_id, endpoint, port, &vector_engine);

    // 创建并启动HTTP服务器
    std::string http_server_address = config["http_server_address"];
    int http_server_port = std::stoi(config["http_server_port"]);
    VdbHttpServer server(http_server_address, http_server_port, &vector_engine, &raft_stuff);
    GlobalLogger->info("HttpServer created");
    server.start();

    delete vector_storage;
    return 0;
}