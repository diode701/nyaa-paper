mod engine;
mod library;
mod paths;
mod playlist;

use std::error::Error;
use tokio::signal::unix::SignalKind;
use tracing::info;
use tracing_subscriber::EnvFilter;
pub const BUS_NAME: &str = "org.nyaa.Paper";
pub const OBJECT_PATH: &str = "/org/nyaa/Paper";

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    tracing_subscriber::fmt()
        .with_env_filter(EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info")))
        .init();

    let engine = engine::Engine::new();
    let library = library::Library::new();
    let (playlist, playlist_events) = playlist::Playlist::new(engine.clone(), library.clone());

    //Using Three itneterfaces: One object Path.... Requesting BUS_NAME also gives us single-instance behaviour: A second daemon fails here and exits. Baisically a documented preset.

    let conn = zbus::connection::Builder::session()
        .name(BUS_NAME)
        .serve_at(OBJECT_PATH, engine.clone())
        .serve_at(OBJECT_PATH, library.clone())
        .serve_at(OBJECT_PATH, playlist.clone())
        .build()
        .await?;


    // From here on the reaper task is allowed to emit signals...

    engine.attach(&conn);
    library.attach(&conn);
    library.start_watching();
    playlist.attach(&conn, playlist_events);
    info!(%BUS_NAME, %OBJECT_PATH, "nyaa-paperd ready");

    wait_for_shutdown().await;
    info!("shutting down");
    engine.shutdown_all().await;
    Ok(())
}


async fn wait_for_shutdown() {
    use tokio::signal::unix::signal;
    let mut term = signal(SignalKind::terminate()).expect("failed to register signal handler....or Install SIGTERM handler? ;p");

    tokio::select! {
        _ = tokio::signal::ctrl_c() => {},
        _ = term.recv() => (),
    }
}