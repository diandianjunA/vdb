#pragma once

#include <vector>
#include "hnswlib/hnswlib.h"

class CUDAHNSWIndex {
public:
    // 构造函数
    CUDAHNSWIndex(int dim, int num_data, int M = 16, int ef_construction = 200); // 将MetricType参数修改为第三个参数

    void init_gpu();

    void check();

    // 插入向量
    void insert_vectors(const float* data, long label);
    void insert_vectors_batch(const std::vector<std::vector<float>>& data, const std::vector<long>& labels);
    void insert_vectors_batch(const std::vector<float>& data, const std::vector<long>& labels);

    // 查询向量
    std::pair<std::vector<long>, std::vector<float>> search_vectors(const std::vector<float>& query, int k, int ef_search = 50);

    std::pair<std::vector<long>, std::vector<float>> search_vectors_gpu(const std::vector<float>& query, int k, int ef_search = 50, bool use_hierarchy = true);

    std::vector<std::pair<std::vector<long>, std::vector<float>>> search_vectors_batch_gpu(const std::vector<float>& query, int k, int ef_search = 50, bool use_hierarchy = true);

private:
    int dim;
    hnswlib::SpaceInterface<float>* space;
    hnswlib::HierarchicalNSW<float>* index;
};