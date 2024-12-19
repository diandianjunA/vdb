#include "include/wal_manager.h"
#include "logger.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <sstream>
#include "include/index_factory.h"

WalManager::~WalManager() {
    if (wal_log_file_.is_open()) {
        wal_log_file_.close();
    }
}

void WalManager::init(const std::string& local_path) {
    wal_log_file_.open(local_path, std::ios::app | std::ios::in | std::ios::out);
    if (!wal_log_file_.is_open()) {
        GlobalLogger->error("Can not open wal log file: {}", std::strerror(errno)); // 使用日志打印错误消息和原因
        throw std::runtime_error("Failed to open WAL log file at path: " + local_path);
    }
}

uint64_t WalManager::increaseID() {
    increaseID_++;
    return increaseID_;
}

uint64_t WalManager::getID() const {
    return increaseID_;
}

void WalManager::writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data, const std::string& version) {
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

void WalManager::writeWALRawLog(uint64_t log_id, const std::string& operation_type, const std::string& raw_data, const std::string& version) {
    wal_log_file_ << log_id << "|" << version << "|" << operation_type << "|" << raw_data << std::endl; // 将 version 添加到日志格式中

    if (wal_log_file_.fail()) { // 检查是否发生错误
        GlobalLogger->error("An error occurred while writing the WAL raw log entry. Reason: {}", std::strerror(errno)); // 使用日志打印错误消息和原因
    } else {
       GlobalLogger->debug("Wrote WAL raw log entry: log_id={}, version={}, operation_type={}, raw_data={}", log_id, version, operation_type, raw_data); // 打印日志
       wal_log_file_.flush(); // 强制持久化
    }
}

void WalManager::readNextWalLog(std::string* operation_type, rapidjson::Document* json_data) {
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

void WalManager::takeSnapshot() {
    GlobalLogger->debug("Taking snapshot");

    lastSnapshotID_ =  increaseID_;
    std::string snapshot_folder_path = "snapshots_";
    IndexFactory* index_factory = getGlobalIndexFactory();
    index_factory->saveIndex(snapshot_folder_path);

    saveLastSnapshotID();
}

void WalManager::loadSnapshot() {
    GlobalLogger->debug("Loading snapshot");
    IndexFactory* index_factory = getGlobalIndexFactory();
    index_factory->loadIndex("snapshots_");
}

void WalManager::saveLastSnapshotID() {
    std::ofstream file("snapshots_MaxLogID");
    if (file.is_open()) {
        file << lastSnapshotID_;
        file.close();
    } else {
        GlobalLogger->error("Failed to open file snapshots_MaxID for writing");
    }
    GlobalLogger->debug("save snapshot Max log ID {}", lastSnapshotID_);
}

void WalManager::loadLastSnapshotID() {
    std::ifstream file("snapshots_MaxLogID");
    if (file.is_open()) {
        file >> lastSnapshotID_;
        file.close();
    } else {
        GlobalLogger->warn("Failed to open file snapshots_MaxID for reading");
    }
    GlobalLogger->debug("Loading snapshot Max log ID {}", lastSnapshotID_);
}
