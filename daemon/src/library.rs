#[derive(Clone)]
pub struct Library;

impl Library {
    pub fn new() -> Self {
        Self
    }
}

#[zbus::interface(name = "org.nyaa.Paper.Library")]
impl Library {
    fn list(&self, _offset: u32, _limit: u32) -> Vec<(String, String, i64)> {
        Vec::new()
    }

    fn count(&self) -> u32 {
        0
    }

    fn rescan(&self) -> u32 {
        0
    }

    #[zbus(signal)]
    async fn library_changed(
        emitter: &zbus::object_server::SignalEmitter<'_>,
    ) -> zbus::Result<()>;
}
