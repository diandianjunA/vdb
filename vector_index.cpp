#include "include/vector_index.h"
#include "include/index_factory.h"
#include "include/flat_index.h"
#include "include/hnsw_index.h"
#include "include/constant.h"
#include "include/logger.h"

VectorIndex::VectorIndex() {};

std::pair<std::vector<long>, std::vector<float>> VectorIndex::search(IndexFactory::IndexType indexType, const std::vector<float>& data, int k) {
    // 使用全局 IndexFactory 获取索引对象
    void* index = getGlobalIndexFactory()->getIndex(indexType);

    // 根据索引类型初始化索引对象并调用 search_vectors 函数
    std::pair<std::vector<long>, std::vector<float>> results;
    switch (indexType) {
        case IndexFactory::IndexType::FLAT: {
            FlatIndex* flat_index = static_cast<FlatIndex*>(index);
            results = flat_index->search_vectors(data, k);
            break;
        }
        case IndexFactory::IndexType::HNSW: {
            HnswIndex* hnsw_index = static_cast<HnswIndex*>(index);
            results = hnsw_index->search_vectors(data, k);
        }
        default:
            break;
    }
    return results;
}

void VectorIndex::insert(IndexFactory::IndexType indexType, const std::vector<float>& data, uint64_t id) {
    // 使用全局 IndexFactory 获取索引对象
    void* index = getGlobalIndexFactory()->getIndex(indexType);

    // 根据索引类型初始化索引对象并调用 search_vectors 函数
    std::pair<std::vector<long>, std::vector<float>> results;
    switch (indexType) {
        case IndexFactory::IndexType::FLAT: {
            FlatIndex* flat_index = static_cast<FlatIndex*>(index);
            flat_index->insert_vectors(data, id);
            break;
        }
        case IndexFactory::IndexType::HNSW: {
            HnswIndex* hnsw_index = static_cast<HnswIndex*>(index);
            hnsw_index->insert_vectors(data, id);
            break;
        }
        default:
            break;
    }
}
