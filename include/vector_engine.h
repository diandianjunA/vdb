#pragma once

#include "vector_index.h"
#include "vector_storage.h"

class VectorEngine {
public:
    VectorEngine(VectorIndex* vector_index, VectorStorage* vector_storage);
    ~VectorEngine();

    std::pair<std::vector<long>, std::vector<float>> search(const rapidjson::Document& json_request);
    void insert(const rapidjson::Document& json_request);
    rapidjson::Document query(const rapidjson::Document& json_request);
    void insert_batch(const rapidjson::Document& json_request);

private:
    VectorIndex* vector_index_;
    VectorStorage* vector_storage_;
};

