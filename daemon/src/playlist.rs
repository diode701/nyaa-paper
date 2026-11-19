//! Playlists: one stored document and at most one runner per monitor.
//!
//! Java equivalent: PlaylistRunner + PlaylistManager. The runner logic lives in
//! `Core`, which knows nothing about D-Bus: it starts wallpapers through a
//! `Launcher` (the engine), asks a `Catalog` (the library) whether a wallpaper
//! still exists, and reports through `Event`s. `Playlist` is the D-Bus face.
//!
//! Each monitor's runner is its own task with its own command channel, so a
//! Next on one monitor can never move another monitor's playlist.

use std::collections::HashMap;
use std::future::Future;
use std::path::PathBuf;
use std::pin::Pin;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime, UNIX_EPOCH};

use serde_json::Value;
use tokio::sync::mpsc;
use tracing::{info, warn};
use zbus::Connection;

use crate::engine::Engine;
use crate::library::Library;
use crate::{paths, OBJECT_PATH};

const IFACE: &str = "org.nyaa.Paper.Playlist";

pub type BoxFuture<'a, T> = Pin<Box<dyn Future<Output = T> + Send + 'a>>;

/// Starts and stops wallpapers. The engine in production, a recorder in tests.
pub trait Launcher: Send + Sync + 'static {
    fn apply<'a>(
        &'a self,
        monitor: &'a str,
        id: &'a str,
        argv: Vec<String>,
        env: Vec<String>,
    ) -> BoxFuture<'a, Result<u32, String>>;
    fn stop<'a>(&'a self, monitor: &'a str) -> BoxFuture<'a, ()>;
}

/// Whether a wallpaper is still subscribed. `None` means the library is empty
/// or unreadable: then nothing may be pruned (Java wiped playlists here).
pub trait Catalog: Send + Sync + 'static {
    fn contains(&self, id: &str) -> Option<bool>;
}

impl Launcher for Engine {
    fn apply<'a>(
        &'a self,
        monitor: &'a str,
        id: &'a str,
        argv: Vec<String>,
        env: Vec<String>,
    ) -> BoxFuture<'a, Result<u32, String>> {
        Box::pin(async move { self.apply_command(monitor, id, argv, env).await.map_err(|e| e.to_string()) })
    }

    fn stop<'a>(&'a self, monitor: &'a str) -> BoxFuture<'a, ()> {
        Box::pin(self.stop_monitor(monitor))
    }
}

impl Catalog for Library {
    fn contains(&self, id: &str) -> Option<bool> {
        self.contains_id(id)
    }
}

/// What the core reports. The D-Bus layer turns these into signals.
#[derive(Debug, Clone, PartialEq)]
pub enum Event {
    /// state is "idle" | "playing" | "paused"; index is -1 when idle
    State { monitor: String, state: &'static str, index: i32 },
    /// Java's statusListener texts: "Playing", "Paused", "End of playlist", ...
    Message { monitor: String, text: String },
    /// The stored document changed (edited, pruned, deleted).
    Changed { monitor: String },
}

#[derive(Debug)]
enum Cmd {
    Timer,
    Next,
    Prev,
    Reset,
    Pause,
    Resume,
    Stop,
    /// Leave without touching the wallpaper (the playlist was deleted).
    Quit,
}

struct Handle {
    tx: mpsc::UnboundedSender<Cmd>,
    paused: bool,
    generation: u64,
}

struct Inner {
    launcher: Arc<dyn Launcher>,
    catalog: Arc<dyn Catalog>,
    docs: Mutex<HashMap<String, Value>>,
    runners: Mutex<HashMap<String, Handle>>,
    generations: AtomicU64,
    events: mpsc::UnboundedSender<Event>,
    dir: PathBuf,
}

#[derive(Clone)]
pub struct Core {
    inner: Arc<Inner>,
}

// ---- document helpers --------------------------------------------------------

fn entry_count(doc: &Value) -> usize {
    doc.get("entries").and_then(Value::as_array).map(Vec::len).unwrap_or(0)
}

fn current_index(doc: &Value) -> usize {
    let len = entry_count(doc);
    let index = doc.get("currentIndex").and_then(Value::as_u64).unwrap_or(0) as usize;
    if index >= len { 0 } else { index }   // Java: out of range → 0
}

fn set_index(doc: &mut Value, index: usize) {
    doc["currentIndex"] = Value::from(index as u64);
}

fn looping(doc: &Value) -> bool {
    doc.get("loop").and_then(Value::as_bool).unwrap_or(true)
}

fn random(doc: &Value) -> bool {
    doc.get("mode").and_then(Value::as_str).map(|m| m.eq_ignore_ascii_case("random")).unwrap_or(false)
}

fn strings(value: Option<&Value>) -> Vec<String> {
    value
        .and_then(Value::as_array)
        .map(|a| a.iter().filter_map(|v| v.as_str().map(str::to_string)).collect())
        .unwrap_or_default()
}

/// Java PlaylistRunner.advance. Returns false at the end of a non-looping
/// sequential playlist.
fn advance_doc(doc: &mut Value, delta: i64) -> bool {
    let len = entry_count(doc);
    if len == 0 {
        return false;
    }
    let index = current_index(doc);
    let next = if random(doc) {
        if len == 1 {
            0
        } else {
            // a different entry each time (Java could repeat the same one)
            let nanos = SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.subsec_nanos()).unwrap_or(0);
            let pick = nanos as usize % (len - 1);
            if pick >= index { pick + 1 } else { pick }
        }
    } else {
        let candidate = index as i64 + delta;
        if candidate < 0 {
            len - 1
        } else if candidate as usize >= len {
            if !looping(doc) {
                return false;
            }
            0
        } else {
            candidate as usize
        }
    };
    set_index(doc, next);
    true
}

// ---- core --------------------------------------------------------------------

impl Core {
    pub fn new(
        launcher: Arc<dyn Launcher>,
        catalog: Arc<dyn Catalog>,
        dir: PathBuf,
    ) -> (Self, mpsc::UnboundedReceiver<Event>) {
        let (events, rx) = mpsc::unbounded_channel();
        let core = Self {
            inner: Arc::new(Inner {
                launcher,
                catalog,
                docs: Mutex::new(HashMap::new()),
                runners: Mutex::new(HashMap::new()),
                generations: AtomicU64::new(0),
                events,
                dir,
            }),
        };
        core.load_all();
        (core, rx)
    }

    fn load_all(&self) {
        let _ = std::fs::create_dir_all(&self.inner.dir);
        let Ok(entries) = std::fs::read_dir(&self.inner.dir) else { return };
        for entry in entries.flatten() {
            let name = entry.file_name().to_string_lossy().into_owned();
            let Some(monitor) = name.strip_suffix(".json") else { continue };
            match std::fs::read_to_string(entry.path()).map(|raw| serde_json::from_str::<Value>(&raw)) {
                Ok(Ok(doc)) => {
                    self.inner.docs.lock().unwrap().insert(monitor.to_string(), doc);
                }
                // Java deleted a playlist it could not read. Keep it for inspection.
                _ => warn!(%monitor, "playlist file is not valid JSON, ignored"),
            }
        }
    }

    // -- queries --

    pub fn get(&self, monitor: &str) -> String {
        self.inner.docs.lock().unwrap().get(monitor).map(Value::to_string).unwrap_or_else(|| "{}".into())
    }

    pub fn list_monitors(&self) -> Vec<String> {
        let mut monitors: Vec<String> = self.inner.docs.lock().unwrap().keys().cloned().collect();
        monitors.sort();
        monitors
    }

    /// (state, index): "idle" with -1, or "playing"/"paused" with the entry shown.
    pub fn state(&self, monitor: &str) -> (&'static str, i32) {
        state_of(&self.inner, monitor)
    }

    // -- editing --

    /// Store a document (Java: PlaylistManager.save + PlaylistRunner.reloadPlaylist).
    pub fn set(&self, monitor: &str, json: &str) -> Result<(), String> {
        let mut doc: Value = serde_json::from_str(json).map_err(|e| format!("not JSON: {e}"))?;
        if doc.get("schema").and_then(Value::as_u64) != Some(1) {
            return Err("schema must be 1".into());
        }
        let entries = doc.get("entries").and_then(Value::as_array).ok_or("entries must be an array")?;
        if entries.iter().any(|e| e.get("argv").and_then(Value::as_array).is_none()) {
            return Err("every entry needs an argv array".into());
        }

        let running = self.inner.runners.lock().unwrap().contains_key(monitor);
        {
            let mut docs = self.inner.docs.lock().unwrap();
            // A running playlist owns its position; the client's copy may be stale.
            let index = match (running, docs.get(monitor)) {
                (true, Some(old)) => current_index(old),
                _ => current_index(&doc),
            };
            let len = entry_count(&doc);
            set_index(&mut doc, if index >= len { 0 } else { index });
            docs.insert(monitor.to_string(), doc);
        }
        persist(&self.inner, monitor);
        emit(&self.inner, Event::Changed { monitor: monitor.into() });

        // reloadPlaylist: an emptied running playlist stops
        if running && self.entry_len(monitor) == 0 {
            self.stop(monitor);
        }
        Ok(())
    }

    /// Java: PlaylistManager.delete. A running runner leaves; the wallpaper stays.
    pub fn delete(&self, monitor: &str) {
        if let Some(handle) = self.inner.runners.lock().unwrap().remove(monitor) {
            let _ = handle.tx.send(Cmd::Quit);
        }
        self.inner.docs.lock().unwrap().remove(monitor);
        let path = self.inner.dir.join(format!("{monitor}.json"));
        if let Err(e) = std::fs::remove_file(&path) {
            if e.kind() != std::io::ErrorKind::NotFound {
                warn!(%monitor, %e, "cannot delete playlist file");
            }
        }
        publish(&self.inner, monitor);
        emit(&self.inner, Event::Changed { monitor: monitor.into() });
    }

    fn entry_len(&self, monitor: &str) -> usize {
        self.inner.docs.lock().unwrap().get(monitor).map(entry_count).unwrap_or(0)
    }

    // -- playback (Java PlaylistRunner's public methods) --

    pub fn play(&self, monitor: &str) {
        {
            let mut runners = self.inner.runners.lock().unwrap();
            if let Some(handle) = runners.get_mut(monitor) {
                if !handle.paused {
                    drop(runners);
                    message(&self.inner, monitor, "Already playing");
                    return;
                }
                // a paused playlist resumes instead of losing its place
                handle.paused = false;
                let _ = handle.tx.send(Cmd::Resume);
                drop(runners);
                publish(&self.inner, monitor);
                return;
            }
        }

        // cleanupMissing, but never on an empty library
        let pruned = {
            let mut docs = self.inner.docs.lock().unwrap();
            match docs.get_mut(monitor) {
                Some(doc) => {
                    let before = entry_count(doc);
                    if let Some(entries) = doc.get_mut("entries").and_then(Value::as_array_mut) {
                        entries.retain(|e| {
                            let id = e.get("wallpaperId").and_then(Value::as_str).unwrap_or("");
                            self.inner.catalog.contains(id) != Some(false)
                        });
                    }
                    let index = current_index(doc);
                    set_index(doc, index);
                    before != entry_count(doc)
                }
                None => false,
            }
        };
        if pruned {
            persist(&self.inner, monitor);
            emit(&self.inner, Event::Changed { monitor: monitor.into() });
        }
        if self.entry_len(monitor) == 0 {
            message(&self.inner, monitor, "Playlist is empty");
            return;
        }

        let generation = self.inner.generations.fetch_add(1, Ordering::SeqCst) + 1;
        let (tx, rx) = mpsc::unbounded_channel();
        self.inner
            .runners
            .lock()
            .unwrap()
            .insert(monitor.to_string(), Handle { tx, paused: false, generation });
        tokio::spawn(run(self.inner.clone(), monitor.to_string(), generation, rx));
        publish(&self.inner, monitor);
        message(&self.inner, monitor, "Playing");
    }

    pub fn pause(&self, monitor: &str) {
        let mut runners = self.inner.runners.lock().unwrap();
        if let Some(handle) = runners.get_mut(monitor) {
            if !handle.paused {
                handle.paused = true;
                let _ = handle.tx.send(Cmd::Pause);
                drop(runners);
                publish(&self.inner, monitor);
            }
        }
    }

    pub fn next(&self, monitor: &str) {
        self.send(monitor, Cmd::Next);
    }

    pub fn prev(&self, monitor: &str) {
        self.send(monitor, Cmd::Prev);
    }

    pub fn reset(&self, monitor: &str) {
        self.send(monitor, Cmd::Reset);
    }

    /// Java stop(): only acts on an active playlist, and stops its wallpaper.
    pub fn stop(&self, monitor: &str) {
        let handle = self.inner.runners.lock().unwrap().remove(monitor);
        if let Some(handle) = handle {
            let _ = handle.tx.send(Cmd::Stop);
            publish(&self.inner, monitor);
        }
    }

    /// Commands for a monitor with no runner are dropped (Java returned early).
    fn send(&self, monitor: &str, cmd: Cmd) {
        if let Some(handle) = self.inner.runners.lock().unwrap().get(monitor) {
            let _ = handle.tx.send(cmd);
        }
    }
}

fn state_of(inner: &Inner, monitor: &str) -> (&'static str, i32) {
    let paused = match inner.runners.lock().unwrap().get(monitor) {
        Some(handle) => handle.paused,
        None => return ("idle", -1),
    };
    let index = inner.docs.lock().unwrap().get(monitor).map(current_index).unwrap_or(0) as i32;
    (if paused { "paused" } else { "playing" }, index)
}

fn emit(inner: &Inner, event: Event) {
    let _ = inner.events.send(event);
}

fn message(inner: &Inner, monitor: &str, text: &str) {
    emit(inner, Event::Message { monitor: monitor.into(), text: text.into() });
}

fn publish(inner: &Inner, monitor: &str) {
    let (state, index) = state_of(inner, monitor);
    emit(inner, Event::State { monitor: monitor.into(), state, index });
}

/// Java saves after every step, so the position survives a restart.
fn persist(inner: &Inner, monitor: &str) {
    let Some(doc) = inner.docs.lock().unwrap().get(monitor).cloned() else { return };
    let _ = std::fs::create_dir_all(&inner.dir);
    if let Err(e) = std::fs::write(inner.dir.join(format!("{monitor}.json")), doc.to_string()) {
        warn!(%monitor, %e, "cannot persist playlist");
    }
}

fn advance(inner: &Inner, monitor: &str, delta: i64) -> bool {
    let moved = inner.docs.lock().unwrap().get_mut(monitor).map(|doc| advance_doc(doc, delta)).unwrap_or(false);
    if moved {
        persist(inner, monitor);
    }
    moved
}

/// getDuration: the entry's video length, else the playlist interval.
fn current_delay(inner: &Inner, monitor: &str) -> Option<Duration> {
    let docs = inner.docs.lock().unwrap();
    let doc = docs.get(monitor)?;
    let entries = doc.get("entries")?.as_array()?;
    let entry = entries.get(current_index(doc))?;
    let seconds = entry
        .get("durationSec")
        .and_then(Value::as_f64)
        .filter(|d| *d > 0.5)
        .or_else(|| doc.get("intervalSec").and_then(Value::as_f64))
        .unwrap_or(60.0);
    Some(Duration::from_secs_f64(seconds.max(0.5)))
}

/// applyEntryAndSchedule's apply half: walk forward until an entry starts,
/// removing entries whose wallpaper is gone. Err carries the message to show.
async fn apply_current(inner: &Arc<Inner>, monitor: &str) -> Result<(), String> {
    let mut attempts = 0;
    loop {
        let (id, argv, env, len) = {
            let mut docs = inner.docs.lock().unwrap();
            let doc = docs.get_mut(monitor).ok_or("Playlist is empty")?;
            let len = entry_count(doc);
            if len == 0 {
                return Err("Playlist is empty".into());
            }
            let index = current_index(doc);
            set_index(doc, index);
            let entry = &doc["entries"][index];
            (
                entry.get("wallpaperId").and_then(Value::as_str).unwrap_or("").to_string(),
                strings(entry.get("argv")),
                strings(entry.get("env")),
                len,
            )
        };

        if inner.catalog.contains(&id) == Some(false) {
            info!(%monitor, %id, "wallpaper not found, removing from playlist");
            {
                let mut docs = inner.docs.lock().unwrap();
                if let Some(doc) = docs.get_mut(monitor) {
                    let index = current_index(doc);
                    if let Some(entries) = doc.get_mut("entries").and_then(Value::as_array_mut) {
                        entries.remove(index);
                    }
                    let index = current_index(doc);
                    set_index(doc, index);
                }
            }
            persist(inner, monitor);
            emit(inner, Event::Changed { monitor: monitor.into() });
            continue; // the list shrank, so this ends
        }

        if attempts >= len {
            return Err("Stopped".into()); // every entry failed
        }
        attempts += 1;

        match inner.launcher.apply(monitor, &id, argv, env).await {
            Ok(_) => {
                info!(%monitor, %id, "playlist applied");
                persist(inner, monitor);
                return Ok(());
            }
            Err(e) => {
                warn!(%monitor, %id, %e, "playlist entry failed, advancing");
                if !advance(inner, monitor, 1) {
                    return Err("Stopped".into());
                }
            }
        }
    }
}

/// The runner ended on its own. Forget it unless a newer one replaced it.
async fn finish(inner: &Arc<Inner>, monitor: &str, generation: u64, text: &str) {
    {
        let mut runners = inner.runners.lock().unwrap();
        if runners.get(monitor).map(|h| h.generation) == Some(generation) {
            runners.remove(monitor);
        }
    }
    inner.launcher.stop(monitor).await;
    message(inner, monitor, text);
    publish(inner, monitor);
}

/// One monitor's runner. Java: the timer task plus applyEntryAndSchedule.
async fn run(inner: Arc<Inner>, monitor: String, generation: u64, mut rx: mpsc::UnboundedReceiver<Cmd>) {
    let mut paused = false;
    let mut apply_now = true;
    loop {
        if apply_now {
            if let Err(text) = apply_current(&inner, &monitor).await {
                finish(&inner, &monitor, generation, &text).await;
                return;
            }
            publish(&inner, &monitor);
        }
        apply_now = true;

        let Some(delay) = current_delay(&inner, &monitor) else {
            finish(&inner, &monitor, generation, "Playlist is empty").await;
            return;
        };

        // scheduleNext: a paused runner has no timer; resume restarts it
        let cmd = if paused {
            rx.recv().await
        } else {
            tokio::select! {
                _ = tokio::time::sleep(delay) => Some(Cmd::Timer),
                cmd = rx.recv() => cmd,
            }
        };

        match cmd {
            None | Some(Cmd::Quit) => return,
            Some(Cmd::Timer) => {
                if !advance(&inner, &monitor, 1) {
                    finish(&inner, &monitor, generation, "Playlist finished").await;
                    return;
                }
            }
            Some(Cmd::Next) => {
                if !advance(&inner, &monitor, 1) {
                    finish(&inner, &monitor, generation, "End of playlist").await;
                    return;
                }
                message(&inner, &monitor, "Next");
            }
            Some(Cmd::Prev) => {
                advance(&inner, &monitor, -1);
                message(&inner, &monitor, "Previous");
            }
            Some(Cmd::Reset) => {
                if let Some(doc) = inner.docs.lock().unwrap().get_mut(&monitor) {
                    set_index(doc, 0);
                }
                persist(&inner, &monitor);
                message(&inner, &monitor, "Reset");
            }
            Some(Cmd::Pause) => {
                paused = true;
                apply_now = false; // the wallpaper stays on screen
                persist(&inner, &monitor);
                message(&inner, &monitor, "Paused");
            }
            Some(Cmd::Resume) => {
                paused = false;
                apply_now = false; // resume continues, it does not re-apply
                message(&inner, &monitor, "Resumed");
            }
            Some(Cmd::Stop) => {
                inner.launcher.stop(&monitor).await;
                message(&inner, &monitor, "Stopped");
                publish(&inner, &monitor);
                return;
            }
        }
    }
}

// ---- D-Bus -------------------------------------------------------------------

#[derive(Clone)]
pub struct Playlist {
    core: Core,
}

impl Playlist {
    pub fn new(engine: Engine, library: Library) -> (Self, mpsc::UnboundedReceiver<Event>) {
        let dir = paths::state_dir().join("playlists");
        let (core, events) = Core::new(Arc::new(engine), Arc::new(library), dir);
        (Self { core }, events)
    }

    /// Forward core events as D-Bus signals.
    pub fn attach(&self, conn: &Connection, mut events: mpsc::UnboundedReceiver<Event>) {
        let conn = conn.clone();
        tokio::spawn(async move {
            while let Some(event) = events.recv().await {
                let sent = match &event {
                    Event::State { monitor, state, index } => {
                        conn.emit_signal(None::<&str>, OBJECT_PATH, IFACE, "StateChanged", &(monitor.as_str(), *state, *index)).await
                    }
                    Event::Message { monitor, text } => {
                        conn.emit_signal(None::<&str>, OBJECT_PATH, IFACE, "Message", &(monitor.as_str(), text.as_str())).await
                    }
                    Event::Changed { monitor } => {
                        conn.emit_signal(None::<&str>, OBJECT_PATH, IFACE, "PlaylistChanged", &(monitor.as_str())).await
                    }
                };
                if let Err(e) = sent {
                    warn!(%e, "could not emit playlist signal");
                }
            }
        });
    }
}

#[zbus::interface(name = "org.nyaa.Paper.Playlist")]
impl Playlist {
    fn set(&self, monitor: String, json: String) -> zbus::fdo::Result<()> {
        self.core.set(&monitor, &json).map_err(zbus::fdo::Error::InvalidArgs)
    }

    fn get(&self, monitor: String) -> String {
        self.core.get(&monitor)
    }

    fn list_monitors(&self) -> Vec<String> {
        self.core.list_monitors()
    }

    fn delete(&self, monitor: String) {
        self.core.delete(&monitor);
    }

    fn play(&self, monitor: String) {
        self.core.play(&monitor);
    }

    fn pause(&self, monitor: String) {
        self.core.pause(&monitor);
    }

    fn next(&self, monitor: String) {
        self.core.next(&monitor);
    }

    fn prev(&self, monitor: String) {
        self.core.prev(&monitor);
    }

    fn reset(&self, monitor: String) {
        self.core.reset(&monitor);
    }

    fn stop(&self, monitor: String) {
        self.core.stop(&monitor);
    }

    fn state(&self, monitor: String) -> (String, i32) {
        let (state, index) = self.core.state(&monitor);
        (state.to_string(), index)
    }

    #[zbus(signal)]
    async fn state_changed(
        emitter: &zbus::object_server::SignalEmitter<'_>,
        monitor: &str,
        state: &str,
        index: i32,
    ) -> zbus::Result<()>;

    #[zbus(signal)]
    async fn playlist_changed(emitter: &zbus::object_server::SignalEmitter<'_>, monitor: &str) -> zbus::Result<()>;

    #[zbus(signal)]
    async fn message(emitter: &zbus::object_server::SignalEmitter<'_>, monitor: &str, text: &str) -> zbus::Result<()>;
}

#[cfg(test)]
mod tests;
