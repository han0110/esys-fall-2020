use crate::actor::db::Database;
use actix::{Actor, Addr, Context, Handler, Message, Running};
use anyhow::anyhow;
use audio::{Audio, Spec};
use bincode::deserialize;
use bytes::BytesMut;
use hound::WavWriter;
use serde::de::Deserialize;
use std::{collections::HashMap, convert::TryInto, fs::File, io::BufWriter, path::PathBuf};
use uuid::Uuid;

pub struct AudioHandler {
    writers: HashMap<Uuid, (u32, WavWriter<BufWriter<File>>)>,
    wav_directory: PathBuf,
    spec: Spec,
    database_addr: Addr<Database>,
}

impl AudioHandler {
    pub fn new(wav_directory: PathBuf, database_addr: Addr<Database>, spec: Spec) -> Self {
        Self {
            writers: HashMap::new(),
            wav_directory,
            spec,
            database_addr,
        }
    }

    fn new_writer(&self, id: Uuid) -> anyhow::Result<WavWriter<BufWriter<File>>> {
        let mut filepath = self.wav_directory.clone();
        filepath.push(id.to_string());
        filepath.set_extension("wav");
        Ok(WavWriter::create(filepath, self.spec.into())?)
    }

    fn write<S, HS>(&mut self, buf: BytesMut) -> anyhow::Result<()>
    where
        S: Copy + Default + for<'de> Deserialize<'de> + TryInto<HS>,
        HS: hound::Sample,
    {
        let audio = deserialize(&buf[..])?;
        match audio {
            Audio::WavChunk::<S>(wav_chunk) => {
                if !self.writers.contains_key(&wav_chunk.id) {
                    self.writers
                        .insert(wav_chunk.id, (0, self.new_writer(wav_chunk.id)?));
                }

                let (seq, writer) = self.writers.get_mut(&wav_chunk.id).unwrap();
                if *seq < wav_chunk.seq {
                    for &sample in &wav_chunk.payload.0[..] {
                        writer.write_sample(
                            sample
                                .try_into()
                                .map_err(|_| anyhow!("unexpected try_into error"))?,
                        )?
                    }
                    *seq = wav_chunk.seq;
                }

                println!("wav chunk {} seq {}", wav_chunk.id, seq);
            }
            Audio::WavEnd { id } => {
                if let Some((_, writer)) = self.writers.remove(&id) {
                    writer.finalize()?;
                    // TODO: create ComposedEventHandler
                }
            }
            _ => {
                // TODO: warning: unexpected kind
            }
        };
        Ok(())
    }
}

impl Actor for AudioHandler {
    type Context = Context<Self>;

    fn started(&mut self, _: &mut Context<Self>) {
        println!("audio handler started");
    }

    fn stopping(&mut self, _: &mut Self::Context) -> Running {
        println!("audio handler is stopping");
        self.writers
            .drain()
            .into_iter()
            .for_each(|(_, (_, writer))| {
                if let Err(_) = writer.finalize() {
                    // TODO: warning
                };
            });
        Running::Stop
    }

    fn stopped(&mut self, _: &mut Context<Self>) {
        println!("audio handler stopped");
    }
}

#[derive(Message)]
#[rtype(result = "()")]
pub struct AudioBytes(pub BytesMut);

impl Handler<AudioBytes> for AudioHandler {
    type Result = ();

    fn handle(&mut self, buf: AudioBytes, _: &mut Self::Context) -> Self::Result {
        if let Err(_) = match self.spec.sample_format {
            audio::SampleFormat::U16 => self.write::<u16, i16>(buf.0),
            audio::SampleFormat::I16 => self.write::<i16, i16>(buf.0),
            audio::SampleFormat::F32 => self.write::<f32, f32>(buf.0),
        } {
            // TODO: warning: print error
        };
        ()
    }
}
