#include "include/logger.h"
#include "include/index_factory.h"
#include "include/http_server.h"
#include "include/vector_index.h"
#include "include/vector_storage.h"
#include "include/vector_engine.h"

int main() {
    init_global_logger();
    set_log_level(spdlog::level::debug);

    GlobalLogger->info("Global logger initialized");

    // 初始化全局IndexFactory
    int dim = 100; // 向量维度
    int max_elements = 1400000; // 数据量
    IndexFactory* globalIndexFactory = getGlobalIndexFactory();
    globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
    globalIndexFactory->init(IndexFactory::IndexType::HNSW, dim, max_elements);
    globalIndexFactory->init(IndexFactory::IndexType::HNSWFLAT, dim);

    std::string db_path = "/tmp/vdb";
    std::string wal_path = "/tmp/vdb/wal";

    VectorIndex vector_index;
    VectorStorage vector_storage(db_path);
    WalManager wal_manager;

    VectorEngine vector_engine(db_path, wal_path, &vector_index, &vector_storage, &wal_manager);
    vector_engine.reloadDatabase();

    // 创建并启动HTTP服务器
    HttpServer server("localhost", 8080, &vector_engine);
    GlobalLogger->info("HttpServer created");
    server.start();

    return 0;
}