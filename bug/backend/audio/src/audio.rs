use cpal;
use hound;
use serde::{
    de::{Error, SeqAccess, Visitor},
    ser::SerializeTuple,
    Deserialize, Deserializer, Serialize, Serializer,
};
use std::convert::From;
use uuid::Uuid;

pub const WAV_CHUNK_PAYLOAD_SIZE: usize = 512;

#[derive(Serialize, Deserialize)]
pub enum Audio<S>
where
    S: Copy + Default,
{
    Spec(Spec),
    WavChunk(WavChunk<S>),
    WavEnd { id: Uuid },
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Copy, Clone)]
pub struct Spec {
    pub channels: u16,
    pub sample_rate: u32,
    pub bits_per_sample: u16,
    pub sample_format: SampleFormat,
}

#[derive(Serialize, Deserialize, Debug, PartialEq, Copy, Clone)]
pub enum SampleFormat {
    U16,
    I16,
    F32,
}

#[derive(Serialize, Deserialize)]
pub struct WavChunk<S>
where
    S: Copy + Default,
{
    pub id: Uuid,
    pub seq: u32,
    pub size: u8,
    pub payload: WavChunkPayload<S>,
}

pub struct WavChunkPayload<S>(pub [S; WAV_CHUNK_PAYLOAD_SIZE]);

impl<S> WavChunkPayload<S>
where
    S: Default + Copy,
{
    pub fn default() -> Self {
        Self([S::default(); WAV_CHUNK_PAYLOAD_SIZE])
    }
}

impl From<cpal::SupportedStreamConfig> for Spec {
    fn from(config: cpal::SupportedStreamConfig) -> Self {
        Self {
            channels: config.channels(),
            sample_rate: config.sample_rate().0,
            bits_per_sample: (config.sample_format().sample_size() * 8) as u16,
            sample_format: config.sample_format().into(),
        }
    }
}

impl From<Spec> for hound::WavSpec {
    fn from(spec: Spec) -> Self {
        Self {
            channels: spec.channels,
            sample_rate: spec.sample_rate,
            bits_per_sample: spec.bits_per_sample,
            sample_format: spec.sample_format.into(),
        }
    }
}

impl From<cpal::SampleFormat> for SampleFormat {
    fn from(sample_format: cpal::SampleFormat) -> Self {
        match sample_format {
            cpal::SampleFormat::U16 => SampleFormat::U16,
            cpal::SampleFormat::I16 => SampleFormat::I16,
            cpal::SampleFormat::F32 => SampleFormat::F32,
        }
    }
}

impl From<SampleFormat> for hound::SampleFormat {
    fn from(sample_format: SampleFormat) -> Self {
        match sample_format {
            SampleFormat::U16 => hound::SampleFormat::Int,
            SampleFormat::I16 => hound::SampleFormat::Int,
            SampleFormat::F32 => hound::SampleFormat::Float,
        }
    }
}

impl<S> Serialize for WavChunkPayload<S>
where
    S: Serialize,
{
    fn serialize<SE>(&self, serializer: SE) -> Result<SE::Ok, SE::Error>
    where
        SE: Serializer,
    {
        let mut seq = serializer.serialize_tuple(self.0.len())?;
        for e in &self.0[..] {
            seq.serialize_element(e)?;
        }
        seq.end()
    }
}

impl<'de, S> Deserialize<'de> for WavChunkPayload<S>
where
    S: Default + Copy + Deserialize<'de>,
{
    fn deserialize<D>(deserializer: D) -> Result<WavChunkPayload<S>, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct ArrayVisitor<S> {
            element: std::marker::PhantomData<S>,
        }

        impl<'de, S> Visitor<'de> for ArrayVisitor<S>
        where
            S: Default + Copy + Deserialize<'de>,
        {
            type Value = WavChunkPayload<S>;

            fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
                write!(formatter, "an array of length {}", WAV_CHUNK_PAYLOAD_SIZE)
            }

            fn visit_seq<A>(self, mut seq: A) -> Result<WavChunkPayload<S>, A::Error>
            where
                A: SeqAccess<'de>,
            {
                let mut payload = WavChunkPayload::<S>::default();
                for i in 0..WAV_CHUNK_PAYLOAD_SIZE {
                    payload.0[i] = seq
                        .next_element()?
                        .ok_or_else(|| Error::invalid_length(i, &self))?;
                }
                Ok(payload)
            }
        }

        let visitor = ArrayVisitor {
            element: std::marker::PhantomData,
        };

        Ok(deserializer.deserialize_tuple(WAV_CHUNK_PAYLOAD_SIZE, visitor)?)
    }
}
