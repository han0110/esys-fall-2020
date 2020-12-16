use crate::{
    actor::db::Database,
    signal::{Signal, SignalStream},
};
use actix::{
    io::{SinkWrite, WriteHandler},
    Actor, ActorContext, Addr, AsyncContext, Context, Handler, Message, Running, StreamHandler,
};
use anyhow::anyhow;
use audio::{Audio, Spec};
use bincode::deserialize;
use bytes::{Bytes, BytesMut};
use futures::stream::{SplitSink, StreamExt};
use hound::WavWriter;
use std::{
    collections::HashMap,
    convert::TryInto,
    fs::{create_dir_all, File},
    io::{self, BufWriter},
    net::{Ipv4Addr, SocketAddr},
    path::{Path, PathBuf},
};
use structopt::StructOpt;
use tokio::net::UdpSocket;
use tokio_util::{codec::BytesCodec, udp::UdpFramed};
use uuid::Uuid;

#[derive(Debug, Clone, StructOpt)]
pub struct Config {
    #[structopt(
        long = "audio-server-port",
        env = "AUDIO_SERVER_PORT",
        default_value = "3002",
        help = "the port for audio server to listen to"
    )]
    pub audio_server_port: u16,
    #[structopt(
        long = "audio-wav-directory",
        env = "AUDIO_WAV_DIRECTORY",
        default_value = "./wav",
        help = "the directory to store collected wav file"
    )]
    pub audio_wav_directory: String,
}

impl Config {
    pub fn build(&self, database_addr: Addr<Database>) -> anyhow::Result<Addr<AudioServer>> {
        let wav_directory = ensure_directory(&self.audio_wav_directory)?;

        let socket = bind_udp_socket(self.audio_server_port)?;
        let (sink, stream) = UdpFramed::new(socket, BytesCodec::new()).split();

        Ok(AudioServer::create(|ctx| {
            ctx.add_stream(stream.filter_map(
                |item: std::io::Result<(BytesMut, SocketAddr)>| async {
                    item.map(move |(data, addr)| UdpStream(data, addr)).ok()
                },
            ));
            ctx.add_message_stream(SignalStream::new());
            AudioServer {
                port: self.audio_server_port,
                wav_directory,
                database_addr,
                sink: SinkWrite::new(sink, ctx),
                writers: HashMap::new(),
            }
        }))
    }
}

type UdpSinkItem = (Bytes, SocketAddr);
type UdpSink = SplitSink<UdpFramed<BytesCodec>, UdpSinkItem>;

pub struct AudioServer {
    port: u16,
    wav_directory: PathBuf,
    database_addr: Addr<Database>,
    sink: SinkWrite<UdpSinkItem, UdpSink>,
    writers: HashMap<SocketAddr, AudioWriter>,
}

impl Actor for AudioServer {
    type Context = Context<Self>;

    fn started(&mut self, _: &mut Context<Self>) {
        println!("audio server listen on port {}", self.port);
    }

    fn stopping(&mut self, _: &mut Self::Context) -> Running {
        println!("audio server is stopping");
        // TODO: Handle unfinished writers
        Running::Stop
    }

    fn stopped(&mut self, _: &mut Context<Self>) {
        println!("audio server stopped");
    }
}

#[derive(Message)]
#[rtype(result = "()")]
struct UdpStream(BytesMut, SocketAddr);

impl StreamHandler<UdpStream> for AudioServer {
    fn handle(&mut self, udp_packet: UdpStream, _: &mut Context<Self>) {
        let UdpStream(buf, client_addr) = udp_packet;
        match self.writers.get_mut(&client_addr) {
            Some(writer) => {
                match writer.write(buf) {
                    Err(_) => {
                        // TODO: Warning
                    }
                    Ok(Some(id)) => {
                        println!("wav stored {}", id);
                        // TODO: Send InsertLog to database_addr
                        // TODO: Create AudioProcessor to process audio
                    }
                    Ok(None) => {}
                }
            }
            None => match deserialize(&buf[..]) {
                Ok(Audio::<f32>::Spec(spec)) => {
                    self.writers.insert(
                        client_addr,
                        AudioWriter::new(self.wav_directory.clone(), spec),
                    );
                }
                Ok(_) => {
                    // Send empty packet to request spec of client
                    self.sink.write((Bytes::new(), client_addr));
                }
                Err(_) => {
                    // TODO: Warning
                }
            },
        }
    }
}

impl WriteHandler<io::Error> for AudioServer {
    fn error(&mut self, _: io::Error, _: &mut Self::Context) -> Running {
        // TODO: Warning
        Running::Continue
    }
}

impl Handler<Signal> for AudioServer {
    type Result = ();

    fn handle(&mut self, _: Signal, ctx: &mut Context<Self>) -> Self::Result {
        ctx.stop();
        ()
    }
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

    fn write(&mut self, buf: BytesMut) -> anyhow::Result<Option<Uuid>> {
        match self.spec.sample_format {
            audio::SampleFormat::U16 => self.try_write::<u16, i16>(buf),
            audio::SampleFormat::I16 => self.try_write::<i16, i16>(buf),
            audio::SampleFormat::F32 => self.try_write::<f32, f32>(buf),
        }
    }

    fn try_write<AS, HS>(&mut self, buf: BytesMut) -> anyhow::Result<Option<Uuid>>
    where
        AS: Copy + Default + for<'de> serde::de::Deserialize<'de> + TryInto<HS>,
        HS: hound::Sample,
    {
        Ok(match deserialize(&buf[..])? {
            Audio::WavChunk::<AS>(wav_chunk) => {
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
                None
            }
            Audio::WavEnd { id } => {
                if let Some((_, writer)) = self.writers.remove(&id) {
                    writer.finalize()?;
                }
                Some(id)
            }
            _ => None,
        })
    }
}

fn ensure_directory(path_str: &String) -> anyhow::Result<PathBuf> {
    let path = PathBuf::from(&path_str);
    if Path::exists(&path) {
        if !Path::is_dir(&path) {
            return Err(anyhow!("file with same name `{}` already exists", path_str));
        }
    } else {
        create_dir_all(&path)?;
    }
    Ok(path)
}

fn bind_udp_socket(port: u16) -> anyhow::Result<UdpSocket> {
    let addr = SocketAddr::new(Ipv4Addr::UNSPECIFIED.into(), port);
    let sock = std::net::UdpSocket::bind(addr)?;
    Ok(UdpSocket::from_std(sock)?)
}

trait Heaviside {
    fn heaviside(&self) -> i8;
}
