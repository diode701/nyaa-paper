fn todo(name: &str) -> zbus::fdo::Error {
    zbus::fdo::Error::NotSupported(format!("org.nyaa.Paper.Playlist.{name} is not implemented yet"))
}

#[derive(Clone)]
pub struct Playlist;

impl Playlist {
    pub fn new() -> Self {
        Self
    }
}

#[zbus::interface(name = "org.nyaa.Paper.Playlist")]
impl Playlist {
    fn set(&self, _monitor: String, _json: String) -> zbus::fdo::Result<()> {
        Err(todo("Set"))
    }

    fn get(&self, _monitor: String) -> zbus::fdo::Result<String> {
        Err(todo("Get"))
    }

    fn play(&self, _monitor: String) -> zbus::fdo::Result<()> {
        Err(todo("Play"))
    }

    fn pause(&self, _monitor: String) -> zbus::fdo::Result<()> {
        Err(todo("Pause"))
    }

    fn stop(&self, _monitor: String) -> zbus::fdo::Result<()> {
        Err(todo("Stop"))
    }

    fn state(&self, _monitor: String) -> zbus::fdo::Result<(String, i32)> {
        Err(todo("State"))
    }

    #[zbus(signal)]
    async fn state_changed(
        emitter: &zbus::object_server::SignalEmitter<'_>,
        monitor: &str,
        state: &str,
        index: i32,
    ) -> zbus::Result<()>;

    #[zbus(signal)]
    async fn playlist_changed(
        emitter: &zbus::object_server::SignalEmitter<'_>,
        monitor: &str,
    ) -> zbus::Result<()>;
}