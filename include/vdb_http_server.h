#pragma once

#include "httplib.h"
#include "index_factory.h"
#include "vector_engine.h"
#include "rapidjson/document.h"
#include <string>
#include "constant.h"
#include "raft_stuff.h"

class VdbHttpServer {
public:
    enum class CheckType {
        SEARCH,
        INSERT,
        QUERY,
        INSERT_BATCH,
        ADD_FOLLOWER,
        SNAPSHOT,
        SET_LEADER,
        LIST_NODE
    };

    VdbHttpServer(const std::string& host, int port, VectorEngine* vector_engine, RaftStuff* raft_stuff);
    void start();

private:
    void searchHandler(const httplib::Request& req, httplib::Response& res);
    void insertHandler(const httplib::Request& req, httplib::Response& res);
    void queryHandler(const httplib::Request& req, httplib::Response& res);
    void insertBatchHandler(const httplib::Request& req, httplib::Response& res);
    void snapshotHandler(const httplib::Request& req, httplib::Response& res);
    void addFollowerHandler(const httplib::Request& req, httplib::Response& res);
    void listNodeHandler(const httplib::Request& req, httplib::Response& res);
    void setJsonResponse(const rapidjson::Document& json_response, httplib::Response& res);
    void setErrorJsonResponse(httplib::Response&res, int error_code, const std::string& errorMsg);
    bool isRequestValid(const rapidjson::Document& json_request, CheckType check_type);

    httplib::Server server;
    std::string host;
    int port;
    VectorEngine* vector_engine_;
    RaftStuff* raft_stuff_;
};