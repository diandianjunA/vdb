#include "include/index_factory.h"
#include "include/flat_index.h"
#include "include/hnsw_index.h"
#include <faiss/MetricType.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>

namespace {
    IndexFactory globalIndexFactory;
}

IndexFactory* getGlobalIndexFactory() {
    return &globalIndexFactory;
}

void IndexFactory::init(IndexType type, int dim, int num_data, MetricType metric) {
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;
    switch (type) {
        case IndexType::FLAT:
            index_map[type] = new FlatIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
            break;
        case IndexType::HNSW:
            index_map[type] = new HnswIndex(dim, num_data, metric);
            break;
        default:
            break;
    }
}

void* IndexFactory::getIndex(IndexType type) const {
    auto it = index_map.find(type);
    if (it != index_map.end()) {
        return it->second;
    }
    return nullptr;
}
