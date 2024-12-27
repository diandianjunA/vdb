#include "include/vector_index.h"
#include "include/index_factory.h"
#include "include/flat_index.h"
#include "include/hnsw_flat_index.h"
#include "include/constant.h"
#include "include/logger.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <sstream>
#include <filesystem>

VectorIndex::~VectorIndex() {
    if (wal_log_file_.is_open()) {
        wal_log_file_.close();
    }
}

void VectorIndex::wal_init(const std::string& local_path) {
    wal_log_file_.open(local_path, std::ios::app | std::ios::in | std::ios::out);
    if (!wal_log_file_.is_open()) {
        GlobalLogger->error("Can not open wal log file: {}", std::strerror(errno)); // 使用日志打印错误消息和原因
        throw std::runtime_error("Failed to open WAL log file at path: " + local_path);
    }
}

std::pair<std::vector<long>, std::vector<float>> VectorIndex::search(const std::vector<float>& data, int k) {
    // 根据索引类型初始化索引对象并调用 search_vectors 函数
    std::pair<std::vector<long>, std::vector<float>> results;
    switch (type) {
        case IndexFactory::IndexType::FLAT: {
            FlatIndex* flat_index = static_cast<FlatIndex*>(index);
            results = flat_index->search_vectors(data, k);
            break;
        }
        case IndexFactory::IndexType::HNSWFLAT: {
            HnswFlatIndex* hnsw_flat_index = static_cast<HnswFlatIndex*>(index);
            results = hnsw_flat_index->search_vectors(data, k);
        }
        default:
            break;
    }
    return results;
}

void VectorIndex::insert(const std::vector<float>& data, uint64_t id) {
    switch (type) {
        case IndexFactory::IndexType::FLAT: {
            FlatIndex* flat_index = static_cast<FlatIndex*>(index);
            flat_index->insert_vectors(data, id);
            break;
        }
        case IndexFactory::IndexType::HNSWFLAT: {
            HnswFlatIndex* hnsw_flat_index = static_cast<HnswFlatIndex*>(index);
            hnsw_flat_index->insert_vectors(data, id);
        }
        default:
            break;
    }
}

void VectorIndex::insert_batch(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids) {
    switch (type) {
        case IndexFactory::IndexType::FLAT: {
            FlatIndex* flat_index = static_cast<FlatIndex*>(index);
            flat_index->insert_batch_vectors(vectors, ids);
            break;
        }
        case IndexFactory::IndexType::HNSWFLAT: {
            HnswFlatIndex* hnsw_flat_index = static_cast<HnswFlatIndex*>(index);
            hnsw_flat_index->insert_batch_vectors(vectors, ids);
        }
        default:
            break;
    }
}

void VectorIndex::saveIndex(const std::string& folder_path) {
    std::string file_path = folder_path + std::to_string(static_cast<int>(type)) + ".index";

    // 根据索引类型调用相应的 saveIndex 函数
    if (type == IndexFactory::IndexType::FLAT) {
        static_cast<FlatIndex*>(index)->saveIndex(file_path);
    }else if (type == IndexFactory::IndexType::HNSWFLAT) {
        static_cast<HnswFlatIndex*>(index)->saveIndex(file_path);
    }
}

void VectorIndex::loadIndex(const std::string& folder_path) {
    std::string file_path = folder_path + std::to_string(static_cast<int>(type)) + ".index";

    if (!std::filesystem::exists(file_path)) {
        return;
    }

    // 根据索引类型调用相应的 loadIndex 函数
    if (type == IndexFactory::IndexType::FLAT) {
        static_cast<FlatIndex*>(index)->loadIndex(file_path);
    } else if (type == IndexFactory::IndexType::HNSWFLAT) {
        static_cast<HnswFlatIndex*>(index)->loadIndex(file_path);
    }
}

uint64_t VectorIndex::increaseID() {
    increaseID_++;
    return increaseID_;
}

uint64_t VectorIndex::getID() const {
    return increaseID_;
}

void VectorIndex::writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data, const std::string& version) {
    uint64_t log_id = increaseID();

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json_data.Accept(writer);

    wal_log_file_ << log_id << "|" << version << "|" << operation_type << "|" << buffer.GetString() << std::endl; // 将 version 添加到日志格式中

    if (wal_log_file_.fail()) { // 检查是否发生错误
        GlobalLogger->error("An error occurred while writing the WAL log entry. Reason: {}", std::strerror(errno)); // 使用日志打印错误消息和原因
    } else {
    //    GlobalLogger->debug("Wrote WAL log entry: log_id={}, version={}, operation_type={}, json_data_str={}", log_id, version, operation_type, buffer.GetString()); // 打印日志
       wal_log_file_.flush(); // 强制持久化
    }
}

void VectorIndex::writeWALRawLog(uint64_t log_id, const std::string& operation_type, const std::string& raw_data, const std::string& version) {
    wal_log_file_ << log_id << "|" << version << "|" << operation_type << "|" << raw_data << std::endl; // 将 version 添加到日志格式中

    if (wal_log_file_.fail()) { // 检查是否发生错误
        GlobalLogger->error("An error occurred while writing the WAL raw log entry. Reason: {}", std::strerror(errno)); // 使用日志打印错误消息和原因
    } else {
       GlobalLogger->debug("Wrote WAL raw log entry: log_id={}, version={}, operation_type={}, raw_data={}", log_id, version, operation_type, raw_data); // 打印日志
       wal_log_file_.flush(); // 强制持久化
    }
}

void VectorIndex::readNextWalLog(std::string* operation_type, rapidjson::Document* json_data) {
    GlobalLogger->debug("Reading next WAL log entry");

    std::string line;
    if (std::getline(wal_log_file_, line)) {
        std::istringstream iss(line);
        std::string log_id_str, version, json_data_str;

        std::getline(iss, log_id_str, '|');
        std::getline(iss, version, '|');
        std::getline(iss, *operation_type, '|'); // 使用指针参数返回 operation_type
        std::getline(iss, json_data_str, '|');

        uint64_t log_id = std::stoull(log_id_str); // 将 log_id_str 转换为 uint64_t 类型
        if (log_id > increaseID_) { // 如果 log_id 大于当前 increaseID_
            increaseID_ = log_id; // 更新 increaseID_
        }

        json_data->Parse(json_data_str.c_str()); // 使用指针参数返回 json_data

        // GlobalLogger->debug("Read WAL log entry: log_id={}, operation_type={}, json_data_str={}", log_id_str, *operation_type, json_data_str);
    } else {
        wal_log_file_.clear();
        GlobalLogger->debug("No more WAL log entries to read");
    }
}

void VectorIndex::takeSnapshot() {
    GlobalLogger->debug("Taking snapshot");

    lastSnapshotID_ =  increaseID_;
    std::string snapshot_folder_path = "snapshots_";
    saveIndex(snapshot_folder_path);

    saveLastSnapshotID();
}

void VectorIndex::loadSnapshot() {
    GlobalLogger->debug("Loading snapshot");
    loadIndex("snapshots_");
}

void VectorIndex::saveLastSnapshotID() {
    std::ofstream file("snapshots_MaxLogID");
    if (file.is_open()) {
        file << lastSnapshotID_;
        file.close();
    } else {
        GlobalLogger->error("Failed to open file snapshots_MaxID for writing");
    }
    GlobalLogger->debug("save snapshot Max log ID {}", lastSnapshotID_);
}

void VectorIndex::loadLastSnapshotID() {
    std::ifstream file("snapshots_MaxLogID");
    if (file.is_open()) {
        file >> lastSnapshotID_;
        file.close();
    } else {
        GlobalLogger->warn("Failed to open file snapshots_MaxID for reading");
    }
    GlobalLogger->debug("Loading snapshot Max log ID {}", lastSnapshotID_);
}
