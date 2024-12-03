#pragma once

#include "vector_index.h"
#include "vector_storage.h"
#include "wal_manager.h"

class VectorEngine {
public:
    VectorEngine(std::string db_path, VectorIndex* vector_index, VectorStorage* vector_storage);
    ~VectorEngine();

    std::pair<std::vector<long>, std::vector<float>> search(const rapidjson::Document& json_request);
    void insert(const rapidjson::Document& json_request);
    rapidjson::Document query(const rapidjson::Document& json_request);
    void insert_batch(const rapidjson::Document& json_request);

    void reloadDatabase();
    void writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data, const std::string& version);

    void takeSnapshot();
    void loadSnapshot();

private:
    std::string db_path;
    VectorIndex* vector_index_;
    VectorStorage* vector_storage_;
    WalManager* wal_manager;
};

