#pragma once

#include <faiss/IndexHNSW.h>
#include <vector>

class HnswFlatIndex {
public:
    HnswFlatIndex(faiss::Index* index);
    void insert_vectors(const std::vector<float>& data, uint64_t label);
    void insert_batch_vectors(const std::vector<std::vector<float>>& vectors, const std::vector<long>& ids);
    void remove_vectors(const std::vector<long>& ids);
    std::pair<std::vector<long>, std::vector<float>> search_vectors(const std::vector<float>& query, int k);

    void train(int num_train, const std::vector<float>& train_vec);
    void add(int num_train, const std::vector<float>& train_vec);

    void saveIndex(const std::string& file_path);
    void loadIndex(const std::string& file_path);
private:
    faiss::Index* index;
};