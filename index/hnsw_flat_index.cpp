#include "include/hnsw_flat_index.h"
#include "include/logger.h"
#include <faiss/IndexIDMap.h>
#include <faiss/index_io.h> 
#include <fstream>

HnswFlatIndex::HnswFlatIndex(faiss::Index* index) : index(index) {
    this->id_map = new faiss::IndexIDMap(index);
}

void HnswFlatIndex::insert_vectors(const std::vector<float>& data, uint64_t label) {
    long id = static_cast<long>(label);
    id_map->add_with_ids(1, data.data(), &id);
}

void HnswFlatIndex::insert_batch_vectors(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids) {
    id_map->add_with_ids(vectors.size(), vectors.data()->data(), ids.data());
}

std::pair<std::vector<long>, std::vector<float>> HnswFlatIndex::search_vectors(const std::vector<float>& query, int k) {
    int dim = index->d;
    int num_queries = query.size() / dim;
    std::vector<long> indices(num_queries * k);
    std::vector<float> distances(num_queries * k);
    
    index->search(num_queries, query.data(), k, distances.data(), indices.data());
    return {indices, distances};
}

void HnswFlatIndex::remove_vectors(const std::vector<long>& ids) {
    faiss::IDSelectorBatch selector(ids.size(), ids.data());
    id_map->remove_ids(selector);
}

void HnswFlatIndex::saveIndex(const std::string& file_path) {
    faiss::write_index(index, file_path.c_str());
}

void HnswFlatIndex::loadIndex(const std::string& file_path) {
    std::ifstream file(file_path);
    if (file.good()) {
        file.close();
        if (index != nullptr) {
            delete index;
        }
        index = faiss::read_index(file_path.c_str());
    } else {
        GlobalLogger->warn("File not found: {}. Skipping loading index.", file_path);
    }
}

void HnswFlatIndex::train(int num_train, const std::vector<float>& train_vec) {
    index->train(num_train, train_vec.data());
}

void HnswFlatIndex::add(int num_train, const std::vector<float>& train_vec) {
    index->add(num_train, train_vec.data());
}