use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, UNIX_EPOCH};

use notify::{RecursiveMode, Watcher};
use serde_json::json;
use tracing::{info, warn};
use zbus::Connection;

use crate::{paths, OBJECT_PATH};

const IFACE: &str = "org.nyaa.Paper.Library";
const DEBOUNCE: Duration = Duration::from_millis(300);

#[derive(Clone, Debug)]
struct Entry {
    id: String,
    base_path: String,
    title: String,
    kind: String,
    file: String,
    mtime: i64,
}

struct Inner {
    root: Mutex<PathBuf>,
    entries: Mutex<Vec<Entry>>,
    dirty: Arc<AtomicBool>,
    conn: Mutex<Option<Connection>>,
    watcher: Mutex<Option<notify::RecommendedWatcher>>,
}

#[derive(Clone)]
pub struct Library {
    inner: Arc<Inner>,
}

impl Library {
    pub fn new() -> Self {
        let library = Self {
            inner: Arc::new(Inner {
                root: Mutex::new(paths::workshop_root()),
                entries: Mutex::new(Vec::new()),
                dirty: Arc::new(AtomicBool::new(false)),
                conn: Mutex::new(None),
                watcher: Mutex::new(None),
            }),
        };
        let total = library.rescan_now();
        info!(root = %library.inner.root.lock().unwrap().display(), total, "library scanned");
        library
    }

    pub fn attach(&self, conn: &Connection) {
        *self.inner.conn.lock().unwrap() = Some(conn.clone());
    }

    /// Points the watcher at the configured root. Called at startup and again
    /// whenever the GUI changes paths.workshop and asks for a rescan.
    fn rewatch(&self) {
        let root = self.inner.root.lock().unwrap().clone();
        let dirty = self.inner.dirty.clone();
        let mut watcher = match notify::recommended_watcher(move |result: notify::Result<notify::Event>| {
            match result {
                Ok(_) => dirty.store(true, Ordering::SeqCst),
                Err(e) => warn!(%e, "watch error"),
            }
        }) {
            Ok(watcher) => watcher,
            Err(e) => {
                warn!(%e, "cannot create filesystem watcher");
                return;
            }
        };

        if let Err(e) = watcher.watch(&root, RecursiveMode::NonRecursive) {
            warn!(root = %root.display(), %e, "cannot watch workshop folder");
            return;
        }
        *self.inner.watcher.lock().unwrap() = Some(watcher);
    }

    /// Watcher callback only raises a flag; one task turns that flag into a
    /// rescan plus a signal, so a burst of Steam writes costs one refresh.
    pub fn start_watching(&self) {
        self.rewatch();

        let inner = self.inner.clone();
        tokio::spawn(async move {
            loop {
                tokio::time::sleep(DEBOUNCE).await;
                if !inner.dirty.swap(false, Ordering::SeqCst) {
                    continue;
                }
                let root = inner.root.lock().unwrap().clone();
                let entries = scan(&root);
                let total = entries.len() as u32;
                *inner.entries.lock().unwrap() = entries;
                info!(total, "library changed");

                let conn = inner.conn.lock().unwrap().clone();
                if let Some(conn) = conn {
                    if let Err(e) = conn
                        .emit_signal(None::<&str>, OBJECT_PATH, IFACE, "LibraryChanged", &())
                        .await
                    {
                        warn!(%e, "could not emit LibraryChanged");
                    }
                }
            }
        });
    }

    fn rescan_now(&self) -> u32 {
        let root = self.inner.root.lock().unwrap().clone();
        let entries = scan(&root);
        let total = entries.len() as u32;
        *self.inner.entries.lock().unwrap() = entries;
        total
    }
}

#[zbus::interface(name = "org.nyaa.Paper.Library")]
impl Library {
    /// JSON array, because qdbusxml2cpp cannot marshal a struct array without
    /// metatype glue. See interfaces/org.nyaa.Paper.Library.xml.
    fn list(&self, offset: u32, limit: u32) -> String {
        let entries = self.inner.entries.lock().unwrap();
        let items: Vec<_> = entries
            .iter()
            .skip(offset as usize)
            .take(limit as usize)
            .map(|entry| {
                json!({
                    "id": entry.id,
                    "basePath": entry.base_path,
                    "title": entry.title,
                    "type": entry.kind,
                    "file": entry.file,
                    "mtime": entry.mtime,
                })
            })
            .collect();
        serde_json::to_string(&items).unwrap_or_else(|_| "[]".to_string())
    }

    fn count(&self) -> u32 {
        self.inner.entries.lock().unwrap().len() as u32
    }

    fn rescan(&self) -> u32 {
        // The GUI may have changed paths.workshop; pick it up without a restart.
        let configured = paths::workshop_root();
        {
            let mut root = self.inner.root.lock().unwrap();
            if *root != configured {
                info!(root = %configured.display(), "workshop root changed");
                *root = configured;
            }
        }
        self.rewatch();
        self.rescan_now()
    }

    #[zbus(signal)]
    async fn library_changed(
        emitter: &zbus::object_server::SignalEmitter<'_>,
    ) -> zbus::Result<()>;
}

fn scan(root: &Path) -> Vec<Entry> {
    let mut out = Vec::new();

    let dir = match std::fs::read_dir(root) {
        Ok(dir) => dir,
        Err(e) => {
            warn!(root = %root.display(), %e, "cannot read workshop folder");
            return out;
        }
    };

    for entry in dir.flatten() {
        let path = entry.path();
        if !path.is_dir() {
            continue;
        }

        let id = entry.file_name().to_string_lossy().into_owned();
        let project = path.join("project.json");

        let (title, kind, file) = match std::fs::read_to_string(&project) {
            Ok(raw) => match serde_json::from_str::<serde_json::Value>(&raw) {
                Ok(value) => (
                    value.get("title").and_then(|v| v.as_str()).unwrap_or(id.as_str()).to_string(),
                    value.get("type").and_then(|v| v.as_str()).unwrap_or("").to_string(),
                    value.get("file").and_then(|v| v.as_str()).unwrap_or("").to_string(),
                ),
                Err(_) => (id.clone(), String::new(), String::new()),
            },
            Err(_) => (id.clone(), String::new(), String::new()),
        };

        let mtime = std::fs::metadata(&project)
            .and_then(|meta| meta.modified())
            .ok()
            .and_then(|time| time.duration_since(UNIX_EPOCH).ok())
            .map(|d| d.as_millis() as i64)
            .unwrap_or(0);

        out.push(Entry {
            id,
            base_path: path.to_string_lossy().into_owned(),
            title,
            kind,
            file,
            mtime,
        });
    }

    out.sort_by(|a, b| a.id.cmp(&b.id));
    out
}
