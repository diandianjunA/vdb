#pragma once

#include <string>
#include <vector>
#include <rocksdb/db.h>
#include "rapidjson/document.h"

class VectorStorage {
public:
    VectorStorage(const std::string& db_path);
    ~VectorStorage();

    void insert(uint64_t id, const rapidjson::Document& data);
    void insert_batch(std::vector<uint64_t> ids, const rapidjson::Document& data);
    rapidjson::Document query(uint64_t id);

private:
    rocksdb::DB* db_;
};