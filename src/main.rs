use http_server::init_router;
use index_factory::init_index;
use tracing::{error, info, Level};
use tracing_subscriber::FmtSubscriber;

pub mod constant;
pub mod errors;
pub mod http_server;
pub mod index_factory;
pub mod indexes;
pub mod vector_database;

#[macro_use]
extern crate lazy_static;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let subscriber = FmtSubscriber::builder()
        .with_max_level(Level::TRACE)
        .finish();

    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    info!("vdb start");

    let dim: u32 = 1;
    let num_data: u32 = 1000;

    match init_index(
        index_factory::IndexType::FLAT,
        dim,
        num_data,
        index_factory::MetricType::L2,
    ) {
        Ok(_) => {}
        Err(e) => {
            error!("Initialze Flat Index error: {}", e);
        }
    }

    match init_index(
        index_factory::IndexType::HNSW,
        dim,
        num_data,
        index_factory::MetricType::L2,
    ) {
        Ok(_) => {}
        Err(e) => {
            error!("Initialze Flat Index error: {}", e);
        }
    }

    let app = init_router();

    let listener = tokio::net::TcpListener::bind("127.0.0.1:8080")
        .await
        .unwrap();
    axum::serve(listener, app).await.unwrap();
    Ok(())
}
