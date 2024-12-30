#pragma once

#include <string>

class IndexFactory {
public:
    enum class IndexType {
        FLAT,
        HNSWFLAT,
        FLAT_GPU,
        IVFPQ,
        CAGRA,
        UNKNOWN = -1 
    };

    enum class MetricType {
        L2,
        IP
    };

    void* init(IndexType type, int dim = 1, int num_data = 0, MetricType metric = MetricType::L2);
};

IndexFactory* getGlobalIndexFactory();