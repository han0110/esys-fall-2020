use crate::{event::Event, Error};
use bincode;
use bytes::{buf::BufMutExt, BytesMut};
use tokio_util::codec::{Decoder, Encoder};

#[derive(Copy, Clone, Debug, Default)]
pub struct EventCodec;

impl Decoder for EventCodec {
    type Item = Event;
    type Error = Error;

    fn decode(&mut self, buf: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        if !buf.is_empty() {
            let event = bincode::deserialize(&buf[..]).map(Some).map_err(Into::into);
            // clear buffer to avoid infinite decode
            buf.clear();
            return event;
        } else {
            Ok(None)
        }
    }
}

impl Encoder<Event> for EventCodec {
    type Error = Error;

    fn encode(&mut self, data: Event, buf: &mut BytesMut) -> Result<(), Self::Error> {
        bincode::serialize_into(&mut buf.writer(), &data)?;
        Ok(())
    }
}
