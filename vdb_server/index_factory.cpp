#include "include/index_factory.h"
#include "include/flat_index.h"
#include "include/hnsw_index.h"
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

void IndexFactory::init(IndexType type, int dim, int max_elements, MetricType metric) {
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;
    switch (type) {
        case IndexType::FLAT: {
            index_map[type] = new FlatIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
            break;
        }
        case IndexType::HNSW: {
            index_map[type] = new HnswIndex(dim, max_elements, metric);
            break;
        }
        case IndexType::HNSWFLAT: {
            auto index = new faiss::IndexHNSWFlat(dim, 16);
            index->hnsw.efConstruction = 200;
            index->hnsw.efSearch = 50;
            index_map[type] = new HnswFlatIndex(new faiss::IndexIDMap(index));
            break;
        }
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

void IndexFactory::saveIndex(const std::string& folder_path) {
    for (const auto& index_entry : index_map) {
        IndexType index_type = index_entry.first;
        void* index = index_entry.second;

        // 为每个索引类型生成一个文件名
        std::string file_path = folder_path + std::to_string(static_cast<int>(index_type)) + ".index";

        // 根据索引类型调用相应的 saveIndex 函数
        if (index_type == IndexType::FLAT) {
            static_cast<FlatIndex*>(index)->saveIndex(file_path);
        } else if (index_type == IndexType::HNSW) {
            static_cast<HnswIndex*>(index)->saveIndex(file_path);
        } else if (index_type == IndexType::HNSWFLAT) {
            static_cast<HnswFlatIndex*>(index)->saveIndex(file_path);
        }
    }
}

void IndexFactory::loadIndex(const std::string& folder_path) {
    for (const auto& index_entry : index_map) {
        IndexType index_type = index_entry.first;
        void* index = index_entry.second;

        // 为每个索引类型生成一个文件名
        std::string file_path = folder_path + std::to_string(static_cast<int>(index_type)) + ".index";

        // 根据索引类型调用相应的 loadIndex 函数
        if (index_type == IndexType::FLAT) {
            static_cast<FlatIndex*>(index)->loadIndex(file_path);
        } else if (index_type == IndexType::HNSW) {
            static_cast<HnswIndex*>(index)->loadIndex(file_path);
        } else if (index_type == IndexType::FILTER) { // 加载 FilterIndex 类型的索引
            static_cast<HnswFlatIndex*>(index)->loadIndex(file_path);
        }
    }
}