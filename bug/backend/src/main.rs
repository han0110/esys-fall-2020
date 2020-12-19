use actix_web::rt::{self as actix_rt};
use backend::{
    actor::{audio_server, db, event_server},
    server,
};
use structopt::StructOpt;

#[derive(Debug, Clone, StructOpt)]
pub struct Config {
    #[structopt(flatten)]
    db: db::Config,
    #[structopt(flatten)]
    audio_server: audio_server::Config,
    #[structopt(flatten)]
    event_server: event_server::Config,
    #[structopt(flatten)]
    server: server::Config,
}

#[actix_rt::main]
async fn main() -> anyhow::Result<()> {
    let config = Config::from_args();

    // create actor Database
    let database_addr = config.db.build()?;

    // create actor AudioServer
    config.audio_server.build(database_addr.clone())?;

    // create actor EventServer
    config.event_server.build(database_addr.clone())?;

    // create Server
    let server = config.server.build(database_addr.clone())?;

    // wait for stop signal and graceful shutdown
    server.await?;
    actix_rt::System::current().stop();

    Ok(())
}
