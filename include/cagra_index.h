#pragma once

#include <faiss/Index.h>
#include <faiss/IndexHNSW.h>
#include <faiss/impl/IDSelector.h>
#include <vector>
#include <faiss/gpu/GpuIndexCagra.h>
#include <faiss/IndexIDMap.h>

class CAGRAIndex {
public:
    CAGRAIndex(faiss::Index* cpu_index, faiss::gpu::GpuIndexCagra* gpu_index);
    void insert_vectors(const std::vector<float>& data, uint64_t label);
    void insert_batch_vectors(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids);
    void remove_vectors(const std::vector<long>& ids);
    std::pair<std::vector<long>, std::vector<float>> search_vectors(const std::vector<float>& query, int k);

    void train(int num_train, const std::vector<float>& train_vec);
    void add(int num_train, const std::vector<float>& train_vec);

    void saveIndex(const std::string& file_path);
    void loadIndex(const std::string& file_path);

    void update_index();
    
private:
    faiss::gpu::GpuIndexCagra* gpu_index;
    faiss::Index* cpu_index;
    faiss::IndexIDMap* id_map;
};