#pragma once

#include "httplib.h"
#include "index_factory.h"
#include "vector_engine.h"
#include "rapidjson/document.h"
#include <string>
#include "include/constant.h"

class HttpServer {
public:
    enum class CheckType {
        SEARCH,
        INSERT,
        QUERY,
        INSERT_BATCH
    };

    HttpServer(const std::string& host, int port, VectorEngine* vector_engine);
    void start();

private:
    void searchHandler(const httplib::Request& req, httplib::Response& res);
    void insertHandler(const httplib::Request& req, httplib::Response& res);
    void queryHandler(const httplib::Request& req, httplib::Response& res);
    void insertBatchHandler(const httplib::Request& req, httplib::Response& res);
    void setJsonResponse(const rapidjson::Document& json_response, httplib::Response& res);
    void setErrorJsonResponse(httplib::Response&res, int error_code, const std::string& errorMsg);
    bool isRequestValid(const rapidjson::Document& json_request, CheckType check_type);

    httplib::Server server;
    std::string host;
    int port;
    VectorEngine* vector_engine_;
};

IndexFactory::IndexType getIndexTypeFromRequest(const rapidjson::Document& json_request);