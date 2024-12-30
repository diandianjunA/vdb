#include "include/cagra_index.h"
#include "include/logger.h"
#include <faiss/IndexIDMap.h>
#include <faiss/index_io.h> 
#include <fstream>

CAGRAIndex::CAGRAIndex(faiss::Index* index) : index(index) {};

void CAGRAIndex::insert_vectors(const std::vector<float>& data, uint64_t label) {
    long id = static_cast<long>(label);
    index->add_with_ids(1, data.data(), &id);
}

void CAGRAIndex::insert_batch_vectors(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids) {
    index->add_with_ids(vectors.size(), vectors.data()->data(), ids.data());
}

void CAGRAIndex::remove_vectors(const std::vector<long>& ids) {
    faiss::IndexIDMap* id_map =  dynamic_cast<faiss::IndexIDMap*>(index);
    if (id_map) {
        faiss::IDSelectorBatch selector(ids.size(), ids.data());
        id_map->remove_ids(selector);
    } else {
        throw std::runtime_error("Underlying Faiss index is not an IndexIDMap");
    }
}

std::pair<std::vector<long>, std::vector<float>> CAGRAIndex::search_vectors(const std::vector<float>& query, int k) {
    int dim = index->d;
    int num_queries = query.size() / dim;
    std::vector<long> indices(num_queries * k);
    std::vector<float> distances(num_queries * k);
    
    index->search(num_queries, query.data(), k, distances.data(), indices.data());
    return {indices, distances};
}

void CAGRAIndex::saveIndex(const std::string& file_path) {
    faiss::write_index(index, file_path.c_str());
}

void CAGRAIndex::loadIndex(const std::string& file_path) {
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