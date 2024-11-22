use std::fmt;

#[derive(Debug)]
pub struct IndexNotFoundError {
    pub(crate) description: String,
}

impl std::error::Error for IndexNotFoundError {
    fn description(&self) -> &str {
        &self.description
    }
}

impl fmt::Display for IndexNotFoundError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.description)
    }
}
