#include "include/index_factory.h"
#include "include/flat_index.h"
#include "include/hnsw_flat_index.h"
#include "include/flat_gpu_index.h"
#include "include/ivfpq_index.h"
#include "include/cagra_index.h"
#include "include/logger.h"
#include <faiss/MetricType.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexHNSW.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/gpu/GpuIndexIVFPQ.h>
#include <faiss/gpu/GpuIndexCagra.h>
#include <faiss/IndexIDMap.h>
#include <faiss/gpu/GpuIndexFlat.h>
#include <faiss/gpu/StandardGpuResources.h>
#include <faiss/gpu/impl/IndexUtils.h>
#include <faiss/gpu/utils/DeviceUtils.h>
#include <random>
#include <ctime>
 
int getRandomIntInRange(int min, int max) {
    // 创建一个随机数生成器
    std::random_device rd;  // 用于获取随机数种子
    std::mt19937 gen(rd()); // 以 rd() 作为种子的 Mersenne Twister 引擎
    std::uniform_int_distribution<> dis(min, max); // [min, max] 范围的均匀分布
 
    return dis(gen); // 生成并返回随机数
}

std::vector<float> randVecs(size_t num, size_t dim) {
    std::vector<float> v(num * dim);
    auto seed = static_cast<unsigned>(std::time(0));

    faiss::float_rand(v.data(), v.size(), seed);

    return v;
}

namespace {
    IndexFactory globalIndexFactory;
}

IndexFactory* getGlobalIndexFactory() {
    return &globalIndexFactory;
}

void* IndexFactory::init(IndexType type, int dim, int max_elements, MetricType metric) {
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;
    switch (type) {
        case IndexType::FLAT: {
            return new FlatIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
        }
        case IndexType::HNSWFLAT: {
            int M = 16;
            auto index = new faiss::IndexHNSWFlat(dim, M);
            index->hnsw.efConstruction = 200;
            index->hnsw.efSearch = 50;
            return new HnswFlatIndex(new faiss::IndexIDMap(index));
        }
        case IndexType::FLAT_GPU: {
            std::srand(static_cast<unsigned>(std::time(0)));
            int device = getRandomIntInRange(0, faiss::gpu::getNumDevices() - 1);
            faiss::gpu::StandardGpuResources res;
            res.noTempMemory();

            faiss::gpu::GpuIndexFlatConfig config;
            config.device = device;
            config.useFloat16 = false;
            return new FlatGPUIndex(new faiss::IndexIDMap(new faiss::gpu::GpuIndexFlat(&res, dim, faiss_metric, config)));
        }
        case IndexType::IVFPQ: {
            std::srand(static_cast<unsigned>(std::time(0)));
            faiss::IndexFlatL2 coarseQuantizerL2(dim);
            faiss::IndexFlatIP coarseQuantizerIP(dim);
            faiss::Index* quantizer = faiss_metric == faiss::METRIC_L2 ? (faiss::Index*)&coarseQuantizerL2 : (faiss::Index*)&coarseQuantizerIP;
            int num_centroids = 256;
            int code = 32;
            int bitsPerCode = 8;
            int num_train = num_centroids * 40;
            int nprobe = std::min(getRandomIntInRange(40, 1000), num_centroids);
            faiss::IndexIVFPQ cpuIndex(quantizer, dim, num_centroids, code, bitsPerCode);
            cpuIndex.metric_type = faiss_metric;
            cpuIndex.nprobe = nprobe;
            std::vector<float> train_vec = randVecs(num_train, dim);
            cpuIndex.train(num_train, train_vec.data());

            int device = getRandomIntInRange(0, faiss::gpu::getNumDevices() - 1);
            bool usePrecomputed = true;
            faiss::gpu::IndicesOptions indicesOpt = faiss::gpu::INDICES_64_BIT;
            faiss::gpu::StandardGpuResources res;

            faiss::gpu::GpuIndexIVFPQConfig config;
            config.device = device;
            config.usePrecomputedTables = usePrecomputed;
            config.indicesOptions = indicesOpt;
            config.useFloat16LookupTables = false;
            config.interleavedLayout = false;
            config.use_cuvs = true;

            return new IVFPQIndex(new faiss::IndexIDMap(new faiss::gpu::GpuIndexIVFPQ(&res, &cpuIndex, config)));
        }
        case IndexType::CAGRA: {
            std::srand(static_cast<unsigned>(std::time(0)));
            int device = getRandomIntInRange(0, faiss::gpu::getNumDevices() - 1);
            int graph_degree = getRandomIntInRange(32, 64);
            int intermediateGraphDegree = getRandomIntInRange(64, 98);
            faiss::gpu::StandardGpuResources res;
            res.noTempMemory();

            int num_train = 2 * getRandomIntInRange(2000, 5000);
            std::vector<float> train_vec = randVecs(num_train, dim);

            faiss::gpu::GpuIndexCagraConfig config;
            config.device = device;
            config.graph_degree = graph_degree;
            config.intermediate_graph_degree = intermediateGraphDegree;
            config.build_algo = faiss::gpu::graph_build_algo::NN_DESCENT;

            faiss::gpu::GpuIndexCagra* gpu_index = new faiss::gpu::GpuIndexCagra(&res, dim, faiss_metric, config);
            int M = 16;
            faiss::IndexHNSWCagra* cpu_index = new faiss::IndexHNSWCagra(dim, M, faiss_metric);
            cpu_index->base_level_only = false;
            cpu_index->hnsw.efConstruction = 200;
            CAGRAIndex* carga_index = new CAGRAIndex(cpu_index, gpu_index);
            carga_index->train(num_train, train_vec);
            return carga_index;
        }
        default:
            return nullptr;
    }
}