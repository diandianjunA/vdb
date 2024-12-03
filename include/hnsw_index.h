#pragma once

#include "hnswlib/hnswlib/hnswlib.h"
#include <vector>
#include "include/index_factory.h"

class HnswIndex {
public:
    HnswIndex(int dim, int num_data, IndexFactory::MetricType metric, int M = 16, int ef_construction = 200);
    void insert_vectors(const std::vector<float>& data, uint64_t id);
    void insert_batch_vectors(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids);
    void remove_vectors(const std::vector<long>& ids);
    std::pair<std::vector<long>, std::vector<float>> search_vectors(const std::vector<float>& query, int k, int ef_search = 50);

    void saveIndex(const std::string& file_path);
    void loadIndex(const std::string& file_path);
private:
    int dim;
    hnswlib::SpaceInterface<float>* space;
    hnswlib::HierarchicalNSW<float>* index;
    size_t max_elements;
};