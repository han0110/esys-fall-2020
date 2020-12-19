use crate::actor::db::Database;
use actix::{
    io::{FramedWrite, WriteHandler},
    Actor, Addr, Context, Running, StreamHandler,
};
use bytes::Bytes;
use event::{self, Event, EventKind};
use tokio::io::WriteHalf;
use tokio::net::TcpStream;
use tokio_util::codec::BytesCodec;

pub struct EventHandler {
    write: FramedWrite<Bytes, WriteHalf<TcpStream>, BytesCodec>,
    database_addr: Addr<Database>,
}

impl EventHandler {
    const MAGIC_RESPONSE: Bytes = Bytes::from_static(&[1]);

    pub fn new(
        write: FramedWrite<Bytes, WriteHalf<TcpStream>, BytesCodec>,
        database_addr: Addr<Database>,
    ) -> Self {
        Self {
            write,
            database_addr,
        }
    }
}

impl Actor for EventHandler {
    type Context = Context<Self>;

    fn started(&mut self, _: &mut Context<Self>) {
        println!("event handler started");
    }

    fn stopping(&mut self, _: &mut Self::Context) -> Running {
        println!("event handler is stopping");
        Running::Stop
    }

    fn stopped(&mut self, _: &mut Context<Self>) {
        println!("event handler stopped");
    }
}

impl StreamHandler<Result<Event, event::Error>> for EventHandler {
    fn handle(&mut self, event: Result<Event, event::Error>, _: &mut Self::Context) {
        let event = match event {
            Ok(event) => event,
            Err(e) => {
                println!("event decoding error: {:?}", e);
                // TODO: warning
                return;
            }
        };

        // TODO: check whether is authorized

        println!("event: {:?}", event);

        match event.kind {
            EventKind::Luminosity { .. } => {
                // TODO: store event
            }
            EventKind::Position { .. } => {
                // TODO: store event
            }
        }

        self.write.write(Self::MAGIC_RESPONSE);
    }
}

impl WriteHandler<std::io::Error> for EventHandler {
    fn error(&mut self, _: std::io::Error, _: &mut Self::Context) -> Running {
        // TODO: warning
        Running::Continue
    }
}
