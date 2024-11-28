#include "include/vector_engine.h"
#include "include/constant.h"

VectorEngine::VectorEngine(VectorIndex* vector_index, VectorStorage* vector_storage) : vector_index_(vector_index), vector_storage_(vector_storage) {}

VectorEngine::~VectorEngine() {
    delete vector_storage_;
}

std::pair<std::vector<long>, std::vector<float>> VectorEngine::search(const rapidjson::Document& json_request) {
    // 从 JSON 请求中获取查询参数
    std::vector<float> data;
    for (const auto& q : json_request[REQUEST_VECTORS].GetArray()) {
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
        }
    }

    return vector_index_->search(indexType, data, k);
}
    
void VectorEngine::insert(const rapidjson::Document& json_request) {
    // 从 JSON 请求中获取查询参数
    std::vector<float> data;
    for (const auto& q : json_request[REQUEST_VECTORS].GetArray()) {
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
        }
    }

    vector_index_->insert(indexType, data, id);
    vector_storage_->insert(id, json_request);
}

rapidjson::Document VectorEngine::query(const rapidjson::Document& json_request) {
    int id = json_request[REQUEST_ID].GetInt();
    return vector_storage_->query(id);
}
