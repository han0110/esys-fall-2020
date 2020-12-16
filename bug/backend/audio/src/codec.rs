use bincode;
use bytes::{buf::BufMutExt, BytesMut};
use serde::{Deserialize, Serialize};
use tokio_util::codec::{Decoder, Encoder};

#[derive(Copy, Clone, Debug, Default)]
pub struct AudioCodec<T> {
    phatom: std::marker::PhantomData<T>,
}

impl<T> Decoder for AudioCodec<T>
where
    T: for<'de> Deserialize<'de> + Copy + Default,
{
    type Item = crate::audio::Audio<T>;
    type Error = crate::Error;

    fn decode(&mut self, buf: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        if !buf.is_empty() {
            Ok(Some(bincode::deserialize(&buf[..])?))
        } else {
            Ok(None)
        }
    }
}

impl<T> Encoder<crate::audio::Audio<T>> for AudioCodec<T>
where
    T: Serialize + Copy + Default,
{
    type Error = crate::Error;

    fn encode(
        &mut self,
        data: crate::audio::Audio<T>,
        buf: &mut BytesMut,
    ) -> Result<(), Self::Error> {
        bincode::serialize_into(&mut buf.writer(), &data)?;
        Ok(())
    }
}
