#include "include/http_server.h"
#include "include/logger.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

HttpServer::HttpServer(const std::string& host, int port, VectorEngine* vector_engine): host(host), port(port), vector_engine_(vector_engine) {
    server.Post("/search", [this](const httplib::Request& req, httplib::Response& res) {
        searchHandler(req, res);
    });
    server.Post("/insert", [this](const httplib::Request& req, httplib::Response& res) {
        insertHandler(req, res);
    });
    server.Post("/query", [this](const httplib::Request& req, httplib::Response& res) {
        queryHandler(req, res);
    });
    server.Post("/insert_batch", [this](const httplib::Request& req, httplib::Response& res) {
        insertBatchHandler(req, res);
    });
}

void HttpServer::start() {
    server.listen(host.c_str(), port);
}

bool HttpServer::isRequestValid(const rapidjson::Document& json_request, CheckType check_type) {
    switch(check_type) {
        case CheckType::SEARCH:
            return json_request.HasMember(REQUEST_VECTOR) && json_request.HasMember(REQUEST_K) && (!json_request.HasMember(REQUEST_INDEX_TYPE) || json_request[REQUEST_INDEX_TYPE].IsString());
        case CheckType::INSERT:
            return json_request.HasMember(REQUEST_OBJECT) && (!json_request.HasMember(REQUEST_INDEX_TYPE) || json_request[REQUEST_INDEX_TYPE].IsString());
        case CheckType::QUERY:
            return json_request.HasMember(REQUEST_ID);
        case CheckType::INSERT_BATCH:
            return json_request.HasMember(REQUEST_OBJECTS) && (!json_request.HasMember(REQUEST_INDEX_TYPE) || json_request[REQUEST_INDEX_TYPE].IsString());
        default:
            return false;
    }
}

void HttpServer::setJsonResponse(const rapidjson::Document& json_response, httplib::Response& res) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json_response.Accept(writer);
    res.set_content(buffer.GetString(), RESPONSE_CONTENT_TYPE_JSON);
}

void HttpServer::setErrorJsonResponse(httplib::Response&res, int error_code, const std::string& errorMsg) {
    rapidjson::Document json_response;
    json_response.SetObject();
    rapidjson::Document::AllocatorType& allocator = json_response.GetAllocator();
    json_response.AddMember(RESPONSE_RETCODE, error_code, allocator);
    json_response.AddMember(RESPONSE_ERROR_MSG, rapidjson::StringRef(errorMsg.c_str()), allocator);
    setJsonResponse(json_response, res);
}

void HttpServer::searchHandler(const httplib::Request& req, httplib::Response& res) {
    GlobalLogger->debug("Received search request");

    // 解析json请求
    rapidjson::Document json_request;
    json_request.Parse(req.body.c_str());

    // // 打印用户的输入参数
    // GlobalLogger->info("Search request parameters: {}", req.body);

    // 检查json文档是否为有效对象
    if (!json_request.IsObject()) {
        GlobalLogger->error("Invalid JSON request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
        return;
    }

    // 检查请求的合法性
    if (!isRequestValid(json_request, CheckType::SEARCH)) {
        GlobalLogger->error("Missing vectors or k parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Missing vectors or k parameter in the request");
        return;
    }

    // 获取查询参数
    std::vector<float> query;
    for (const auto& q: json_request[REQUEST_VECTOR].GetArray()) {
        query.push_back(q.GetFloat());
    }
    int k = json_request[REQUEST_K].GetInt();
    
    GlobalLogger->debug("Query parameters: k = {}", k);

    // 获取请求参数中的索引类型
    IndexFactory::IndexType indexType = getIndexTypeFromRequest(json_request);

    // 如果索引类型为UNKNOWN，返回400错误
    if (indexType == IndexFactory::IndexType::UNKNOWN) {
        GlobalLogger->error("Invalid indexType parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid indexType parameter in the request");
        return;
    }

    // 使用 VectorIndex 的 search 接口执行查询
    std::pair<std::vector<long>, std::vector<float>> results = vector_engine_->search(json_request);

    // 将结果转换为JSON
    rapidjson::Document json_response;
    json_response.SetObject();
    rapidjson::Document::AllocatorType& allocator = json_response.GetAllocator();

    // 检查是否有有效的搜索结果
    bool valid_results = false;
    rapidjson::Value vectors(rapidjson::kArrayType);
    rapidjson::Value distances(rapidjson::kArrayType);
    for (size_t i = 0; i < results.first.size(); i++) {
        if (results.first[i] != -1) {
            valid_results = true;
            vectors.PushBack(results.first[i], allocator);
            distances.PushBack(results.second[i], allocator);
        }
    }

    if (valid_results) {
        json_response.AddMember(RESPONSE_VECTORS, vectors, allocator);
        json_response.AddMember(RESPONSE_DISTANCES, distances, allocator);
    }

    // 设置响应
    json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
    setJsonResponse(json_response, res);
}

void HttpServer::insertHandler(const httplib::Request& req, httplib::Response& res) {
    GlobalLogger->debug("Received insert request");

    // 解析JSON请求
    rapidjson::Document json_request;
    json_request.Parse(req.body.c_str());

    // // 打印用户的输入参数
    // GlobalLogger->info("Insert request parameters: {}", req.body);

    // 检查JSON文档是否为有效对象
    if (!json_request.IsObject()) {
        GlobalLogger->error("Invalid JSON request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
        return;
    }

    // 检查请求的合法性
    if (!isRequestValid(json_request, CheckType::INSERT)) { // 添加对isRequestValid的调用
        GlobalLogger->error("Missing vectors or id parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Missing vectors or k parameter in the request");
        return;
    }

    // 获取请求参数中的索引类型
    IndexFactory::IndexType indexType = getIndexTypeFromRequest(json_request);

    // 如果索引类型为UNKNOWN，返回400错误
    if (indexType == IndexFactory::IndexType::UNKNOWN) {
        GlobalLogger->error("Invalid indexType parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid indexType parameter in the request"); 
        return;
    }

    vector_engine_->insert(json_request);
    vector_engine_->writeWalLog("insert", json_request);

    // 设置响应
    rapidjson::Document json_response;
    json_response.SetObject();
    rapidjson::Document::AllocatorType& allocator = json_response.GetAllocator();

    // 添加retCode到响应
    json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);

    setJsonResponse(json_response, res);
}

void HttpServer::queryHandler(const httplib::Request& req, httplib::Response& res) {
    GlobalLogger->debug("Received query request");

    // 解析JSON请求
    rapidjson::Document json_request;
    json_request.Parse(req.body.c_str());

    // // 打印用户的输入参数
    // GlobalLogger->info("Insert request parameters: {}", req.body);

    // 检查JSON文档是否为有效对象
    if (!json_request.IsObject()) {
        GlobalLogger->error("Invalid JSON request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
        return;
    }

    // 检查请求的合法性
    if (!isRequestValid(json_request, CheckType::QUERY)) { // 添加对isRequestValid的调用
        GlobalLogger->error("Missing vectors or id parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Missing vectors or k parameter in the request");
        return;
    }

    rapidjson::Document result = vector_engine_->query(json_request);

    // 设置响应
    rapidjson::Document json_response;
    json_response.SetObject();
    rapidjson::Document::AllocatorType& allocator = json_response.GetAllocator();

    if (result.IsObject()) {
        // 添加retCode到响应
        json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);
        json_response.AddMember(RESPONSE_RETDATA, result, allocator);
    } else {
        json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_ERROR, allocator);
    }

    setJsonResponse(json_response, res);
}

void HttpServer::insertBatchHandler(const httplib::Request& req, httplib::Response& res) {
    GlobalLogger->debug("Received insert batch request");

    // 解析JSON请求
    rapidjson::Document json_request;
    json_request.Parse(req.body.c_str());

    // // 打印用户的输入参数
    // GlobalLogger->info("Insert request parameters: {}", req.body);

    // 检查JSON文档是否为有效对象
    if (!json_request.IsObject()) {
        GlobalLogger->error("Invalid JSON request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
        return;
    }

    // 检查请求的合法性
    if (!isRequestValid(json_request, CheckType::INSERT_BATCH)) { // 添加对isRequestValid的调用
        GlobalLogger->error("Missing vectors or id parameter in the request");
        res.status = 400;
        setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Missing vectors or k parameter in the request");
        return;
    }

    vector_engine_->insert_batch(json_request);
    vector_engine_->writeWalLog("insert_batch", json_request);

    // 设置响应
    rapidjson::Document json_response;
    json_response.SetObject();
    rapidjson::Document::AllocatorType& allocator = json_response.GetAllocator();

    // 添加retCode到响应
    json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS, allocator);

    setJsonResponse(json_response, res);
}

IndexFactory::IndexType getIndexTypeFromRequest(const rapidjson::Document& json_request) {
    // 获取请求参数中的索引类型
    if (json_request.HasMember(REQUEST_INDEX_TYPE)) {
        std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
        if (index_type_str == INDEX_TYPE_FLAT) {
            return IndexFactory::IndexType::FLAT;
        } else if (index_type_str == INDEX_TYPE_HNSW) {
            return IndexFactory::IndexType::HNSW;
        } else if (index_type_str == INDEX_TYPE_HNSWFLAT) {
            return IndexFactory::IndexType::HNSWFLAT;
        }
    }
    return IndexFactory::IndexType::UNKNOWN;
}