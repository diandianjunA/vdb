use crate::constant::{INDEX_TYPE_FLAT, INDEX_TYPE_HNSW};
use axum::{http::StatusCode, routing::post, Json, Router};
use request_body::{InsertRequestBody, SearchRequestBody};
use serde_json::json;
use tracing::{debug, error, info};

use crate::vector_database::insert;
use crate::{index_factory::IndexType, vector_database::search};

pub mod request_body;

pub fn init_router() -> Router {
    Router::new()
        .route("/insert", post(insert_handler))
        .route("/search", post(search_handler))
}

async fn insert_handler(
    Json(payload): Json<InsertRequestBody>,
) -> (StatusCode, Json<serde_json::Value>) {
    debug!("Received insert request");
    info!("Insert request parameters: {:#?}", payload);

    let index_type = match payload.index_type.as_str() {
        INDEX_TYPE_FLAT => IndexType::FLAT,
        INDEX_TYPE_HNSW => IndexType::HNSW,
        _ => IndexType::UNKNOWN,
    };
    if index_type == IndexType::UNKNOWN {
        error!("search error: Unknown Index type");
        return (
            StatusCode::BAD_REQUEST,
            Json(json!({
                "error": "Unknown Index type"
            })),
        );
    }
    match insert(payload.vectors, payload.id, index_type) {
        Ok(id) => {
            let result = Json(json!({
                "msg": "success",
                "id": id
            }));
            (StatusCode::OK, result)
        }
        Err(e) => {
            error!("search error: {}", e);
            let error_msg = format!("{}", e);
            (
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(json!({
                    "error": error_msg
                })),
            )
        }
    }
}

async fn search_handler(
    Json(payload): Json<SearchRequestBody>,
) -> (StatusCode, Json<serde_json::Value>) {
    debug!("Received search request");
    info!("Search request parameters: {:#?}", payload);

    let index_type = match payload.index_type.as_str() {
        INDEX_TYPE_FLAT => IndexType::FLAT,
        INDEX_TYPE_HNSW => IndexType::HNSW,
        _ => IndexType::UNKNOWN,
    };
    if index_type == IndexType::UNKNOWN {
        error!("search error: Unknown Index type");
        return (
            StatusCode::BAD_REQUEST,
            Json(json!({
                "error": "Unknown Index type"
            })),
        );
    }
    match search(payload.vectors, payload.k, index_type, 0) {
        Ok((ids, distances)) => {
            let result = Json(json!({
                "vectors": ids,
                "distances": distances
            }));
            (StatusCode::OK, result)
        }
        Err(e) => {
            error!("search error: {}", e);
            let error_msg = format!("{}", e);
            (
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(json!({
                    "error": error_msg
                })),
            )
        }
    }
}
