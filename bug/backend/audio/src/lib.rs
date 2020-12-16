mod audio;
pub use audio::*;

mod codec;
pub use crate::codec::AudioCodec;

#[derive(Debug)]
pub enum Error {
    Io(std::io::Error),
    Codec(bincode::Error),
    Unhandled(Box<dyn std::error::Error>),
}

impl From<bincode::Error> for Error {
    fn from(error: bincode::Error) -> Self {
        Error::Codec(error)
    }
}

impl From<std::io::Error> for Error {
    fn from(error: std::io::Error) -> Self {
        Error::Io(error)
    }
}
