use actix::{Actor, Addr, SyncArbiter, SyncContext};
use anyhow;
use diesel::{
    pg::PgConnection,
    r2d2::{ConnectionManager, Pool},
};
use num_cpus;
use structopt::StructOpt;

pub mod event;
pub mod log;
pub mod user;

#[derive(Debug, Clone, StructOpt)]
pub struct Config {
    #[structopt(
        long = "db-host",
        env = "DB_HOST",
        default_value = "localhost",
        help = "the host of postgres"
    )]
    db_host: String,
    #[structopt(
        long = "db-port",
        env = "DB_PORT",
        default_value = "5432",
        help = "the port of postgres"
    )]
    db_port: u16,
    #[structopt(
        long = "db-name",
        env = "DB_NAME",
        default_value = "bugdev",
        help = "the database name of postgres"
    )]
    db_name: String,
    #[structopt(
        long = "db-user",
        env = "DB_USER",
        default_value = "dev",
        help = "the user of postgres"
    )]
    db_user: String,
    #[structopt(
        long = "db-password",
        env = "DB_PASSWORD",
        default_value = "dev",
        help = "the password of postgres"
    )]
    db_password: String,
}

impl Config {
    pub fn build(&self) -> anyhow::Result<Addr<Database>> {
        let manager = ConnectionManager::<PgConnection>::new(self.build_dsn());
        let pg_pool = Pool::builder().build(manager)?;
        Ok(SyncArbiter::start(num_cpus::get(), move || {
            Database(pg_pool.clone())
        }))
    }

    fn build_dsn(&self) -> String {
        format!(
            "postgres://{user}:{password}@{host}:{port}/{name}",
            user = self.db_user,
            password = self.db_password,
            host = self.db_host,
            port = self.db_port,
            name = self.db_name,
        )
    }
}

pub struct Database(pub Pool<ConnectionManager<PgConnection>>);

impl Actor for Database {
    type Context = SyncContext<Self>;
}
