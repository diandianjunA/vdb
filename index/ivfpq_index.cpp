#include "include/ivfpq_index.h"
#include "include/logger.h"
#include <faiss/IndexIDMap.h>
#include <faiss/index_io.h> 
#include <fstream>
#include "ivfpq_index.h"

IVFPQIndex::IVFPQIndex(faiss::Index* index) : index(index) {
    this->id_map = new faiss::IndexIDMap(index);
};

void IVFPQIndex::insert_vectors(const std::vector<float>& data, uint64_t label) {
    long id = static_cast<long>(label);
    try {
        id_map->add_with_ids(1, data.data(), &id);
    } catch (const std::exception& e) {
        GlobalLogger->error("insert error: {}", e.what());
    }
}

void IVFPQIndex::insert_batch_vectors(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids) {
    try {
        id_map->add_with_ids(vectors.size(), vectors.data()->data(), ids.data());
    } catch (std::runtime_error e) {
        GlobalLogger->error("insert error: {}", e.what());
    }
}

void IVFPQIndex::remove_vectors(const std::vector<long>& ids) {
    faiss::IDSelectorBatch selector(ids.size(), ids.data());
    id_map->remove_ids(selector);
}

std::pair<std::vector<long>, std::vector<float>> IVFPQIndex::search_vectors(const std::vector<float>& query, int k) {
    int dim = index->d;
    int num_queries = query.size() / dim;
    std::vector<long> indices(num_queries * k);
    std::vector<float> distances(num_queries * k);
    
    index->search(num_queries, query.data(), k, distances.data(), indices.data());
    return {indices, distances};
}

void IVFPQIndex::saveIndex(const std::string& file_path) {
    faiss::write_index(index, file_path.c_str());
}

void IVFPQIndex::loadIndex(const std::string& file_path) {
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

void IVFPQIndex::train(int num_train, const std::vector<float>& train_vec) {
    index->train(num_train, train_vec.data());
}

void IVFPQIndex::add(int num_train, const std::vector<float>& train_vec) {
    index->add(num_train, train_vec.data());
}