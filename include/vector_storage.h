#pragma once

#include <string>
#include <rocksdb/db.h>
#include "rapidjson/document.h"

class VectorStorage {
public:
    VectorStorage(const std::string& db_path);
    ~VectorStorage();

    void insert(uint64_t id, const rapidjson::Document& data);
    rapidjson::Document query(uint64_t id);

private:
    rocksdb::DB* db_;
};