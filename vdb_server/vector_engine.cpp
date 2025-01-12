#include "include/vector_engine.h"
#include "include/constant.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "logger.h"
#include "vdb_http_server.h"

VectorEngine::VectorEngine(std::string db_path, std::string wal_path, VectorIndex* vector_index, VectorStorage* vector_storage, ServerType server_type) :db_path(db_path), vector_index_(vector_index), vector_storage_(vector_storage), server_type(server_type) {
    if (vector_index_ != nullptr) {
        vector_index_->wal_init(wal_path);
    }
}

VectorEngine::~VectorEngine() {
    delete vector_storage_;
}

std::pair<std::vector<long>, std::vector<float>> VectorEngine::search(const rapidjson::Document& json_request) {
    if (server_type == ServerType::STORAGE) {
        throw std::runtime_error("This is storage node, cannot handle search!");
    }
    // 从 JSON 请求中获取查询参数
    std::vector<float> data;
    for (const auto& q : json_request[REQUEST_VECTOR].GetArray()) {
        data.push_back(q.GetFloat());
    }
    int k = json_request[REQUEST_K].GetInt();

    auto start = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto res = vector_index_->search(data, k);
    auto end = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    GlobalLogger->debug("开始查询的时间:{}, 结束查询的时间:{}", start, end);
    return res;
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

    auto start = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if (server_type == ServerType::INDEX || server_type == ServerType::VDB) {
        vector_index_->insert(data, id);
    }
    if (server_type == ServerType::STORAGE || server_type == ServerType::VDB) {
        vector_storage_->insert(id, json_request);
    }
    auto end = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    GlobalLogger->debug("开始插入的时间:{}, 结束插入的时间:{}", start, end);
}

rapidjson::Document VectorEngine::query(const rapidjson::Document& json_request) {
    if (server_type == ServerType::INDEX) {
        throw std::runtime_error("This is index node, cannot handle query!");
    }
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

    if (server_type == ServerType::INDEX || server_type == ServerType::VDB) {
        vector_index_->insert_batch(vectors, ids);
    }
    if (server_type == ServerType::STORAGE || server_type == ServerType::VDB) {
        vector_storage_->insert_batch(ids, json_request);
    }
}

void VectorEngine::reloadDatabase() {
    if (server_type == ServerType::STORAGE) {
        return;
    }
    // if (vector_index_->type == IndexFactory::IndexType::CAGRA) {
    //     return;
    // }

    vector_index_->loadSnapshot();
    std::string operation_type;
    rapidjson::Document json_data;
    vector_index_->readNextWalLog(&operation_type, &json_data);

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
        vector_index_->readNextWalLog(&operation_type, &json_data);
    }
}

void VectorEngine::writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data) {
    if (server_type == ServerType::STORAGE) {
        return;
    }
    std::string version = "1.0";
    vector_index_->writeWalLog(operation_type, json_data, version);
}

void VectorEngine::writeWALLogWithID(uint64_t log_id, const std::string& data) {
    if (server_type == ServerType::STORAGE) {
        return;
    }
    rapidjson::Document json_data;
    json_data.Parse(data.c_str());
    std::string operation_type = json_data[REQUEST_OPERATION].GetString();
    std::string version = "1.0";
    vector_index_->writeWALRawLog(log_id, operation_type, data, version);
}

void VectorEngine::takeSnapshot() {
    if (server_type == ServerType::STORAGE) {
        throw std::runtime_error("This is storage node, cannot taking snapshot!");
    }
    vector_index_->takeSnapshot();
}

void VectorEngine::loadSnapshot() {
    if (server_type == ServerType::STORAGE) {
        throw std::runtime_error("This is storage node, cannot loading snapshot!");
    }
    vector_index_->takeSnapshot();
}

int64_t VectorEngine::getStartIndexID() const {
    if (server_type == ServerType::STORAGE) {
        return 1;
    }
    return vector_index_->getID();
}