use core::f32;
use std::error::Error;

use crate::index_factory::VectorIndex;
use hnsw::{Hnsw, Searcher};
use rand_pcg::Pcg64;
use space::{Metric, Neighbor};

pub enum Distance {
    Euclidean,
    InnerProduct,
}

impl Metric<Vec<f32>> for Distance {
    type Unit = u32;

    fn distance(&self, a: &Vec<f32>, b: &Vec<f32>) -> Self::Unit {
        match self {
            Distance::Euclidean => a
                .iter()
                .zip(b.iter())
                .map(|(&a, &b)| (a - b).powi(2))
                .sum::<f32>()
                .sqrt()
                .to_bits(),
            Distance::InnerProduct => a
                .iter()
                .zip(b.iter())
                .map(|(&a, &b)| a * b)
                .sum::<f32>()
                .sqrt()
                .to_bits(),
        }
    }
}

pub struct HNSWIndex {
    searcher: Searcher<u32>,
    hnsw: Hnsw<Distance, Vec<f32>, Pcg64, 12, 24>,
}

impl<'a> HNSWIndex {
    pub fn new(_d: u32, metric: Distance) -> Result<Self, Box<dyn Error>> {
        let searcher: Searcher<u32> = Searcher::default();
        let hnsw: Hnsw<Distance, Vec<f32>, Pcg64, 12, 24> = Hnsw::new(metric);
        Ok(HNSWIndex { searcher, hnsw })
    }
}

impl VectorIndex for HNSWIndex {
    fn insert(&mut self, data: &[f32], _id: i64) -> Result<i64, Box<dyn Error>> {
        let feature = data.to_vec();
        let id = self.hnsw.insert(feature, &mut self.searcher);
        Ok(id as i64)
    }

    fn remove(&mut self, _ids: &[i64]) -> Result<(), Box<dyn Error>> {
        todo!()
    }

    fn search(
        &mut self,
        query: &[f32],
        k: usize,
        ef_search: i32,
    ) -> Result<(Vec<i64>, Vec<f32>), Box<dyn Error>> {
        let mut neighbors = vec![
            Neighbor {
                index: !0,
                distance: !0,
            };
            k
        ];
        let result = self.hnsw.nearest(
            &query.to_vec(),
            ef_search as usize,
            &mut self.searcher,
            &mut neighbors,
        );
        if result.is_empty() {
            return Ok((Vec::new(), Vec::new()));
        }
        let mut indices: Vec<i64> = vec![-1; k];
        let mut distance: Vec<f32> = vec![f32::MAX; k];
        for (i, neighbor) in result.iter().enumerate() {
            indices[i] = neighbor.index as i64;
            distance[i] = neighbor.distance as f32;
        }
        Ok((indices, distance))
    }
}
