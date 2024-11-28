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
    int dim = 1; // 向量维度
    int num_data = 1000; // 数据量
    IndexFactory* globalIndexFactory = getGlobalIndexFactory();
    globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
    globalIndexFactory->init(IndexFactory::IndexType::HNSW, dim, num_data);

    VectorIndex vector_index;
    VectorStorage vector_storage("/tmp/vdb");

    VectorEngine vector_engine(&vector_index, &vector_storage);

    // 创建并启动HTTP服务器
    HttpServer server("localhost", 8080, &vector_engine);
    GlobalLogger->info("HttpServer created");
    server.start();

    return 0;
}