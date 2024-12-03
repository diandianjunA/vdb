#pragma once

#include "index_factory.h"
#include <string>
#include <vector>
#include "rapidjson/document.h"

class VectorIndex {
public:
    VectorIndex();

    std::pair<std::vector<long>, std::vector<float>> search(IndexFactory::IndexType indexType, const std::vector<float>& data, int k);
    void insert(IndexFactory::IndexType indexType, const std::vector<float>& data, uint64_t id);

    void saveIndex(const std::string& folder_path);
    void loadIndex(const std::string& folder_path);
};
