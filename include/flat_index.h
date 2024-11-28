#pragma once

#include <faiss/Index.h>
#include <faiss/impl/IDSelector.h>
#include <vector>

class FlatIndex {
public:
    FlatIndex(faiss::Index* index);
    void insert_vectors(const std::vector<float>& data, uint64_t label);
    void remove_vectors(const std::vector<long>& ids);
    std::pair<std::vector<long>, std::vector<float>> search_vectors(const std::vector<float>& query, int k);
private:
    faiss::Index* index;
};