#include "include/log_state_machine.h"
#include "include/logger.h" // 包含 logger.h 以使用日志记录器
#include "include/constant.h"
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace nuraft;

log_state_machine::log_state_machine(VectorEngine* vector_engine) {
    this->vector_engine_ = vector_engine;
    this->last_committed_idx_ = vector_engine->getStartIndexID();
}

ptr<buffer> log_state_machine::commit(const ulong log_idx, buffer& data) {
    std::string content(reinterpret_cast<const char*>(data.data() + data.pos()+sizeof(int)), data.size()-sizeof(int));
    GlobalLogger->debug("Commit log_idx: {}", log_idx); // 添加打印日志
    
    rapidjson::Document json_request;
    json_request.Parse(content.c_str());
    std::string operation_type = json_request[REQUEST_OPERATION].GetString();
    if (operation_type == "insert") {
        vector_engine_->insert(json_request);
    } else if (operation_type == "insert_batch") {
        vector_engine_->insert_batch(json_request);
    }

    // Return Raft log number as a return result.
    ptr<buffer> ret = buffer::alloc( sizeof(log_idx) );
    buffer_serializer bs(ret);
    bs.put_u64(log_idx);
    return ret;
}

ptr<buffer> log_state_machine::pre_commit(const ulong log_idx, buffer& data) {
    std::string content(reinterpret_cast<const char*>(data.data() + data.pos()+sizeof(int)), data.size()-sizeof(int));
    GlobalLogger->debug("Pre Commit log_idx: {}", log_idx); // 添加打印日志
    return nullptr;
}