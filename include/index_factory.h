#pragma once

#include <map>
#include <string>

class IndexFactory {
public:
    enum class IndexType {
        FLAT,
        HNSW,
        HNSWFLAT,
        FILTER,
        UNKNOWN = -1 
    };

    enum class MetricType {
        L2,
        IP
    };
    void init(IndexType type, int dim = 1, int num_data = 0, MetricType metric = MetricType::L2);
    void* getIndex(IndexType type) const;

    void saveIndex(const std::string& folder_path);
    void loadIndex(const std::string& folder_path);
private:
    std::map<IndexType, void*> index_map;
};

IndexFactory* getGlobalIndexFactory();