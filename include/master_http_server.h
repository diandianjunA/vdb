#pragma once

#include "include/httplib.h"
#include <etcd/Client.hpp>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

enum class ServerRole { Master, Backup };

struct ServerInfo {
    std::string url;
    ServerRole role;
    rapidjson::Document toJson() const;
    static ServerInfo fromJson(const rapidjson::Document& value);
};

class MasterHttpServer {
public:
    explicit MasterHttpServer(const std::string& etcdEndpoints, int httpPort);
    ~MasterHttpServer();
    void run();

private:
    etcd::Client etcdClient_;
    httplib::Server httpServer_;
    int httpPort_;
    std::set<std::string> key_set;

    void setResponse(httplib::Response& res, int retCode, const std::string& msg, const rapidjson::Document* data = nullptr);
    void getNodeInfo(const httplib::Request& req, httplib::Response& res);
    void addNode(const httplib::Request& req, httplib::Response& res);
    void removeNode(const httplib::Request& req, httplib::Response& res);
    void getInstance(const httplib::Request& req, httplib::Response& res);

};