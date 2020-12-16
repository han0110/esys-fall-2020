use actix::Message;
use actix_web::rt::signal::unix::{self, SignalKind};
use core::pin::Pin;
use futures::{
    stream::Stream,
    task::{Context, Poll},
};

#[allow(dead_code)]
#[derive(PartialEq, Clone, Copy, Debug, Message)]
#[rtype(result = "()")]
pub enum Signal {
    Hup,
    Int,
    Term,
    Quit,
}

pub struct SignalStream {
    streams: Vec<(Signal, unix::Signal)>,
}

impl SignalStream {
    pub fn new() -> Self {
        Self {
            streams: [
                (SignalKind::interrupt(), Signal::Int),
                (SignalKind::hangup(), Signal::Hup),
                (SignalKind::terminate(), Signal::Term),
                (SignalKind::quit(), Signal::Quit),
            ]
            .iter()
            .fold(Vec::new(), |mut streams, (stream, signal)| {
                match unix::signal(*stream) {
                    Ok(stream) => {
                        streams.push((*signal, stream));
                        streams
                    }
                    Err(_) => {
                        // TODO: Warning
                        streams
                    }
                }
            }),
        }
    }
}

impl Stream for SignalStream {
    type Item = Signal;

    fn poll_next(mut self: Pin<&mut Self>, ctx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        for idx in 0..self.streams.len() {
            loop {
                match self.streams[idx].1.poll_recv(ctx) {
                    Poll::Ready(None) => return Poll::Ready(None),
                    Poll::Pending => break,
                    Poll::Ready(Some(_)) => return Poll::Ready(Some(self.streams[idx].0)),
                }
            }
        }
        Poll::Pending
    }
}
