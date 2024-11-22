use std::{
    error::Error,
    sync::{Arc, Mutex},
};

use tracing::debug;

use crate::{
    errors::IndexNotFoundError,
    index_factory::{get_index, IndexType},
};

pub struct VectorDatabase {}

impl VectorDatabase {
    fn search(
        &self,
        query: Vec<f32>,
        k: usize,
        index_type: IndexType,
        _ef_search: i32,
    ) -> Result<(Vec<i64>, Vec<f32>), Box<dyn Error>> {
        match get_index(index_type) {
            Some(r) => {
                let mut index = r.lock().ok().unwrap();
                let (indices, distances) = index.search(&query, k, _ef_search)?;
                debug!("Retrieved values:");

                for (id, distance) in indices.iter().zip(distances.iter()) {
                    if *id != -1 {
                        debug!("ID: {}, Distance: {}", *id, *distance);
                    } else {
                        debug!("No specific value found");
                    }
                }
                Ok((indices, distances))
            }
            None => Err(Box::new(IndexNotFoundError {
                description: "Can not find Vector Index".to_string(),
            })),
        }
    }

    fn insert(
        &self,
        vector: Vec<f32>,
        id: i64,
        index_type: IndexType,
    ) -> Result<i64, Box<dyn Error>> {
        match get_index(index_type) {
            Some(r) => {
                let mut index = r.lock().ok().unwrap();
                index.insert(&vector, id)
            }
            None => Err(Box::new(IndexNotFoundError {
                description: "Can not find Vector Index".to_string(),
            })),
        }
    }
}

lazy_static! {
    static ref VECTOR_DATABASE: Arc<Mutex<VectorDatabase>> = {
        let vector_database = VectorDatabase {};
        Arc::new(Mutex::new(vector_database))
    };
}

pub fn search(
    query: Vec<f32>,
    k: usize,
    index_type: IndexType,
    _ef_search: i32,
) -> Result<(Vec<i64>, Vec<f32>), Box<dyn Error>> {
    let vector_database = VECTOR_DATABASE.lock()?;
    vector_database.search(query, k, index_type, _ef_search)
}

pub fn insert(vector: Vec<f32>, id: i64, index_type: IndexType) -> Result<i64, Box<dyn Error>> {
    let vector_database = VECTOR_DATABASE.lock()?;
    vector_database.insert(vector, id, index_type)
}
