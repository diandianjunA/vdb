#include "include/vector_storage.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

VectorStorage::VectorStorage(const std::string& db_path) {
    rocksdb::DB* db;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db);
    if (!status.ok()) {
        throw std::runtime_error("rocksdb open error");
    }
    db_ = db;
}

VectorStorage::~VectorStorage() {
    delete db_;
}

void VectorStorage::insert(uint64_t id, const rapidjson::Document& data) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);
    std::string value = buffer.GetString();
    db_->Put(rocksdb::WriteOptions(), std::to_string(id), value);
}
    
    
rapidjson::Document VectorStorage::query(uint64_t id) {
    std::string value;
    db_->Get(rocksdb::ReadOptions(), std::to_string(id), &value);
    rapidjson::Document data;
    data.Parse(value.c_str());
    return data;
}