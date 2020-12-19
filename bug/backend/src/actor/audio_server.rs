use crate::{
    actor::{
        audio_handler::{AudioBytes, AudioHandler},
        db::Database,
    },
    signal::{Signal, SignalStream},
};
use actix::{
    io::{SinkWrite, WriteHandler},
    Actor, ActorContext, Addr, AsyncContext, Context, Handler, Message, Running, StreamHandler,
};
use anyhow::anyhow;
use audio::Audio;
use bincode::deserialize;
use bytes::{Bytes, BytesMut};
use futures::stream::{SplitSink, StreamExt};
use std::{
    collections::HashMap,
    fs::create_dir_all,
    io,
    net::{Ipv4Addr, SocketAddr},
    path::{Path, PathBuf},
};
use structopt::StructOpt;
use tokio::net::UdpSocket;
use tokio_util::{codec::BytesCodec, udp::UdpFramed};

#[derive(Debug, Clone, StructOpt)]
pub struct Config {
    #[structopt(
        long = "audio-server-port",
        env = "AUDIO_SERVER_PORT",
        default_value = "3003",
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
            ctx.add_message_stream(SignalStream::new());
            ctx.add_stream(stream.filter_map(
                |item: std::io::Result<(BytesMut, SocketAddr)>| async {
                    item.map(move |(data, addr)| UdpStream(data, addr)).ok()
                },
            ));
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
    writers: HashMap<SocketAddr, Addr<AudioHandler>>,
}

impl Actor for AudioServer {
    type Context = Context<Self>;

    fn started(&mut self, _: &mut Context<Self>) {
        println!("audio server listen on port {}", self.port);
    }

    fn stopping(&mut self, _: &mut Self::Context) -> Running {
        println!("audio server is stopping");
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
            Some(writer) => writer.do_send(AudioBytes(buf)),
            None => match deserialize(&buf[..]) {
                Ok(Audio::<f32>::Spec(spec)) => {
                    let writer = AudioHandler::new(
                        self.wav_directory.clone(),
                        self.database_addr.clone(),
                        spec,
                    )
                    .start();
                    self.writers.insert(client_addr, writer);
                }
                Ok(_) => {
                    // Send handshake to request spec of client
                    self.sink.write((
                        bincode::serialize(&audio::Audio::<f32>::Handshake)
                            .unwrap()
                            .into(),
                        client_addr,
                    ));
                }
                Err(_) => {
                    // TODO: warning
                }
            },
        }
    }
}

impl WriteHandler<io::Error> for AudioServer {
    fn error(&mut self, _: io::Error, _: &mut Self::Context) -> Running {
        // TODO: warning
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
