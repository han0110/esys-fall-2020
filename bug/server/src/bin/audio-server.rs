use actix::{Actor, AsyncContext, Context, Message, StreamHandler};
use actix_rt;
use anyhow::anyhow;
use audio::{Audio, Spec};
use bincode::deserialize;
use bytes::BytesMut;
use futures_util::stream::StreamExt;
use hound::WavWriter;
use std::{
    collections::HashMap,
    fs::{create_dir_all, File},
    io::BufWriter,
    net::{Ipv4Addr, SocketAddr},
    path::{Path, PathBuf},
};
use structopt::StructOpt;
use tokio::net::UdpSocket;
use tokio_util::{codec::BytesCodec, udp::UdpFramed};
use uuid::Uuid;

#[derive(Debug, Clone, StructOpt)]
pub struct Args {
    #[structopt(
        long = "wav-directory",
        default_value = "./wav",
        help = "the directory to store collected wav file"
    )]
    pub wav_directory: String,
}

#[actix_rt::main]
async fn main() -> Result<(), anyhow::Error> {
    let args = Args::from_args();

    let addr: SocketAddr = SocketAddr::new(Ipv4Addr::UNSPECIFIED.into(), 4000);
    let sock = UdpSocket::bind(&addr).await.unwrap();
    println!("Started udp server on: {:?}", sock.local_addr().unwrap());

    let (_, stream) = UdpFramed::new(sock, BytesCodec::new()).split();
    AudioWriterActor::create(|ctx| {
        ctx.add_stream(
            stream.filter_map(|item: std::io::Result<(BytesMut, SocketAddr)>| async {
                item.map(move |(data, addr)| UdpPacket(data, addr)).ok()
            }),
        );
        AudioWriterActor::new(args.wav_directory.into()).unwrap()
    });

    actix_rt::Arbiter::local_join().await;

    Ok(())
}

struct AudioWriter {
    wav_directory: PathBuf,
    spec: Spec,
    writers: HashMap<Uuid, (u32, WavWriter<BufWriter<File>>)>,
}

impl AudioWriter {
    fn new(wav_directory: PathBuf, spec: Spec) -> Self {
        Self {
            wav_directory,
            spec,
            writers: HashMap::new(),
        }
    }

    fn new_writer(&self, id: Uuid) -> anyhow::Result<WavWriter<BufWriter<File>>> {
        let mut filepath = self.wav_directory.clone();
        filepath.push(id.to_string());
        filepath.set_extension("wav");
        Ok(WavWriter::create(filepath, self.spec.into())?)
    }

    fn write(&mut self, buf: BytesMut) -> anyhow::Result<()> {
        match self.spec.sample_format {
            audio::SampleFormat::U16 => {
                match deserialize(&buf[..]).unwrap() {
                    Audio::WavChunk::<u16>(wav_chunk) => {
                        if !self.writers.contains_key(&wav_chunk.id) {
                            self.writers
                                .insert(wav_chunk.id, (0, self.new_writer(wav_chunk.id)?));
                        }

                        let (seq, writer) = self.writers.get_mut(&wav_chunk.id).unwrap();
                        if *seq < wav_chunk.seq {
                            for &sample in &wav_chunk.payload.0[..] {
                                writer.write_sample(sample as i16).unwrap();
                            }
                            *seq = wav_chunk.seq;
                        }

                        println!("wav chunk {} seq {}", wav_chunk.id, seq);
                    }
                    Audio::WavEnd { id } => {
                        if let Some((_, writer)) = self.writers.remove(&id) {
                            writer.finalize().unwrap();
                        }

                        println!("wav end {}", id);
                    }
                    _ => {}
                };
            }
            audio::SampleFormat::I16 => {
                match deserialize(&buf[..]).unwrap() {
                    Audio::WavChunk::<i16>(wav_chunk) => {
                        if !self.writers.contains_key(&wav_chunk.id) {
                            self.writers
                                .insert(wav_chunk.id, (0, self.new_writer(wav_chunk.id)?));
                        }

                        let (seq, writer) = self.writers.get_mut(&wav_chunk.id).unwrap();
                        if *seq < wav_chunk.seq {
                            for &sample in &wav_chunk.payload.0[..] {
                                writer.write_sample(sample).unwrap();
                            }
                            *seq = wav_chunk.seq;
                        }

                        println!("wav chunk {} seq {}", wav_chunk.id, seq);
                    }
                    Audio::WavEnd { id } => {
                        if let Some((_, writer)) = self.writers.remove(&id) {
                            writer.finalize().unwrap();
                        }

                        println!("wav end {}", id);
                    }
                    _ => {}
                };
            }
            audio::SampleFormat::F32 => {
                match deserialize(&buf[..]).unwrap() {
                    Audio::WavChunk::<f32>(wav_chunk) => {
                        if !self.writers.contains_key(&wav_chunk.id) {
                            self.writers
                                .insert(wav_chunk.id, (0, self.new_writer(wav_chunk.id)?));
                        }

                        let (seq, writer) = self.writers.get_mut(&wav_chunk.id).unwrap();
                        if *seq < wav_chunk.seq {
                            for &sample in &wav_chunk.payload.0[..] {
                                writer.write_sample(sample).unwrap();
                            }
                            *seq = wav_chunk.seq;
                        }

                        println!("wav chunk {} seq {}", wav_chunk.id, seq);
                    }
                    Audio::WavEnd { id } => {
                        if let Some((_, writer)) = self.writers.remove(&id) {
                            writer.finalize().unwrap();
                        }

                        println!("wav end {}", id);
                    }
                    _ => {}
                };
            }
        };
        Ok(())
    }
}

struct AudioWriterActor {
    wav_directory: PathBuf,
    writers: HashMap<SocketAddr, AudioWriter>,
}

impl AudioWriterActor {
    fn new(wav_directory: PathBuf) -> anyhow::Result<Self> {
        if Path::exists(&wav_directory) {
            if !Path::is_dir(&wav_directory) {
                return Err(anyhow!(
                    "file with same name of `wav_directory` already exists"
                ));
            }
        } else {
            create_dir_all(&wav_directory)?;
        }
        Ok(Self {
            wav_directory,
            writers: HashMap::new(),
        })
    }
}

impl Actor for AudioWriterActor {
    type Context = Context<Self>;

    fn started(&mut self, _ctx: &mut Context<Self>) {
        println!("AudioWriterActor is alive");
    }

    fn stopped(&mut self, _ctx: &mut Context<Self>) {
        println!("AudioWriterActor is stopped");
    }
}

#[derive(Message)]
#[rtype(result = "()")]
struct UdpPacket(BytesMut, SocketAddr);

impl StreamHandler<UdpPacket> for AudioWriterActor {
    fn handle(&mut self, udp_packet: UdpPacket, _: &mut Context<Self>) {
        let UdpPacket(buf, client_addr) = udp_packet;
        match self.writers.get_mut(&client_addr) {
            Some(writer) => {
                writer.write(buf).unwrap();
            }
            None => match deserialize(&buf[..]).unwrap() {
                Audio::<f32>::Spec(spec) => {
                    self.writers.insert(
                        client_addr,
                        AudioWriter::new(self.wav_directory.clone(), spec),
                    );
                }
                _ => {}
            },
        }
    }
}

impl actix::io::WriteHandler<std::io::Error> for AudioWriterActor {}
