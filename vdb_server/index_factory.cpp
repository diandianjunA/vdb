#include "include/index_factory.h"
#include "include/flat_index.h"
#include "include/hnsw_flat_index.h"
#include <faiss/MetricType.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexHNSW.h>
#include <faiss/IndexIDMap.h>

namespace {
    IndexFactory globalIndexFactory;
}

IndexFactory* getGlobalIndexFactory() {
    return &globalIndexFactory;
}

void* IndexFactory::init(IndexType type, int dim, int max_elements, MetricType metric) {
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;
    switch (type) {
        case IndexType::FLAT: {
            return new FlatIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
        }
        case IndexType::HNSWFLAT: {
            auto index = new faiss::IndexHNSWFlat(dim, 16);
            index->hnsw.efConstruction = 200;
            index->hnsw.efSearch = 50;
            return new HnswFlatIndex(new faiss::IndexIDMap(index));
        }
        default:
            return nullptr;
    }
}