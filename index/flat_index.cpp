#include "include/flat_index.h"
#include "include/logger.h"
#include <faiss/IndexIDMap.h>

FlatIndex::FlatIndex(faiss::Index* index) : index(index) {}

void FlatIndex::insert_vectors(const std::vector<float>& data, uint64_t label) {
    long id = static_cast<long>(label);
    index->add_with_ids(1, data.data(), &id);
}

std::pair<std::vector<long>, std::vector<float>> FlatIndex::search_vectors(const std::vector<float>& query, int k) {
    int dim = index->d;
    int num_queries = query.size() / dim;
    std::vector<long> indices(num_queries * k);
    std::vector<float> distances(num_queries * k);
    
    index->search(num_queries, query.data(), k, distances.data(), indices.data());
    return {indices, distances};
}

void FlatIndex::remove_vectors(const std::vector<long>& ids) {
    faiss::IndexIDMap* id_map =  dynamic_cast<faiss::IndexIDMap*>(index);
    if (id_map) {
        faiss::IDSelectorBatch selector(ids.size(), ids.data());
        id_map->remove_ids(selector);
    } else {
        throw std::runtime_error("Underlying Faiss index is not an IndexIDMap");
    }
}