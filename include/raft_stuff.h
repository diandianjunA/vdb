#pragma once

#include "in_memory_state_mgr.h"
#include "log_state_machine.h"
#include <libnuraft/asio_service.hxx>
#include "logger.h" // 包含 logger.h 以使用日志记录器

class RaftStuff {
public:
    RaftStuff(int node_id, const std::string& endpoint, int port, VectorEngine* vector_engine);

    void Init();
    ptr<cmd_result<ptr<buffer>>> addSrv(int srv_id, const std::string& srv_endpoint);
    void enableElectionTimeout(int lower_bound, int upper_bound); // 定义 enableElectionTimeout 方法
    bool isLeader() const; // 添加 isLeader 方法声明
    std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>> getAllNodesInfo() const;
    std::string getNodeStatus(int node_id) const; // 添加 getNodeStatus 方法声明
    ptr<cmd_result<ptr<buffer>>> appendEntries(const std::string& entry);

private:
    int node_id;
    std::string endpoint;
    ptr<state_mgr> smgr_;
    ptr<state_machine> sm_;
    int port_;
    raft_launcher launcher_;
    ptr<raft_server> raft_instance_;
    VectorEngine* vector_engine_;
};