#include "include/vector_engine.h"
#include "include/constant.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "logger.h"
#include "http_server.h"

VectorEngine::VectorEngine(std::string db_path, std::string wal_path, VectorIndex* vector_index, VectorStorage* vector_storage, WalManager* wal_manager) :db_path(db_path), vector_index_(vector_index), vector_storage_(vector_storage), wal_manager(wal_manager) {
    wal_manager->init(wal_path);
}

VectorEngine::~VectorEngine() {
    delete vector_storage_;
}

std::pair<std::vector<long>, std::vector<float>> VectorEngine::search(const rapidjson::Document& json_request) {
    // 从 JSON 请求中获取查询参数
    std::vector<float> data;
    for (const auto& q : json_request[REQUEST_VECTOR].GetArray()) {
        data.push_back(q.GetFloat());
    }
    int k = json_request[REQUEST_K].GetInt();

    // 获取请求参数中的索引类型
    IndexFactory::IndexType indexType = IndexFactory::IndexType::UNKNOWN;
    if (json_request.HasMember(REQUEST_INDEX_TYPE) && json_request[REQUEST_INDEX_TYPE].IsString()) {
        std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
        if (index_type_str == INDEX_TYPE_FLAT) {
            indexType = IndexFactory::IndexType::FLAT;
        } else if (index_type_str == INDEX_TYPE_HNSW) {
            indexType = IndexFactory::IndexType::HNSW;
        } else if (index_type_str == INDEX_TYPE_HNSWFLAT) {
            indexType = IndexFactory::IndexType::HNSWFLAT;
        }
    }

    return vector_index_->search(indexType, data, k);
}
    
void VectorEngine::insert(const rapidjson::Document& json_request) {
    // 从 JSON 请求中获取查询参数
    std::vector<float> data;
    for (const auto& q : json_request[REQUEST_VECTOR].GetArray()) {
        data.push_back(q.GetFloat());
    }
    int id = json_request[REQUEST_ID].GetInt();

    // 获取请求参数中的索引类型
    IndexFactory::IndexType indexType = IndexFactory::IndexType::UNKNOWN;
    if (json_request.HasMember(REQUEST_INDEX_TYPE) && json_request[REQUEST_INDEX_TYPE].IsString()) {
        std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
        if (index_type_str == INDEX_TYPE_FLAT) {
            indexType = IndexFactory::IndexType::FLAT;
        } else if (index_type_str == INDEX_TYPE_HNSW) {
            indexType = IndexFactory::IndexType::HNSW;
        } else if (index_type_str == INDEX_TYPE_HNSWFLAT) {
            indexType = IndexFactory::IndexType::HNSWFLAT;
        }
    }

    vector_index_->insert(indexType, data, id);
    vector_storage_->insert(id, json_request);
}

rapidjson::Document VectorEngine::query(const rapidjson::Document& json_request) {
    int id = json_request[REQUEST_ID].GetInt();
    return vector_storage_->query(id);
}

void VectorEngine::insert_batch(const rapidjson::Document& json_request) {
    std::vector<std::vector<float>> vectors;

    const rapidjson::Value& t = json_request[REQUEST_VECTORS];
    for (rapidjson::SizeType i = 0; i < t.Size(); i++) {
        const rapidjson::Value& row = t[i];
        std::vector<float> vector;
        for (rapidjson::SizeType j = 0; j < row.Size(); j++) {
            vector.push_back(row[j].GetFloat());
        }
        vectors.push_back(vector);
    }

    std::vector<long> ids;
    for (const auto& q : json_request[REQUEST_IDS].GetArray()) {
        ids.push_back(q.GetInt());
    }

    if (vectors.size() != ids.size()) {
        throw std::runtime_error("data format error, vectors size can not match ids");
    }

    // 获取请求参数中的索引类型
    IndexFactory::IndexType indexType = IndexFactory::IndexType::UNKNOWN;
    if (json_request.HasMember(REQUEST_INDEX_TYPE) && json_request[REQUEST_INDEX_TYPE].IsString()) {
        std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
        if (index_type_str == INDEX_TYPE_FLAT) {
            indexType = IndexFactory::IndexType::FLAT;
        } else if (index_type_str == INDEX_TYPE_HNSW) {
            indexType = IndexFactory::IndexType::HNSW;
        } else if (index_type_str == INDEX_TYPE_HNSWFLAT) {
            indexType = IndexFactory::IndexType::HNSWFLAT;
        }
    }

    vector_index_->insert_batch(indexType, vectors, ids);
}

void VectorEngine::reloadDatabase() {
    GlobalLogger->info("Entering VectorDatabase::reloadDatabase()");

    wal_manager->loadSnapshot();
    std::string operation_type;
    rapidjson::Document json_data;
    wal_manager->readNextWalLog(&operation_type, &json_data);

    while (!operation_type.empty()) {
        GlobalLogger->info("Operation Type: {}", operation_type);

        // 打印读取的一行内容
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        json_data.Accept(writer);
        GlobalLogger->info("Read Line: {}", buffer.GetString());

        if (operation_type == "insert") {
            insert(json_data); // 调用 VectorDatabase::upsert 接口重建数据
        } else if (operation_type == "insert_batch") {
            insert_batch(json_data);
        }

        // 清空 json_data
        rapidjson::Document().Swap(json_data);

        // 读取下一条 WAL 日志
        operation_type.clear();
        wal_manager->readNextWalLog(&operation_type, &json_data);
    }
}

void VectorEngine::writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data) {
    std::string version = "1.0";
    wal_manager->writeWalLog(operation_type, json_data, version);
}

void VectorEngine::takeSnapshot() {
    wal_manager->takeSnapshot();
}

void VectorEngine::loadSnapshot() {
    wal_manager->takeSnapshot();
}
