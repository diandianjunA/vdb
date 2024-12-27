#pragma once

#include "vector_index.h"
#include "vector_storage.h"

enum class ServerType {
    VDB,
    INDEX,
    STORAGE
};

class VectorEngine {
public:
    VectorEngine(std::string db_path, std::string wal_path, VectorIndex* vector_index, VectorStorage* vector_storage, ServerType server_type);
    ~VectorEngine();

    std::pair<std::vector<long>, std::vector<float>> search(const rapidjson::Document& json_request);
    void insert(const rapidjson::Document& json_request);
    rapidjson::Document query(const rapidjson::Document& json_request);
    void insert_batch(const rapidjson::Document& json_request);

    void reloadDatabase();
    void writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data);
    void writeWALLogWithID(uint64_t log_id, const std::string& data);
    int64_t getStartIndexID() const;

    void takeSnapshot();
    void loadSnapshot();

private:
    std::string db_path;
    VectorIndex* vector_index_;
    VectorStorage* vector_storage_;
    ServerType server_type;
};

