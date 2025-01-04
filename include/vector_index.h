#pragma once

#include "index_factory.h"
#include <string>
#include <vector>
#include "rapidjson/document.h"
#include <fstream>

class VectorIndex {
public:
    VectorIndex(void* index, IndexFactory::IndexType type): increaseID_(0), index(index), type(type) {};
    ~VectorIndex();

    std::pair<std::vector<long>, std::vector<float>> search(const std::vector<float>& data, int k);
    void insert(const std::vector<float>& data, uint64_t id);
    void insert_batch(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids);

    void saveIndex(const std::string& folder_path);
    void loadIndex(const std::string& folder_path);

    void wal_init(const std::string& local_path);
    uint64_t increaseID();
    uint64_t getID() const;
    void writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data, const std::string& version);
    void writeWALRawLog(uint64_t log_id, const std::string& operation_type, const std::string& raw_data, const std::string& version);
    void readNextWalLog(std::string* operation_type, rapidjson::Document* json_data);

    void takeSnapshot(); 
    void loadSnapshot();
    void saveLastSnapshotID();
    void loadLastSnapshotID();

    IndexFactory::IndexType type;

private:
    void* index;

    uint64_t increaseID_;
    std::fstream wal_log_file_;
    uint64_t lastSnapshotID_;
};
