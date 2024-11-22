use serde::{Deserialize, Serialize};

#[derive(Deserialize, Serialize, Debug)]
pub struct InsertRequestBody {
    pub vectors: Vec<f32>,
    pub id: i64,
    pub index_type: String,
}

#[derive(Deserialize, Serialize, Debug)]
pub struct SearchRequestBody {
    pub vectors: Vec<f32>,
    pub k: usize,
    pub index_type: String,
}
