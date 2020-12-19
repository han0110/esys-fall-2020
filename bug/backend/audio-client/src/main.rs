use actix::{
    io::{SinkWrite, WriteHandler},
    Actor, ActorContext, AsyncContext, Context, Running, StreamHandler,
};
use actix_rt::signal::unix::{signal, SignalKind};
use anyhow;
use audio::{Audio, WavChunk, WavChunkPayload, WAV_CHUNK_PAYLOAD_SIZE};
use audio_client::noise_gate::{NoiseGate, Sink};
use core::task;
use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use crossbeam_channel::{bounded, select};
use crossbeam_queue::ArrayQueue;
use dasp::Sample;
use futures::{stream::poll_fn, task::Poll};
use futures_sink::Sink as SinkTrait;
use futures_util::StreamExt;
use std::{
    net::{Ipv4Addr, SocketAddr},
    sync::Arc,
    thread,
};
use structopt::StructOpt;
use tokio::net::UdpSocket;
use tokio_util::udp::UdpFramed;
use uuid::Uuid;

#[derive(Debug, Clone, StructOpt)]
pub struct Config {
    #[structopt(
        long = "threshold",
        default_value = "0.1",
        help = "the noise gate open threshold"
    )]
    pub open_threshold: f32,
    #[structopt(
        long = "release-time",
        help = "the noise gate release time in second",
        default_value = "0.5"
    )]
    pub release_time: f32,
    #[structopt(
        long = "server-address",
        help = "the audio server address to send collected audio to in ipv4",
        default_value = "127.0.0.1:3003"
    )]
    pub server_address: String,
}

#[actix_rt::main]
async fn main() -> Result<(), anyhow::Error> {
    let config = Config::from_args();

    let host = cpal::default_host();
    let input_device = host.default_input_device().unwrap();
    let input_config = input_device.default_input_config().unwrap();

    let spec = input_config.clone().into();

    let server_address = config.server_address.parse()?;
    let socket = UdpSocket::bind(SocketAddr::new(Ipv4Addr::UNSPECIFIED.into(), 0))
        .await
        .unwrap();

    let (terminate_tx, terminate_rx) = bounded(1);
    let (waker_tx, waker_rx) = bounded::<task::Waker>(1);
    let waker_thread = thread::spawn(move || loop {
        select!(
            recv(waker_rx) -> waker => {
                match waker {
                    Ok(waker) => {
                        std::thread::sleep(std::time::Duration::from_millis(1));
                        waker.wake_by_ref();
                    }
                    _ => {}
                }
            }
            recv(terminate_rx) -> _ => {
                drop(waker_rx);
                break
            }
        );
    });

    let open_threshold = config.open_threshold;
    let release_sample_count: usize =
        (input_config.sample_rate().0 as f32 * config.release_time).round() as _;

    // FIXME: Use generic way to build actor and input stream
    let input_stream = match input_config.sample_format() {
        cpal::SampleFormat::F32 => {
            let tx = Arc::new(ArrayQueue::new(input_config.sample_rate().0 as usize));
            let rx = tx.clone();
            let input_stream = input_device.build_input_stream(
                &input_config.into(),
                move |data, _: &_| write_input_data::<f32>(data, tx.clone()),
                error_callback,
            )?;
            let (sink, stream) = UdpFramed::new(socket, audio::AudioCodec::default()).split();
            AudioCollector::create(move |ctx| {
                ctx.add_stream(stream);
                ctx.add_stream(poll_fn(move |ctx| -> Poll<Option<WavChunkPayload<f32>>> {
                    if rx.len() < WAV_CHUNK_PAYLOAD_SIZE {
                        match waker_tx.send(ctx.waker().clone()) {
                            Ok(_) => {}
                            Err(_) => {
                                return Poll::Ready(None);
                            }
                        }
                        return Poll::Pending;
                    }
                    let mut payload = WavChunkPayload::<f32>::default();
                    for i in 0..WAV_CHUNK_PAYLOAD_SIZE {
                        payload.0[i] = rx.pop().unwrap();
                    }
                    Poll::Ready(Some(payload))
                }));
                AudioCollector {
                    server_address,
                    noise_gate: NoiseGate::new(open_threshold as f32, release_sample_count),
                    sink: AudioSink::new(
                        UdpSink::new(server_address, SinkWrite::new(sink, ctx)),
                        spec,
                    ),
                }
            });
            input_stream
        }
        cpal::SampleFormat::I16 => {
            let tx = Arc::new(ArrayQueue::new(input_config.sample_rate().0 as usize * 16));
            let rx = tx.clone();
            let input_stream = input_device.build_input_stream(
                &input_config.into(),
                move |data, _: &_| write_input_data::<i16>(data, tx.clone()),
                error_callback,
            )?;
            let (sink, stream) = UdpFramed::new(socket, audio::AudioCodec::default()).split();
            AudioCollector::create(|ctx| {
                ctx.add_stream(stream);
                ctx.add_stream(poll_fn(move |ctx| -> Poll<Option<WavChunkPayload<i16>>> {
                    if rx.len() < WAV_CHUNK_PAYLOAD_SIZE {
                        match waker_tx.send(ctx.waker().clone()) {
                            Ok(_) => {}
                            Err(_) => {
                                return Poll::Ready(None);
                            }
                        }
                        return Poll::Pending;
                    }
                    let mut payload = WavChunkPayload::<i16>::default();
                    for i in 0..WAV_CHUNK_PAYLOAD_SIZE {
                        payload.0[i] = rx.pop().unwrap();
                    }
                    Poll::Ready(Some(payload))
                }));
                AudioCollector {
                    server_address,
                    noise_gate: NoiseGate::new(open_threshold as i16, release_sample_count),
                    sink: AudioSink::new(
                        UdpSink::new(server_address, SinkWrite::new(sink, ctx)),
                        spec,
                    ),
                }
            });
            input_stream
        }
        cpal::SampleFormat::U16 => {
            let tx = Arc::new(ArrayQueue::new(input_config.sample_rate().0 as usize * 16));
            let rx = tx.clone();
            let input_stream = input_device.build_input_stream(
                &input_config.into(),
                move |data, _: &_| write_input_data::<u16>(data, tx.clone()),
                error_callback,
            )?;
            let (sink, stream) = UdpFramed::new(socket, audio::AudioCodec::default()).split();
            AudioCollector::create(|ctx| {
                ctx.add_stream(stream);
                ctx.add_stream(poll_fn(move |ctx| -> Poll<Option<WavChunkPayload<u16>>> {
                    if rx.len() < WAV_CHUNK_PAYLOAD_SIZE {
                        match waker_tx.send(ctx.waker().clone()) {
                            Ok(_) => {}
                            Err(_) => {
                                return Poll::Ready(None);
                            }
                        }
                        return Poll::Pending;
                    }
                    let mut payload = WavChunkPayload::<u16>::default();
                    for i in 0..WAV_CHUNK_PAYLOAD_SIZE {
                        payload.0[i] = rx.pop().unwrap();
                    }
                    Poll::Ready(Some(payload))
                }));
                AudioCollector {
                    server_address,
                    noise_gate: NoiseGate::new(open_threshold as u16, release_sample_count),
                    sink: AudioSink::new(
                        UdpSink::new(server_address, SinkWrite::new(sink, ctx)),
                        spec,
                    ),
                }
            });
            input_stream
        }
    };

    input_stream.play()?;

    signal(SignalKind::interrupt()).unwrap().recv().await;

    input_stream.pause()?;
    terminate_tx.send(())?;
    waker_thread.join().unwrap();

    actix_rt::Arbiter::local_join().await;

    Ok(())
}

type UdpSinkItem<S> = (Audio<S>, SocketAddr);

struct UdpSink<S, D>
where
    S: Sample + Copy + Default,
    D: SinkTrait<UdpSinkItem<S>> + Unpin,
{
    server_address: SocketAddr,
    sink: SinkWrite<UdpSinkItem<S>, D>,
}

impl<S, D> UdpSink<S, D>
where
    S: Sample + Copy + Default,
    D: SinkTrait<UdpSinkItem<S>> + Unpin,
{
    fn new(server_address: SocketAddr, sink: SinkWrite<UdpSinkItem<S>, D>) -> Self {
        Self {
            server_address,
            sink,
        }
    }
}

impl<S, D> AudioSinkWrite<S> for UdpSink<S, D>
where
    S: Sample + Copy + Default + 'static,
    D: SinkTrait<UdpSinkItem<S>> + Unpin + 'static,
{
    fn write(&mut self, item: Audio<S>) -> Option<Audio<S>> {
        self.sink
            .write((item, self.server_address))
            .map(|(audio, _)| audio)
    }
}

trait AudioSinkWrite<S>
where
    S: Sample + Default,
{
    fn write(&mut self, item: Audio<S>) -> Option<Audio<S>>;
}

struct AudioSink<S, D>
where
    S: Sample + Default,
    D: AudioSinkWrite<S>,
{
    inner: D,
    spec: audio::Spec,
    buf: WavChunkPayload<S>,
    buf_pos: usize,
    buf_id_seq: (Uuid, u32),
}

impl<S, D> AudioSink<S, D>
where
    S: Sample + Default,
    D: AudioSinkWrite<S>,
{
    fn new(inner: D, spec: audio::Spec) -> Self {
        Self {
            inner,
            spec,
            buf: WavChunkPayload::default(),
            buf_pos: 0,
            buf_id_seq: (Uuid::new_v4(), 0),
        }
    }

    fn write_spec(&mut self) {
        if self.inner.write(Audio::<S>::Spec(self.spec)).is_some() {
            eprintln!("udp sink buffer is full");
        }
    }

    fn write_payload(&mut self) {
        if self
            .inner
            .write(Audio::WavChunk(WavChunk {
                id: self.buf_id_seq.0,
                seq: self.buf_id_seq.1,
                size: self.buf_pos as u8,
                payload: std::mem::replace(&mut self.buf, WavChunkPayload::<S>::default()),
            }))
            .is_some()
        {
            eprintln!("udp sink buffer is full");
        }
        self.buf_pos = 0;
        self.buf_id_seq.1 += 1;
    }

    fn write_eof(&mut self) {
        if self
            .inner
            .write(audio::Audio::WavEnd {
                id: self.buf_id_seq.0,
            })
            .is_some()
        {
            eprintln!("udp sink buffer is full");
        }
        self.buf_id_seq = (Uuid::new_v4(), 0);
    }
}

impl<S, D> Sink<S> for AudioSink<S, D>
where
    S: Sample + Default,
    D: AudioSinkWrite<S>,
{
    fn record(&mut self, sample: S) {
        self.buf.0[self.buf_pos] = sample;
        self.buf_pos += 1;
        if self.buf_pos == WAV_CHUNK_PAYLOAD_SIZE {
            self.write_payload()
        }
    }

    fn end_of_transmission(&mut self) {
        if self.buf_pos != 0 {
            self.write_payload();
            self.write_eof();
        }
    }
}

struct AudioCollector<S, D>
where
    S: Sample + Default,
    D: AudioSinkWrite<S>,
{
    server_address: SocketAddr,
    noise_gate: NoiseGate<S>,
    sink: AudioSink<S, D>,
}

impl<S, D> Actor for AudioCollector<S, D>
where
    S: Sample + Default + Unpin + 'static,
    D: AudioSinkWrite<S> + Unpin + 'static,
{
    type Context = Context<Self>;

    fn started(&mut self, _: &mut Self::Context) {
        println!("collector started");
        self.sink.write_spec();
    }

    fn stopped(&mut self, _: &mut Self::Context) {
        println!("collector stopped")
    }
}

impl<S, D> StreamHandler<WavChunkPayload<S>> for AudioCollector<S, D>
where
    S: Sample + Default + Unpin + 'static,
    D: AudioSinkWrite<S> + Unpin + 'static,
{
    fn handle(&mut self, samples: WavChunkPayload<S>, _: &mut Context<Self>) {
        self.noise_gate.process(&samples.0[..], &mut self.sink);
    }

    fn finished(&mut self, ctx: &mut Self::Context) {
        self.sink.end_of_transmission();
        ctx.run_later(core::time::Duration::from_millis(10), |_, ctx| ctx.stop());
    }
}

impl<S, D> StreamHandler<Result<(audio::Audio<S>, SocketAddr), audio::Error>>
    for AudioCollector<S, D>
where
    S: Sample + Default + Unpin + 'static,
    D: AudioSinkWrite<S> + Unpin + 'static,
{
    fn handle(
        &mut self,
        audio: Result<(audio::Audio<S>, SocketAddr), audio::Error>,
        _: &mut Context<Self>,
    ) {
        let audio = match audio {
            Ok((audio, req_address)) => {
                if req_address != self.server_address {
                    // TODO: warning: unknown requester
                    return;
                }
                audio
            }
            Err(_) => {
                // TODO: warning
                return;
            }
        };

        // The only reason to receive packet from server is to request audio spec
        match audio {
            Audio::Handshake => self.sink.write_spec(),
            _ => {
                // TODO: warning
            }
        }
    }
}

impl<S, D> WriteHandler<audio::Error> for AudioCollector<S, D>
where
    S: Sample + Default + Unpin + 'static,
    D: AudioSinkWrite<S> + Unpin + 'static,
{
    fn error(&mut self, _: audio::Error, _: &mut Self::Context) -> Running {
        // TODO: warning
        Running::Continue
    }
}

fn write_input_data<S>(samples: &[S], writer: Arc<ArrayQueue<S>>)
where
    S: Sample,
{
    for &sample in samples {
        if writer.push(sample).is_err() {
            eprintln!("ring buffer too small, try allocate a larger one");
        };
    }
}

fn error_callback(err: cpal::StreamError) {
    eprintln!("an error occurred on stream: {}", err);
}
