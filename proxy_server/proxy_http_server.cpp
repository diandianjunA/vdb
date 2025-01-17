#include "include/proxy_http_server.h"
#include "include/logger.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <sstream>
#include <chrono>

ProxyHttpServer::ProxyHttpServer(const std::string& masterServerHost, int masterServerPort, const std::string& instanceId)
: masterServerHost_(masterServerHost), masterServerPort_(masterServerPort), instanceId_(instanceId), curlHandle_(nullptr), activeNodesIndex_(0) , nextNodeIndex_(0), running_(true) {
    initCurl();
    setupForwarding();
    startNodeUpdateTimer(); // 启动节点更新定时器
    follower_request = {"/search", "/query", "/listNode"};
    leader_request = {"/insert", "/insert_batch", "/snapshot", "/addFollower"};
    index_cannot = {"/query"};
    storage_cannot = {"/search", "/snapshot"};
}


ProxyHttpServer::~ProxyHttpServer() {
    running_ = false; // 停止定时器循环
    cleanupCurl();
}

void ProxyHttpServer::initCurl() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curlHandle_ = curl_easy_init();
    if (curlHandle_) {
        curl_easy_setopt(curlHandle_, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curlHandle_, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curlHandle_, CURLOPT_TCP_KEEPINTVL, 60L);
    }
}

void ProxyHttpServer::cleanupCurl() {
    if (curlHandle_) {
        curl_easy_cleanup(curlHandle_);
    }
    curl_global_cleanup();
}

void ProxyHttpServer::start(int port) {
    fetchAndUpdateNodes(); // 获取节点信息
    GlobalLogger->info("Proxy server created");
    httpServer_.listen("0.0.0.0", port);
}

void ProxyHttpServer::setupForwarding() {
    httpServer_.Post("/search", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding POST /search");
        forwardRequest(req, res, "/search");
    });
    httpServer_.Post("/insert", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding POST /insert");
        forwardRequest(req, res, "/insert");
    });
    httpServer_.Post("/insert_batch", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding POST /insert_batch");
        forwardRequest(req, res, "/insert_batch");
    });
    httpServer_.Post("/query", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding POST /query");
        forwardRequest(req, res, "/query");
    });
    httpServer_.Post("/snapshot", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding POST /snapshot");
        forwardRequest(req, res, "/snapshot");
    });
    httpServer_.Post("/addFollower", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding POST /addFollower");
        forwardRequest(req, res, "/addFollower");
    });
    httpServer_.Get("/listNode", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding GET /listNode");
        forwardRequest(req, res, "/listNode");
    });
    httpServer_.Get("/topology", [this](const httplib::Request&, httplib::Response& res) {
        this->handleTopologyRequest(res);
    });
}

void ProxyHttpServer::forwardRequest(const httplib::Request& req, httplib::Response& res, const std::string& path) {
    auto start = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int activeIndex = activeNodesIndex_.load(); // 读取当前活动的数组索引
    if (nodes_[activeIndex].empty()) {
        GlobalLogger->error("No available nodes for forwarding");
        res.status = 503;
        res.set_content("Service Unavailable", "text/plain");
        return;
    }

    size_t nodeIndex = nextNodeIndex_.load() % nodes_[activeIndex].size();;

    // 检查是否需要强制路由到主节点
    if (leader_request.find(path) != leader_request.end()) {
        if (index_cannot.find(path) != index_cannot.end()) {
            for (size_t i = nodeIndex; ; i = (i + 1) % nodes_[activeIndex].size()) {
                if (nodes_[activeIndex][i].role == 0 && nodes_[activeIndex][i].type != 1) {
                    nodeIndex = i;
                    break;
                }
            }
        } else if (storage_cannot.find(path) != storage_cannot.end()) {
            for (size_t i = nodeIndex; ; i = (i + 1) % nodes_[activeIndex].size()) {
                if (nodes_[activeIndex][i].role == 0 && nodes_[activeIndex][i].type != 2) {
                    nodeIndex = i;
                    break;
                }
            }
        } else {
            for (size_t i = nodeIndex; ; i = (i + 1) % nodes_[activeIndex].size()) {
                if (nodes_[activeIndex][i].role == 0) {
                    nodeIndex = i;
                    break;
                }
            }
        }
    } else {
        if (index_cannot.find(path) != index_cannot.end()) {
            for (size_t i = nodeIndex; ; i = (i + 1) % nodes_[activeIndex].size()) {
                if (nodes_[activeIndex][i].type != 1) {
                    nodeIndex = i;
                    break;
                }
            }
        } else if (storage_cannot.find(path) != storage_cannot.end()) {
            for (size_t i = nodeIndex; ; i = (i + 1) % nodes_[activeIndex].size()) {
                if (nodes_[activeIndex][i].type != 2) {
                    nodeIndex = i;
                    break;
                }
            }
        }
    }

    nextNodeIndex_.store(nodeIndex + 1);

    const auto& targetNode = nodes_[activeIndex][nodeIndex];
    std::string targetUrl = targetNode.url + path;
    GlobalLogger->info("Forwarding request to: {}", targetUrl);

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3); //连接超时限制
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20); //整个CURL操作的超时限制

    // 设置 CURL 选项
    curl_easy_setopt(curl, CURLOPT_URL, targetUrl.c_str());
    if (req.method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req.body.c_str());
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

    // 响应数据容器
    std::string response_data;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // 执行 CURL 请求
    CURLcode curl_res = curl_easy_perform(curl);
    if (curl_res != CURLE_OK) {
        GlobalLogger->error("curl_easy_perform() failed: {}", curl_easy_strerror(curl_res));
        res.status = 500;
        res.set_content("Internal Server Error", "text/plain");
    } else {
        GlobalLogger->info("Received response from server");
        // 确保响应数据不为空
        if (response_data.empty()) {
            GlobalLogger->error("Received empty response from server");
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        } else {
            res.set_content(response_data, "application/json");
        }
    }
    auto end = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    curl_easy_cleanup(curl);
    GlobalLogger->debug("收到请求的时间:{}, 收到请求的时间:{}", start, end);
}

size_t ProxyHttpServer::writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void ProxyHttpServer::fetchAndUpdateNodes() {
    GlobalLogger->info("Fetching nodes from Master Server");

    // 构建请求 URL
    std::string url = "http://" + masterServerHost_ + ":" + std::to_string(masterServerPort_) + "/getInstance?instanceId=" + instanceId_;
    GlobalLogger->debug("Requesting URL: {}", url);

    // 设置 CURL 选项
    curl_easy_setopt(curlHandle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle_, CURLOPT_HTTPGET, 1L);
    std::string response_data;
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEDATA, &response_data);

    // 执行 CURL 请求
    CURLcode curl_res = curl_easy_perform(curlHandle_);
    if (curl_res != CURLE_OK) {
        GlobalLogger->error("curl_easy_perform() failed: {}", curl_easy_strerror(curl_res));
        return;
    }

    // 解析响应数据
    rapidjson::Document doc;
    if (doc.Parse(response_data.c_str()).HasParseError()) {
        GlobalLogger->error("Failed to parse JSON response");
        return;
    }

    // 检查返回码
    if (doc["retCode"].GetInt() != 0) {
        GlobalLogger->error("Error from Master Server: {}", doc["msg"].GetString());
        return;
    }

    // GlobalLogger->info(response_data);

    int inactiveIndex = activeNodesIndex_.load() ^ 1; // 获取非活动数组的索引
    nodes_[inactiveIndex].clear();
    const auto& nodesArray = doc["data"]["nodes"].GetArray();
    for (const auto& nodeVal : nodesArray) {
        NodeInfo node;
        node.nodeId = nodeVal["nodeId"].GetString();
        node.url = nodeVal["url"].GetString();
        node.role = nodeVal["role"].GetInt();
        node.type = nodeVal["type"].GetInt();
        nodes_[inactiveIndex].push_back(node);
    }

    // 原子地切换活动数组索引
    activeNodesIndex_.store(inactiveIndex);
    GlobalLogger->info("Nodes updated successfully");
}


void ProxyHttpServer::handleTopologyRequest(httplib::Response& res) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    // 添加 Master Server 信息
    doc.AddMember("masterServer", rapidjson::Value(masterServerHost_.c_str(), allocator), allocator);
    doc.AddMember("masterServerPort", masterServerPort_, allocator);

    // 添加 instanceId
    doc.AddMember("instanceId", rapidjson::Value(instanceId_.c_str(), allocator), allocator);

    // 添加节点信息
    rapidjson::Value nodesArray(rapidjson::kArrayType);
    int activeIndex = activeNodesIndex_.load();
    for (const auto& node : nodes_[activeIndex]) {
        rapidjson::Value nodeObj(rapidjson::kObjectType);
        nodeObj.AddMember("nodeId", rapidjson::Value(node.nodeId.c_str(), allocator), allocator);
        nodeObj.AddMember("url", rapidjson::Value(node.url.c_str(), allocator), allocator);
        nodeObj.AddMember("role", node.role, allocator);
        nodeObj.AddMember("type", node.type, allocator);
        nodesArray.PushBack(nodeObj, allocator);
    }
    doc.AddMember("nodes", nodesArray, allocator);

    // 转换 JSON 对象为字符串
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    // 设置响应
    res.set_content(buffer.GetString(), "application/json");
}


void ProxyHttpServer::startNodeUpdateTimer() {
    std::thread([this]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            fetchAndUpdateNodes();
        }
    }).detach();
}
