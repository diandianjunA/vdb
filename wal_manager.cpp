#include "include/wal_manager.h"

WalManager::~WalManager() {

}

void WalManager::init(const std::string& local_path) {

}

uint64_t WalManager::increaseID() {

}

uint64_t WalManager::getID() const {

}

void WalManager::writeWalLog(const std::string& operation_type, const rapidjson::Document& json_data, const std::string& version) {

}

void WalManager::readNextWalLog(std::string* operation_type, rapidjson::Document* json_data) {
    
}