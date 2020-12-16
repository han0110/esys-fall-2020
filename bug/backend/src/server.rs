use crate::actor::db::Database;
use actix::Addr;
use actix_web::{dev::Server, App, HttpServer};
use std::net::{Ipv4Addr, SocketAddr};
use structopt::StructOpt;

#[derive(Debug, Clone, StructOpt)]
pub struct Config {
    #[structopt(
        long = "server-port",
        env = "SERVER_PORT",
        default_value = "3001",
        help = "the port for server to listen to"
    )]
    pub server_port: u16,
    #[structopt(
        long = "server-enable-graphiql",
        env = "SERVER_ENABLE_GRAPHIQL",
        help = "whether to enable gql playgrond"
    )]
    pub server_enable_graphiql: bool,
}

impl Config {
    pub fn build(&self, database_addr: Addr<Database>) -> anyhow::Result<Server> {
        let addr = SocketAddr::new(Ipv4Addr::UNSPECIFIED.into(), self.server_port);
        Ok(
            HttpServer::new(move || App::new().data(database_addr.clone()))
                .bind(addr)?
                .run(),
        )
    }
}
