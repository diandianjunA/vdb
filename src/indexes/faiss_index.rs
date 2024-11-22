use std::error::Error;
use std::u64;

use crate::index_factory::VectorIndex;
use faiss::index::IndexImpl;
use faiss::selector::IdSelector;
use faiss::{index_factory, IdMap, Idx, Index, MetricType};

pub struct FaissIndex {
    id_map: IdMap<IndexImpl>,
}

impl FaissIndex {
    pub fn new(d: u32, description: &str, metric: MetricType) -> Result<Self, Box<dyn Error>> {
        let index = index_factory(d, description, metric)?;
        let id_map = IdMap::new(index)?;
        Ok(FaissIndex { id_map })
    }
}

impl VectorIndex for FaissIndex {
    fn insert(&mut self, data: &[f32], id: i64) -> Result<i64, Box<dyn Error>> {
        self.id_map.add_with_ids(data, &[id.into()])?;
        Ok(id)
    }

    fn remove(&mut self, ids: &[i64]) -> Result<(), Box<dyn Error>> {
        let idx: Vec<Idx> = ids.iter().map(|x| (*x).into()).collect();
        let selector: IdSelector = IdSelector::batch(idx.as_slice())?;
        self.id_map.remove_ids(&selector)?;
        Ok(())
    }

    fn search(
        &mut self,
        query: &[f32],
        k: usize,
        _ef_search: i32,
    ) -> Result<(Vec<i64>, Vec<f32>), Box<dyn Error>> {
        let result = self.id_map.search(query, k)?;

        if result.labels.is_empty() {
            return Ok((Vec::new(), Vec::new()));
        }
        // debug!("indices: {:#?}, distances: {:#?}", result.labels, result.distances);
        let indices: Vec<i64> = result
            .labels
            .iter()
            .map(|x| (*x).get().unwrap_or(u64::MAX) as i64)
            .collect();
        let distances: Vec<f32> = result.distances;

        Ok((indices, distances))
    }
}
