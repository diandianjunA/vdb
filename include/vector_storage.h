#pragma once

#include <string>
#include <vector>
#include <rocksdb/db.h>
#include "rapidjson/document.h"

class VectorStorage {
public:
    VectorStorage(const std::string& db_path);
    ~VectorStorage();

    void insert(long id, const rapidjson::Document& data);
    void insert_batch(std::vector<long> ids, const rapidjson::Document& data);
    rapidjson::Document query(long id);

private:
    rocksdb::DB* db_;
};