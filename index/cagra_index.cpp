#include "include/cagra_index.h"
#include "include/logger.h"
#include <faiss/Index.h>
#include <faiss/IndexHNSW.h>
#include <faiss/IndexIDMap.h>
#include <faiss/index_io.h>
#include <faiss/gpu/GpuIndexCagra.h>
#include <fstream>

CAGRAIndex::CAGRAIndex(faiss::Index* cpu_index, faiss::gpu::GpuIndexCagra* gpu_index): cpu_index(cpu_index), gpu_index(gpu_index) {
    this->id_map = new faiss::IndexIDMap(cpu_index);
};

void CAGRAIndex::insert_vectors(const std::vector<float>& data, uint64_t label) {
    long id = static_cast<long>(label);
    try {
        id_map->add_with_ids(1, data.data(), &id);
    } catch (const std::exception& e) {
        GlobalLogger->error("insert error: {}", e.what());
    }
}

void CAGRAIndex::insert_batch_vectors(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids) {
    try {
        id_map->add_with_ids(vectors.size(), vectors.data()->data(), ids.data());
    } catch (std::runtime_error e) {
        GlobalLogger->error("insert error: {}", e.what());
    }
}

void CAGRAIndex::remove_vectors(const std::vector<long>& ids) {
    faiss::IDSelectorBatch selector(ids.size(), ids.data());
    id_map->remove_ids(selector);
}

std::pair<std::vector<long>, std::vector<float>> CAGRAIndex::search_vectors(const std::vector<float>& query, int k) {
    int dim = cpu_index->d;
    int num_queries = query.size() / dim;
    std::vector<long> indices(num_queries * k);
    std::vector<float> distances(num_queries * k);
    
    gpu_index->search(num_queries, query.data(), k, distances.data(), indices.data());
    return {indices, distances};
}

void CAGRAIndex::saveIndex(const std::string& file_path) {
    faiss::write_index(cpu_index, file_path.c_str());
}

void CAGRAIndex::loadIndex(const std::string& file_path) {
    std::ifstream file(file_path);
    if (file.good()) {
        file.close();
        if (cpu_index != nullptr) {
            delete cpu_index;
        }
        cpu_index = faiss::read_index(file_path.c_str());
    } else {
        GlobalLogger->warn("File not found: {}. Skipping loading index.", file_path);
    }
}

void CAGRAIndex::train(int num_train, const std::vector<float>& train_vec) {
    gpu_index->train(num_train, train_vec.data());
    // gpu_index->copyTo(dynamic_cast<faiss::IndexHNSWCagra*>(cpu_index));
}

void CAGRAIndex::add(int num_train, const std::vector<float>& train_vec) {
    cpu_index->add(num_train, train_vec.data());
}

void CAGRAIndex::update_index() {
    gpu_index->copyFrom(dynamic_cast<faiss::IndexHNSWCagra*>(cpu_index));
}