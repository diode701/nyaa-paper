use std::collections::HashMap;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};
use std::time::{Duration, SystemTime, UNIX_EPOCH};

use serde_json::Value;
use tracing::{info, warn};
use tokio::sync::Notify;
use zbus::Connection;

use crate::engine::Engine;
use crate::{paths, OBJECT_PATH};

const IFACE: &str = "org.nyaa.Paper.Playlist";

#[derive(Clone, Copy, Default)]
struct State {
    active: bool,
    paused: bool,
    index: usize,
    generation: u64,
}

struct Inner {
    engine: Engine,
    pending: Mutex<HashMap<String, i32>>,
    skip: Notify,
    docs: Mutex<HashMap<String, Value>>,
    states: Mutex<HashMap<String, State>>,
    conn: Mutex<Option<Connection>>,
    dir: PathBuf,
}

#[derive(Clone)]
pub struct Playlist {
    inner: Arc<Inner>,
}

impl Playlist {
    pub fn new(engine: Engine) -> Self {
        let dir = paths::state_dir().join("playlists");
        let playlist = Self {
            inner: Arc::new(Inner {
                engine,
                docs: Mutex::new(HashMap::new()),
                pending: Mutex::new(HashMap::new()),
                skip: Notify::new(),
                states: Mutex::new(HashMap::new()),
                conn: Mutex::new(None),
                dir,
            }),
        };
        playlist.load_all();
        playlist
    }

    pub fn attach(&self, conn: &Connection) {
        *self.inner.conn.lock().unwrap() = Some(conn.clone());
    }

    fn load_all(&self) {
        let _ = std::fs::create_dir_all(&self.inner.dir);
        let Ok(entries) = std::fs::read_dir(&self.inner.dir) else { return };
        for entry in entries.flatten() {
            let name = entry.file_name().to_string_lossy().into_owned();
            let Some(monitor) = name.strip_suffix(".json") else { continue };
            if let Ok(raw) = std::fs::read_to_string(entry.path()) {
                if let Ok(doc) = serde_json::from_str::<Value>(&raw) {
                    self.inner.docs.lock().unwrap().insert(monitor.to_string(), doc);
                }
            }
        }
    }

    fn persist(&self, monitor: &str, doc: &Value) {
        let _ = std::fs::create_dir_all(&self.inner.dir);
        let path = self.inner.dir.join(format!("{monitor}.json"));
        if let Err(e) = std::fs::write(&path, doc.to_string()) {
            warn!(%monitor, %e, "cannot persist playlist");
        }
    }

    fn state_text(state: State) -> &'static str {
        if !state.active {
            "idle"
        } else if state.paused {
            "paused"
        } else {
            "playing"
        }
    }

    fn publish(&self, monitor: &str) {
        let state = *self.inner.states.lock().unwrap().get(monitor).unwrap_or(&State::default());
        let index = state.index as i32;
        let text = Self::state_text(state);
        let conn = self.inner.conn.lock().unwrap().clone();
        let monitor = monitor.to_string();
        tokio::spawn(async move {
            if let Some(conn) = conn {
                let _ = conn
                    .emit_signal(
                        None::<&str>,
                        OBJECT_PATH,
                        IFACE,
                        "StateChanged",
                        &(monitor.as_str(), text, index),
                    )
                    .await;
            }
        });
    }

    fn bump(&self, monitor: &str, active: bool, paused: bool) -> u64 {
        let mut states = self.inner.states.lock().unwrap();
        let state = states.entry(monitor.to_string()).or_default();
        state.active = active;
        state.paused = paused;
        state.generation += 1;
        state.generation
    }

    fn start_runner(&self, monitor: String) {
        let generation = {
            let mut states = self.inner.states.lock().unwrap();
            let state = states.entry(monitor.clone()).or_default();
            state.active = true;
            state.paused = false;
            state.generation += 1;
            state.generation
        };

        let inner = self.inner.clone();
        tokio::spawn(async move {
            loop {
                // apply the entry we are on, then wait for its duration
                let (entry, delay) = match current_entry(&inner, &monitor, generation) {
                    Some(pair) => pair,
                    None => break,
                };

                if let Err(e) = apply_entry(&inner, &monitor, &entry).await {
                    warn!(%monitor, %e, "playlist entry failed, advancing");
                }

                // A Next/Prev from the UI wakes the sleep instead of waiting.
                let step = tokio::select! {
                    _ = tokio::time::sleep(Duration::from_secs_f64(delay)) => 0,
                    _ = inner.skip.notified() => {
                        inner.pending.lock().unwrap().remove(&monitor).unwrap_or(0)
                    }
                };

                if step != 0 {
                    let len = {
                        let docs = inner.docs.lock().unwrap();
                        docs.get(&monitor)
                            .and_then(|doc| doc.get("entries"))
                            .and_then(|entries| entries.as_array())
                            .map(|entries| entries.len())
                            .unwrap_or(0)
                    };
                    if len > 0 {
                        let mut states = inner.states.lock().unwrap();
                        let state = states.entry(monitor.clone()).or_default();
                        state.index = if step > 0 {
                            (state.index + 1) % len
                        } else {
                            (state.index + len - 1) % len
                        };
                        drop(states);
                        publish_from(&inner, &monitor);
                        continue;
                    }
                }

                if !still_current(&inner, &monitor, generation) {
                    return;
                }

                let advanced = {
                    let mut states = inner.states.lock().unwrap();
                    let state = states.entry(monitor.clone()).or_default();
                    match next_index(&inner, &monitor, state.index) {
                        Some(next) => {
                            state.index = next;
                            true
                        }
                        None => false,
                    }
                };

                if !advanced {
                    info!(%monitor, "playlist finished");
                    let mut states = inner.states.lock().unwrap();
                    if let Some(state) = states.get_mut(&monitor) {
                        state.active = false;
                    }
                    break;
                }

                publish_from(&inner, &monitor);
            }

            // runner ended: report idle. The guard lives in its own block so it
            // can never be held across the await below.
            {
                let mut states = inner.states.lock().unwrap();
                if let Some(state) = states.get_mut(&monitor) {
                    if state.generation == generation {
                        state.active = false;
                    }
                }
            }
            let conn = inner.conn.lock().unwrap().clone();
            if let Some(conn) = conn {
                let _ = conn
                    .emit_signal(
                        None::<&str>,
                        OBJECT_PATH,
                        IFACE,
                        "StateChanged",
                        &(monitor.as_str(), "idle", -1i32),
                    )
                    .await;
            }
        });
    }
}

fn still_current(inner: &Arc<Inner>, monitor: &str, generation: u64) -> bool {
    let states = inner.states.lock().unwrap();
    match states.get(monitor) {
        Some(state) => state.generation == generation && state.active && !state.paused,
        None => false,
    }
}

fn current_entry(inner: &Arc<Inner>, monitor: &str, generation: u64) -> Option<(Value, f64)> {
    if !still_current(inner, monitor, generation) {
        return None;
    }
    let docs = inner.docs.lock().unwrap();
    let doc = docs.get(monitor)?;
    let entries = doc.get("entries")?.as_array()?;
    if entries.is_empty() {
        return None;
    }
    let index = {
        let states = inner.states.lock().unwrap();
        states.get(monitor).map(|state| state.index).unwrap_or(0)
    } % entries.len();
    let entry = entries[index].clone();
    let delay = entry
        .get("durationSec")
        .and_then(Value::as_f64)
        .filter(|d| *d > 0.5)
        .or_else(|| doc.get("intervalSec").and_then(Value::as_f64))
        .unwrap_or(60.0);
    Some((entry, delay))
}

fn next_index(inner: &Arc<Inner>, monitor: &str, index: usize) -> Option<usize> {
    let docs = inner.docs.lock().unwrap();
    let doc = docs.get(monitor)?;
    let entries = doc.get("entries")?.as_array()?;
    if entries.is_empty() {
        return None;
    }
    let random = doc
        .get("mode")
        .and_then(Value::as_str)
        .map(|m| m.eq_ignore_ascii_case("random"))
        .unwrap_or(false);

    if random {
        if entries.len() == 1 {
            return Some(0);
        }
        let nanos = SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.subsec_nanos()).unwrap_or(0);
        let mut next = (nanos as usize) % entries.len();
        if next == index {
            next = (next + 1) % entries.len();
        }
        return Some(next);
    }

    let looped = doc.get("loop").and_then(Value::as_bool).unwrap_or(true);
    let next = index + 1;
    if next >= entries.len() {
        if looped { Some(0) } else { None }
    } else {
        Some(next)
    }
}

async fn apply_entry(inner: &Arc<Inner>, monitor: &str, entry: &Value) -> zbus::fdo::Result<u32> {
    let id = entry.get("wallpaperId").and_then(Value::as_str).unwrap_or("");
    let argv: Vec<String> = entry
        .get("argv")
        .and_then(Value::as_array)
        .map(|a| a.iter().filter_map(|v| v.as_str().map(str::to_string)).collect())
        .unwrap_or_default();
    let env: Vec<String> = entry
        .get("env")
        .and_then(Value::as_array)
        .map(|a| a.iter().filter_map(|v| v.as_str().map(str::to_string)).collect())
        .unwrap_or_default();
    inner.engine.apply_command(monitor, id, argv, env).await
}

fn publish_from(inner: &Arc<Inner>, monitor: &str) {
    let state = *inner.states.lock().unwrap().get(monitor).unwrap_or(&State::default());
    let conn = inner.conn.lock().unwrap().clone();
    let monitor = monitor.to_string();
    let text = Playlist::state_text(state);
    let index = state.index as i32;
    tokio::spawn(async move {
        if let Some(conn) = conn {
            let _ = conn
                .emit_signal(
                    None::<&str>,
                    OBJECT_PATH,
                    IFACE,
                    "StateChanged",
                    &(monitor.as_str(), text, index),
                )
                .await;
        }
    });
}

#[zbus::interface(name = "org.nyaa.Paper.Playlist")]
impl Playlist {
    fn set(&self, monitor: String, json: String) -> zbus::fdo::Result<()> {
        let doc: Value = serde_json::from_str(&json)
            .map_err(|e| zbus::fdo::Error::InvalidArgs(format!("not JSON: {e}")))?;
        if doc.get("schema").and_then(Value::as_u64) != Some(1) {
            return Err(zbus::fdo::Error::InvalidArgs("schema must be 1".to_string()));
        }
        let entries = doc
            .get("entries")
            .and_then(Value::as_array)
            .ok_or_else(|| zbus::fdo::Error::InvalidArgs("entries must be an array".to_string()))?;
        if entries.iter().any(|e| e.get("argv").and_then(Value::as_array).is_none()) {
            return Err(zbus::fdo::Error::InvalidArgs(
                "every entry needs an argv array".to_string(),
            ));
        }

        self.persist(&monitor, &doc);
        self.inner.docs.lock().unwrap().insert(monitor.clone(), doc);

        let conn = self.inner.conn.lock().unwrap().clone();
        if let Some(conn) = conn {
            let monitor = monitor.clone();
            tokio::spawn(async move {
                let _ = conn
                    .emit_signal(None::<&str>, OBJECT_PATH, IFACE, "PlaylistChanged", &(monitor.as_str()))
                    .await;
            });
        }
        Ok(())
    }

    fn get(&self, monitor: String) -> String {
        self.inner
            .docs
            .lock()
            .unwrap()
            .get(&monitor)
            .map(|doc| doc.to_string())
            .unwrap_or_else(|| "{}".to_string())
    }

    fn play(&self, monitor: String) -> zbus::fdo::Result<()> {
        let has_entries = self
            .inner
            .docs
            .lock()
            .unwrap()
            .get(&monitor)
            .and_then(|doc| doc.get("entries"))
            .and_then(Value::as_array)
            .map(|entries| !entries.is_empty())
            .unwrap_or(false);
        if !has_entries {
            return Err(zbus::fdo::Error::Failed("playlist is empty".to_string()));
        }

        let paused = self
            .inner
            .states
            .lock()
            .unwrap()
            .get(&monitor)
            .map(|state| state.active && state.paused)
            .unwrap_or(false);

        if paused {
            self.bump(&monitor, true, false);
        }
        self.start_runner(monitor.clone());
        self.publish(&monitor);
        Ok(())
    }

    fn pause(&self, monitor: String) {
        self.bump(&monitor, true, true);
        self.publish(&monitor);
    }

    async fn next(&self, monitor: String) -> zbus::fdo::Result<()> {
        self.inner.pending.lock().unwrap().insert(monitor, 1);
        self.inner.skip.notify_waiters();
        Ok(())
    }

    async fn prev(&self, monitor: String) -> zbus::fdo::Result<()> {
        self.inner.pending.lock().unwrap().insert(monitor, -1);
        self.inner.skip.notify_waiters();
        Ok(())
    }

    async fn stop(&self, monitor: String) {
        self.bump(&monitor, false, false);
        self.inner.engine.stop_monitor(&monitor).await;
        self.publish(&monitor);
    }

    fn state(&self, monitor: String) -> (String, i32) {
        let state = *self.inner.states.lock().unwrap().get(&monitor).unwrap_or(&State::default());
        (Self::state_text(state).to_string(), state.index as i32)
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
