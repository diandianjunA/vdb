#include "include/raft_stuff.h"

RaftStuff::RaftStuff(int node_id, const std::string& endpoint, int port, VectorEngine* vector_engine) : node_id(node_id), endpoint(endpoint), port_(port), raft_logger_(nullptr), vector_engine_(vector_engine) {
    Init();
}

void RaftStuff::Init() {
    smgr_ = cs_new<inmem_state_mgr>(node_id, endpoint, vector_engine_);
    sm_ = cs_new<log_state_machine>(vector_engine_);

    asio_service::options asio_opt;
    // asio_opt.thread_pool_size_ = 4;

    raft_params params;
    params.heart_beat_interval_ = 100;
    params.election_timeout_lower_bound_ = 200;
    params.election_timeout_upper_bound_ = 400;
    // // Upto 5 logs will be preserved ahead the last snapshot.
    // params.reserved_log_items_ = 5;
    // // Snapshot will be created for every 5 log appends.
    // params.snapshot_distance_ = 5;
    // // Client timeout: 3000 ms.
    // params.client_req_timeout_ = 3000;
    // // According to this method, `append_log` function
    // // should be handled differently.
    // params.return_method_ = raft_params::blocking;

    raft_instance_ = launcher_.init(sm_, smgr_, raft_logger_, port_, asio_opt, params);
    
    // Wait until Raft server is ready (upto 5 seconds).
    const size_t MAX_TRY = 20;
    for (size_t ii=0; ii<MAX_TRY; ++ii) {
        if (raft_instance_->is_initialized()) {
            GlobalLogger->debug("RaftStuff initialized with node_id: {}, endpoint: {}, port: {}", node_id, endpoint, port_);
            return;
        }
        fflush(stdout);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    GlobalLogger->error("RaftStuff initialized failed");
}

ptr<cmd_result<ptr<buffer>>> RaftStuff::addSrv(int srv_id, const std::string& srv_endpoint) {
    srv_config srv_conf_to_add(srv_id, srv_endpoint);
    GlobalLogger->debug("Adding server with srv_id: {}, srv_endpoint: {}", srv_id, srv_endpoint);
    return raft_instance_->add_srv(srv_conf_to_add);
}

bool RaftStuff::isLeader() const {
    if (!raft_instance_) {
        return false;
    }
    return raft_instance_->is_leader(); // 调用 raft_instance_ 的 is_leader() 方法
}

std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>> RaftStuff::getAllNodesInfo() const {
    std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>> nodes_info;

    if (!raft_instance_) {
        return nodes_info;
    }

    // 获取配置信息
    auto config = raft_instance_->get_config();
    if (!config) {
        return nodes_info;
    }

    // 获取服务器列表
    auto servers = config->get_servers();
    // 遍历所有节点并将其添加到 nodes_info 中
    for (const auto& srv : servers) {
        if (srv) {
            // 获取节点状态
            std::string node_state;
            if (srv->get_id() == raft_instance_->get_id()) {
                node_state = raft_instance_->is_leader() ? "leader" : "follower";
            } else {
                node_state = "follower";
            }
            
            // 使用正确的类型
            nuraft::raft_server::peer_info node_info = raft_instance_->get_peer_info(srv->get_id());
            nuraft::ulong last_log_idx = node_info.last_log_idx_;
            nuraft::ulong last_succ_resp_us = node_info.last_succ_resp_us_;

            nodes_info.push_back(std::make_tuple(srv->get_id(), srv->get_endpoint(), node_state, last_log_idx, last_succ_resp_us));
        }
    }

    return nodes_info;
}

ptr<cmd_result<ptr<buffer>>> RaftStuff::appendEntries(const std::string& entry) {
    if (!raft_instance_ || !raft_instance_->is_leader()) {
        // 添加调试日志
        if (!raft_instance_) {
            throw std::runtime_error("Cannot append entries: Raft instance is not available");
        } else {
            throw std::runtime_error("Cannot append entries: Current node is not the leader");
        }
        return nullptr;
    }

    // 计算所需的内存大小
    size_t total_size = sizeof(int) + entry.size();

    // 添加调试日志
    GlobalLogger->debug("Total size of entry: {}", total_size);

    // 创建一个 Raft 日志条目
    ptr<buffer> log_entry_buffer = buffer::alloc(total_size);
    buffer_serializer bs_log(log_entry_buffer);

    bs_log.put_str(entry);

    // 添加调试日志
    GlobalLogger->debug("Created log_entry_buffer at address: {}", static_cast<const void*>(log_entry_buffer.get()));

    // 添加调试日志
    GlobalLogger->debug("Appending entry to Raft instance");

    // 将日志条目追加到 Raft 实例中
    return raft_instance_->append_entries({log_entry_buffer});
}