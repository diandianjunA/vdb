#include "include/vector_storage.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "constant.h"

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

void VectorStorage::insert(long id, const rapidjson::Document& data) {
    const rapidjson::Value& object = data[REQUEST_OBJECT];
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    object.Accept(writer);
    std::string value = buffer.GetString();
    db_->Put(rocksdb::WriteOptions(), std::to_string(id), value);
}
    
    
rapidjson::Document VectorStorage::query(long id) {
    std::string value;
    db_->Get(rocksdb::ReadOptions(), std::to_string(id), &value);
    rapidjson::Document data;
    data.Parse(value.c_str());
    return data;
}

void VectorStorage::insert_batch(std::vector<long> ids, const rapidjson::Document& data) {
    const rapidjson::Value& objects = data[REQUEST_OBJECTS];
    if (!objects.IsArray()) {
        throw std::runtime_error("objects type not match");
    }
    
    for (int i = 0; i < ids.size(); i++) {
        const rapidjson::Value& row = objects[i];
        long id = ids[i];
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        row.Accept(writer);
        std::string value = buffer.GetString();
        db_->Put(rocksdb::WriteOptions(), std::to_string(id), value);
    }
}
