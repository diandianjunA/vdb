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
    const rapidjson::Value& object = json_request[REQUEST_OBJECT];
    if (object.HasMember(REQUEST_VECTOR) && object[REQUEST_VECTOR].IsArray() && object.HasMember(REQUEST_ID) && object[REQUEST_ID].IsInt()) {
        const rapidjson::Value& vector = object[REQUEST_VECTOR];
        for (const auto& q : vector.GetArray()) {
            data.push_back(q.GetFloat());
        }
    } else {
        throw std::runtime_error("Missing vectors or id parameter in the request");
    }
    int id = object[REQUEST_ID].GetInt();

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
    std::vector<long> ids;

    const rapidjson::Value& objects = json_request[REQUEST_OBJECTS];
    if (!objects.IsArray()) {
        throw std::runtime_error("objects type not match");
    }
    for (auto& obj : objects.GetArray()) {
        if (obj.HasMember(REQUEST_VECTOR) && obj[REQUEST_VECTOR].IsArray() && obj.HasMember(REQUEST_ID) && obj[REQUEST_ID].IsInt()) {
            const rapidjson::Value& row = obj[REQUEST_VECTOR];
            std::vector<float> vector;
            for (rapidjson::SizeType j = 0; j < row.Size(); j++) {
                vector.push_back(row[j].GetFloat());
            }
            vectors.push_back(vector);
            ids.push_back(obj[REQUEST_ID].GetInt());
        } else {
            throw std::runtime_error("Missing vectors or id parameter in the request");
        }
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
    vector_storage_->insert_batch(ids, json_request);
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
            insert(json_data);
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

void VectorEngine::writeWALLogWithID(uint64_t log_id, const std::string& data) {
    rapidjson::Document json_data;
    json_data.Parse(data.c_str());
    std::string operation_type = json_data[REQUEST_OPERATION].GetString();
    std::string version = "1.0";
    wal_manager->writeWALRawLog(log_id, operation_type, data, version);
}

void VectorEngine::takeSnapshot() {
    wal_manager->takeSnapshot();
}

void VectorEngine::loadSnapshot() {
    wal_manager->takeSnapshot();
}

int64_t VectorEngine::getStartIndexID() const {
    return wal_manager->getID();
}