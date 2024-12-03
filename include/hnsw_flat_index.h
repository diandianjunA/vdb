#pragma once

#include <faiss/IndexHNSW.h>
#include <vector>

class HnswFlatIndex {
public:
    HnswFlatIndex(faiss::Index* index);
    void insert_vectors(const std::vector<float>& data, uint64_t label);
    void insert_batch_vectors(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids);
    void remove_vectors(const std::vector<long>& ids);
    std::pair<std::vector<long>, std::vector<float>> search_vectors(const std::vector<float>& query, int k);
private:
    faiss::Index* index;
};