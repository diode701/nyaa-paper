use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, UNIX_EPOCH};

use notify::event::{AccessKind, EventKind};
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
    /// Workshop item this one is a preset of (project.json "dependency")
    dependency: String,
    /// The preset's stored property values, passed to the engine as
    /// --set-property overrides when the dependency is run
    preset: serde_json::Value,
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
                // Reading project.json during a scan produces Access events on
                // the same directory, which would rescan forever.
                Ok(event) => {
                    if !matches!(event.kind, EventKind::Access(_))
                        && !matches!(event.kind, EventKind::Access(AccessKind::Close(_)))
                    {
                        dirty.store(true, Ordering::SeqCst);
                    }
                }
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
                let ids: Vec<String> = entries.iter().map(|e| e.id.clone()).collect();
                let previous: Vec<String> = {
                    let current = inner.entries.lock().unwrap();
                    current.iter().map(|e| e.id.clone()).collect()
                };
                *inner.entries.lock().unwrap() = entries;
                if ids == previous {
                    continue; // the event was noise, say nothing
                }
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

    /// Whether a wallpaper is subscribed. None while the library is empty
    /// (workshop missing or unreadable): callers must not prune on that.
    pub fn contains_id(&self, id: &str) -> Option<bool> {
        let entries = self.inner.entries.lock().unwrap();
        if entries.is_empty() {
            return None;
        }
        Some(entries.iter().any(|entry| entry.id == id))
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
                    "dependency": entry.dependency,
                    "preset": entry.preset,
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

        // WpprRtrv: no project.json, not a wallpaper. ParseMD: unparsable
        // JSON returns null and the folder is skipped.
        let Ok(raw) = std::fs::read_to_string(&project) else { continue };
        let value = match serde_json::from_str::<serde_json::Value>(&raw) {
            Ok(value) => value,
            Err(e) => {
                warn!(%id, %e, "project.json is not valid JSON, skipped");
                continue;
            }
        };
        // Workshop "Asset" items (post-processing presets) have a project.json
        // but are not wallpapers; the engine refuses them. Java listed them.
        if value.get("category").and_then(|v| v.as_str()).is_some_and(|c| c.eq_ignore_ascii_case("asset")) {
            continue;
        }
        let text = |key: &str| value.get(key).and_then(|v| v.as_str()).map(str::to_string);
        let title = text("title").unwrap_or_else(|| id.clone());
        let mut kind = text("type").unwrap_or_default();
        let mut file = text("file").unwrap_or_default();

        // A preset (project.json with "dependency" and "preset", no "type") is
        // a saved configuration of another workshop item: report the type and
        // file of the item it belongs to, keep its own title/preview, and hand
        // its values to the engine as overrides.
        let mut dependency = String::new();
        let mut preset = serde_json::Value::Null;

        if kind.is_empty() {
            if let Some(dep) = value.get("dependency").and_then(|v| v.as_str()) {
                dependency = dep.to_string();
                preset = value.get("preset").cloned().unwrap_or(serde_json::Value::Null);

                if let Ok(raw) = std::fs::read_to_string(root.join(dep).join("project.json")) {
                    if let Ok(dep_value) = serde_json::from_str::<serde_json::Value>(&raw) {
                        kind = dep_value.get("type").and_then(|v| v.as_str()).unwrap_or_default().to_string();
                        file = dep_value.get("file").and_then(|v| v.as_str()).unwrap_or_default().to_string();
                    }
                }
            }
        }

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
            dependency,
            preset,
        });
    }

    out.sort_by(|a, b| a.id.cmp(&b.id));
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn scan_keeps_only_real_wallpapers() {
        let root = std::env::temp_dir().join(format!("nyaa-scan-test-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&root);
        for (id, project) in [
            ("100", Some(r#"{"title": "Good", "type": "scene", "file": "scene.json"}"#)),
            ("200", None),                                   // no project.json
            ("300", Some("{ not json")),                     // broken
            ("400", Some(r#"{"type": "video"}"#)),           // no title
            ("500", Some(r#"{"category": "Asset", "title": "A preset"}"#)), // not a wallpaper
            (
                "600",
                Some(r#"{"title": "Preset", "dependency": "100", "preset": {"schemecolor": "0 0 0"}}"#),
            ),
        ] {
            let dir = root.join(id);
            std::fs::create_dir_all(&dir).unwrap();
            if let Some(json) = project {
                std::fs::write(dir.join("project.json"), json).unwrap();
            }
        }
        std::fs::write(root.join("stray-file.txt"), "x").unwrap();

        let entries = scan(&root);
        let ids: Vec<&str> = entries.iter().map(|e| e.id.as_str()).collect();
        assert_eq!(ids, ["100", "400", "600"]);
        assert_eq!(entries[0].title, "Good");
        assert_eq!(entries[0].kind, "scene");
        assert_eq!(entries[1].title, "400");
        // the preset borrows the dependency's type and keeps its own values
        assert_eq!(entries[2].title, "Preset");
        assert_eq!(entries[2].kind, "scene");
        assert_eq!(entries[2].dependency, "100");
        assert_eq!(entries[2].preset["schemecolor"], "0 0 0");
        let _ = std::fs::remove_dir_all(&root);
    }
}
