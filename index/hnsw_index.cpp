#include "include/hnsw_index.h"
#include "include/logger.h"
#include <faiss/IndexIDMap.h>

HnswIndex::HnswIndex(int dim, int num_data, IndexFactory::MetricType metric, int M, int ef_construction) :dim(dim) {
    bool normalize = false;
    if (metric == IndexFactory::MetricType::L2) {
        space = new hnswlib::L2Space(dim);
    } else {
        throw std::runtime_error("Invalid metric type");
    }
    index = new hnswlib::HierarchicalNSW<float>(space, num_data, M, ef_construction);
}

void HnswIndex::insert_vectors(const std::vector<float>& data, uint64_t id) {
    index->addPoint(data.data(), id);
}

std::pair<std::vector<long>, std::vector<float>> HnswIndex::search_vectors(const std::vector<float>& query, int k, int ef_search) {
    index->setEf(ef_search);

    auto result = index->searchKnn(query.data(), k);

    std::vector<long> indices(k);
    std::vector<float> distances(k);
    while (!result.empty()) { // 检查result是否为空
        auto item = result.top();
        indices.push_back(item.second);
        distances.push_back(item.first);
        result.pop();
    }

    return {indices, distances};
}