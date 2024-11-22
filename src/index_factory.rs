use std::{
    collections::HashMap,
    error::Error,
    sync::{Arc, Mutex},
};

use crate::indexes::{
    faiss_index::FaissIndex,
    hnsw_index::{
        Distance,
        Distance::{Euclidean, InnerProduct},
        HNSWIndex,
    },
};

#[derive(PartialEq, Eq, Hash, Clone, Copy)]
pub enum IndexType {
    FLAT,
    HNSW,
    FILTER,
    UNKNOWN = -1,
}

#[derive(PartialEq, Eq)]
pub enum MetricType {
    L2,
    IP,
}

pub struct GlobalIndexFactory;

impl GlobalIndexFactory {
    fn init_index(
        &self,
        type_: IndexType,
        dim: u32,
        _num_data: u32,
        metric: MetricType,
    ) -> Result<(), Box<dyn Error>> {
        let mut map = GLOBAL_MAP.lock().unwrap();
        map.insert(
            type_,
            match type_ {
                IndexType::FLAT => {
                    let faiss_metric: faiss::MetricType = if metric == MetricType::L2 {
                        faiss::MetricType::L2
                    } else {
                        faiss::MetricType::InnerProduct
                    };
                    Arc::new(Mutex::new(Box::new(FaissIndex::new(
                        dim,
                        "Flat",
                        faiss_metric,
                    )?)))
                }
                IndexType::HNSW => {
                    let hnsw_metric: Distance = if metric == MetricType::L2 {
                        Euclidean
                    } else {
                        InnerProduct
                    };
                    Arc::new(Mutex::new(Box::new(HNSWIndex::new(dim, hnsw_metric)?)))
                }
                IndexType::FILTER => todo!(),
                IndexType::UNKNOWN => todo!(),
            },
        );
        Ok(())
    }

    fn get_index(&self, type_: IndexType) -> Option<Arc<Mutex<Box<dyn VectorIndex>>>> {
        let map = GLOBAL_MAP.lock().unwrap();
        return map.get(&type_).cloned();
    }
}

pub trait VectorIndex: Sync + Send {
    fn insert(&mut self, data: &[f32], id: i64) -> Result<i64, Box<dyn Error>>;

    fn remove(&mut self, ids: &[i64]) -> Result<(), Box<dyn Error>>;

    fn search(
        &mut self,
        query: &[f32],
        k: usize,
        ef_search: i32,
    ) -> Result<(Vec<i64>, Vec<f32>), Box<dyn Error>>;
}

lazy_static! {
    static ref GLOBAL_MAP: Arc<Mutex<HashMap<IndexType, Arc<Mutex<Box<dyn VectorIndex>>>>>> = {
        let map = HashMap::new();
        Arc::new(Mutex::new(map))
    };
    static ref GLOBAL_INDEX_FACTORY: Arc<Mutex<GlobalIndexFactory>> = {
        let global_index_factory = GlobalIndexFactory {};
        Arc::new(Mutex::new(global_index_factory))
    };
}

pub fn init_index(
    type_: IndexType,
    dim: u32,
    _num_data: u32,
    metric: MetricType,
) -> Result<(), Box<dyn Error>> {
    let global_index_factory = GLOBAL_INDEX_FACTORY.lock().unwrap();
    global_index_factory.init_index(type_, dim, _num_data, metric)
}

pub fn get_index(type_: IndexType) -> Option<Arc<Mutex<Box<dyn VectorIndex>>>> {
    let global_index_factory = GLOBAL_INDEX_FACTORY.lock().unwrap();
    global_index_factory.get_index(type_)
}
