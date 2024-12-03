#include <cstdint>
#include <fstream>
#include "rapidjson/document.h"

class WalManager {
public:
    WalManager() : increaseID_(0) {}
    ~WalManager();
    void init(const std::string& local_path);
    uint64_t increaseID();
    uint64_t getID() const;
    void writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data, const std::string& version);
    void readNextWalLog(std::string* operation_type, rapidjson::Document* json_data);

private:
    uint64_t increaseID_;
    std::fstream wal_log_file_;
}