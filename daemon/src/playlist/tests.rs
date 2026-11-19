//! The runner against Java's PlaylistRunner behaviour (docs/TODO.md group E).
//! Fake engine, fake library, and tokio's paused clock: "wait 60 s" is instant.

use std::collections::HashSet;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use serde_json::{json, Value};
use tokio::sync::mpsc::UnboundedReceiver;

use super::*;

#[derive(Default)]
struct FakeEngine {
    applied: Mutex<Vec<(String, String)>>, // (monitor, wallpaper id)
    stopped: Mutex<Vec<String>>,
    failing: Mutex<HashSet<String>>,
}

impl FakeEngine {
    fn applied(&self) -> Vec<(String, String)> {
        self.applied.lock().unwrap().clone()
    }
    fn applied_on(&self, monitor: &str) -> Vec<String> {
        self.applied().into_iter().filter(|(m, _)| m == monitor).map(|(_, id)| id).collect()
    }
    fn stopped(&self) -> Vec<String> {
        self.stopped.lock().unwrap().clone()
    }
}

impl Launcher for FakeEngine {
    fn apply<'a>(&'a self, monitor: &'a str, id: &'a str, _: Vec<String>, _: Vec<String>) -> BoxFuture<'a, Result<u32, String>> {
        Box::pin(async move {
            if self.failing.lock().unwrap().contains(id) {
                return Err("cannot start".into());
            }
            self.applied.lock().unwrap().push((monitor.into(), id.into()));
            Ok(1)
        })
    }
    fn stop<'a>(&'a self, monitor: &'a str) -> BoxFuture<'a, ()> {
        Box::pin(async move { self.stopped.lock().unwrap().push(monitor.into()) })
    }
}

/// None = empty / unreadable library
struct FakeLibrary(Mutex<Option<HashSet<String>>>);

impl FakeLibrary {
    fn with(ids: &[&str]) -> Arc<Self> {
        Arc::new(Self(Mutex::new(Some(ids.iter().map(|s| s.to_string()).collect()))))
    }
    fn unreadable() -> Arc<Self> {
        Arc::new(Self(Mutex::new(None)))
    }
    fn unsubscribe(&self, id: &str) {
        if let Some(ids) = self.0.lock().unwrap().as_mut() {
            ids.remove(id);
        }
    }
}

impl Catalog for FakeLibrary {
    fn contains(&self, id: &str) -> Option<bool> {
        self.0.lock().unwrap().as_ref().map(|ids| ids.contains(id))
    }
}

static DIRS: AtomicUsize = AtomicUsize::new(0);

fn temp_dir() -> PathBuf {
    let n = DIRS.fetch_add(1, Ordering::SeqCst);
    let dir = std::env::temp_dir().join(format!("nyaa-playlist-test-{}-{n}", std::process::id()));
    let _ = std::fs::remove_dir_all(&dir);
    dir
}

fn doc(ids: &[&str], interval: u64, looping: bool) -> String {
    let entries: Vec<Value> = ids
        .iter()
        .map(|id| json!({ "wallpaperId": id, "argv": ["--bg", id], "env": [] }))
        .collect();
    json!({ "schema": 1, "mode": "sequential", "intervalSec": interval, "loop": looping,
            "currentIndex": 0, "entries": entries })
    .to_string()
}

struct Rig {
    core: Core,
    engine: Arc<FakeEngine>,
    events: UnboundedReceiver<Event>,
    dir: PathBuf,
}

fn rig(library: Arc<FakeLibrary>) -> Rig {
    rig_in(temp_dir(), library)
}

fn rig_in(dir: PathBuf, library: Arc<FakeLibrary>) -> Rig {
    let engine = Arc::new(FakeEngine::default());
    let (core, events) = Core::new(engine.clone(), library, dir.clone());
    Rig { core, engine, events, dir }
}

impl Rig {
    fn messages(&mut self, monitor: &str) -> Vec<String> {
        let mut out = Vec::new();
        while let Ok(event) = self.events.try_recv() {
            if let Event::Message { monitor: m, text } = event {
                if m == monitor {
                    out.push(text);
                }
            }
        }
        out
    }
    fn stored_index(&self, monitor: &str) -> u64 {
        let raw = std::fs::read_to_string(self.dir.join(format!("{monitor}.json"))).unwrap();
        serde_json::from_str::<Value>(&raw).unwrap()["currentIndex"].as_u64().unwrap()
    }
    fn stored_ids(&self, monitor: &str) -> Vec<String> {
        let raw = std::fs::read_to_string(self.dir.join(format!("{monitor}.json"))).unwrap();
        serde_json::from_str::<Value>(&raw).unwrap()["entries"]
            .as_array()
            .unwrap()
            .iter()
            .map(|e| e["wallpaperId"].as_str().unwrap().to_string())
            .collect()
    }
}

/// Let the runners react (the paused clock moves only when everything idles).
async fn settle() {
    tokio::time::sleep(Duration::from_millis(5)).await;
}

async fn wait(seconds: u64) {
    tokio::time::sleep(Duration::from_secs(seconds)).await;
    settle().await;
}

// ---- E2: Next / Prev / Reset only touch their own monitor ---------------------

#[tokio::test(start_paused = true)]
async fn next_moves_only_its_own_monitor() {
    let mut r = rig(FakeLibrary::with(&["a", "b", "c"]));
    r.core.set("A", &doc(&["a", "b", "c"], 60, true)).unwrap();
    r.core.set("B", &doc(&["a", "b", "c"], 60, true)).unwrap();
    r.core.play("A");
    r.core.play("B");
    settle().await;

    r.core.next("A");
    settle().await;
    assert_eq!(r.engine.applied_on("A"), ["a", "b"]);
    assert_eq!(r.engine.applied_on("B"), ["a"]); // untouched
    assert_eq!(r.core.state("B"), ("playing", 0));

    r.core.prev("B");
    settle().await;
    assert_eq!(r.engine.applied_on("B"), ["a", "c"]); // wraps to the last entry
    assert_eq!(r.engine.applied_on("A"), ["a", "b"]);
    assert!(r.messages("A").contains(&"Next".to_string()));
}

// ---- E3: Next / Prev while idle do nothing and leave nothing behind ----------

#[tokio::test(start_paused = true)]
async fn next_while_idle_is_ignored() {
    let r = rig(FakeLibrary::with(&["a", "b"]));
    r.core.set("A", &doc(&["a", "b"], 60, true)).unwrap();
    r.core.next("A");
    r.core.prev("A");
    settle().await;
    assert!(r.engine.applied().is_empty());

    r.core.play("A");
    settle().await;
    assert_eq!(r.engine.applied_on("A"), ["a"]); // no stale step was waiting
}

// ---- E4: Play while playing ---------------------------------------------------

#[tokio::test(start_paused = true)]
async fn play_while_playing_says_so() {
    let mut r = rig(FakeLibrary::with(&["a"]));
    r.core.set("A", &doc(&["a"], 60, true)).unwrap();
    r.core.play("A");
    settle().await;
    r.core.play("A");
    settle().await;
    assert_eq!(r.engine.applied_on("A"), ["a"]);
    let messages = r.messages("A");
    assert_eq!(messages, ["Playing", "Already playing"]);
}

// ---- E5: Resume continues without re-applying ---------------------------------

#[tokio::test(start_paused = true)]
async fn resume_does_not_reapply() {
    let mut r = rig(FakeLibrary::with(&["a", "b"]));
    r.core.set("A", &doc(&["a", "b"], 60, true)).unwrap();
    r.core.play("A");
    settle().await;
    r.core.pause("A");
    settle().await;
    assert_eq!(r.core.state("A"), ("paused", 0));

    wait(600).await; // a paused playlist has no timer
    assert_eq!(r.engine.applied_on("A"), ["a"]);

    r.core.play("A"); // Play on a paused playlist resumes it
    settle().await;
    assert_eq!(r.engine.applied_on("A"), ["a"]); // nothing re-applied
    assert_eq!(r.core.state("A"), ("playing", 0));

    wait(61).await;
    assert_eq!(r.engine.applied_on("A"), ["a", "b"]); // and the timer runs again
    let messages = r.messages("A");
    assert!(messages.contains(&"Paused".to_string()));
    assert!(messages.contains(&"Resumed".to_string()));
}

// ---- E6: the position is saved and survives a restart -------------------------

#[tokio::test(start_paused = true)]
async fn position_survives_restart() {
    let dir = temp_dir();
    let library = FakeLibrary::with(&["a", "b", "c"]);
    {
        let r = rig_in(dir.clone(), library.clone());
        r.core.set("A", &doc(&["a", "b", "c"], 60, true)).unwrap();
        r.core.play("A");
        settle().await;
        r.core.next("A");
        settle().await;
        assert_eq!(r.stored_index("A"), 1);
        r.core.stop("A");
        settle().await;
    }
    let r = rig_in(dir, library); // a new daemon reading the same files
    r.core.play("A");
    settle().await;
    assert_eq!(r.engine.applied_on("A"), ["b"]);
}

#[tokio::test(start_paused = true)]
async fn editing_while_playing_keeps_the_position() {
    let r = rig(FakeLibrary::with(&["a", "b", "c"]));
    r.core.set("A", &doc(&["a", "b", "c"], 60, true)).unwrap();
    r.core.play("A");
    settle().await;
    r.core.next("A");
    settle().await;
    // the client still thinks currentIndex is 0
    r.core.set("A", &doc(&["a", "b", "c", "d"], 60, true)).unwrap();
    assert_eq!(r.core.state("A"), ("playing", 1));
}

// ---- E7: Play prunes missing wallpapers; nothing left → "Playlist is empty" ---

#[tokio::test(start_paused = true)]
async fn play_removes_missing_wallpapers() {
    let mut r = rig(FakeLibrary::with(&["a", "c"]));
    r.core.set("A", &doc(&["a", "b", "c"], 60, true)).unwrap();
    r.core.play("A");
    settle().await;
    assert_eq!(r.stored_ids("A"), ["a", "c"]);
    let changed = std::iter::from_fn(|| r.events.try_recv().ok())
        .any(|e| e == Event::Changed { monitor: "A".into() });
    assert!(changed);

    let mut empty = rig(FakeLibrary::with(&["z"]));
    empty.core.set("A", &doc(&["a", "b"], 60, true)).unwrap();
    empty.core.play("A");
    settle().await;
    assert!(empty.engine.applied().is_empty());
    assert_eq!(empty.messages("A"), ["Playlist is empty"]);
    assert_eq!(empty.core.state("A"), ("idle", -1));
}

// ---- E8: a wallpaper that disappears while playing is skipped and removed ----

#[tokio::test(start_paused = true)]
async fn missing_wallpaper_is_skipped_while_playing() {
    let library = FakeLibrary::with(&["a", "b", "c"]);
    let r = rig(library.clone());
    r.core.set("A", &doc(&["a", "b", "c"], 60, true)).unwrap();
    r.core.play("A");
    settle().await;
    library.unsubscribe("b");
    wait(61).await;
    assert_eq!(r.engine.applied_on("A"), ["a", "c"]);
    assert_eq!(r.stored_ids("A"), ["a", "c"]);
}

#[tokio::test(start_paused = true)]
async fn failing_entry_is_skipped() {
    let r = rig(FakeLibrary::with(&["a", "b"]));
    r.engine.failing.lock().unwrap().insert("a".into());
    r.core.set("A", &doc(&["a", "b"], 60, true)).unwrap();
    r.core.play("A");
    settle().await;
    assert_eq!(r.engine.applied_on("A"), ["b"]);
    assert_eq!(r.core.state("A"), ("playing", 1));
}

// ---- E9: the end of a non-looping playlist stops playback and the wallpaper ---

#[tokio::test(start_paused = true)]
async fn next_at_end_without_loop_stops_everything() {
    let mut r = rig(FakeLibrary::with(&["a", "b"]));
    r.core.set("A", &doc(&["a", "b"], 60, false)).unwrap();
    r.core.play("A");
    settle().await;
    r.core.next("A");
    settle().await;
    r.core.next("A");
    settle().await;
    assert_eq!(r.engine.applied_on("A"), ["a", "b"]);
    assert_eq!(r.engine.stopped(), ["A"]);
    assert_eq!(r.core.state("A"), ("idle", -1));
    assert!(r.messages("A").contains(&"End of playlist".to_string()));
}

#[tokio::test(start_paused = true)]
async fn timer_at_end_without_loop_finishes() {
    let mut r = rig(FakeLibrary::with(&["a", "b"]));
    r.core.set("A", &doc(&["a", "b"], 30, false)).unwrap();
    r.core.play("A");
    wait(31).await;
    wait(31).await;
    assert_eq!(r.engine.applied_on("A"), ["a", "b"]);
    assert_eq!(r.engine.stopped(), ["A"]);
    assert!(r.messages("A").contains(&"Playlist finished".to_string()));
}

#[tokio::test(start_paused = true)]
async fn looping_playlist_wraps() {
    let r = rig(FakeLibrary::with(&["a", "b"]));
    r.core.set("A", &doc(&["a", "b"], 10, true)).unwrap();
    r.core.play("A");
    wait(11).await;
    wait(11).await;
    assert_eq!(r.engine.applied_on("A"), ["a", "b", "a"]);
}

// ---- Reset, Stop, Delete -------------------------------------------------------

#[tokio::test(start_paused = true)]
async fn reset_goes_back_to_the_first_entry() {
    let mut r = rig(FakeLibrary::with(&["a", "b", "c"]));
    r.core.set("A", &doc(&["a", "b", "c"], 60, true)).unwrap();
    r.core.play("A");
    settle().await;
    r.core.next("A");
    r.core.next("A");
    settle().await;
    r.core.reset("A");
    settle().await;
    assert_eq!(r.engine.applied_on("A"), ["a", "b", "c", "a"]);
    assert!(r.messages("A").contains(&"Reset".to_string()));
}

#[tokio::test(start_paused = true)]
async fn stop_stops_the_wallpaper_and_delete_does_not() {
    let mut r = rig(FakeLibrary::with(&["a"]));
    r.core.set("A", &doc(&["a"], 60, true)).unwrap();
    r.core.set("B", &doc(&["a"], 60, true)).unwrap();
    r.core.play("A");
    r.core.play("B");
    settle().await;

    r.core.stop("A");
    settle().await;
    assert_eq!(r.engine.stopped(), ["A"]);
    assert!(r.messages("A").contains(&"Stopped".to_string()));

    r.core.delete("B");
    settle().await;
    assert_eq!(r.engine.stopped(), ["A"]); // B's wallpaper stays
    assert_eq!(r.core.list_monitors(), ["A"]);
    assert!(!r.dir.join("B.json").exists());

    r.core.stop("never-played"); // Java: stop on an inactive playlist does nothing
    settle().await;
    assert_eq!(r.engine.stopped(), ["A"]);
}

// ---- E11: an unreadable library never wipes a playlist (Java bug) ---------------

#[tokio::test(start_paused = true)]
async fn empty_library_never_prunes() {
    let r = rig(FakeLibrary::unreadable());
    r.core.set("A", &doc(&["a", "b"], 60, true)).unwrap();
    r.core.play("A");
    settle().await;
    wait(61).await;
    assert_eq!(r.stored_ids("A"), ["a", "b"]);
    assert_eq!(r.engine.applied_on("A"), ["a", "b"]);
}

// ---- video entries play for their own length ------------------------------------

#[tokio::test(start_paused = true)]
async fn video_duration_overrides_interval() {
    let r = rig(FakeLibrary::with(&["v", "s"]));
    let document = json!({ "schema": 1, "mode": "sequential", "intervalSec": 60, "loop": true,
        "entries": [ { "wallpaperId": "v", "argv": [], "durationSec": 12.0 },
                     { "wallpaperId": "s", "argv": [] } ] });
    r.core.set("A", &document.to_string()).unwrap();
    r.core.play("A");
    wait(13).await;
    assert_eq!(r.engine.applied_on("A"), ["v", "s"]);
}

#[test]
fn random_never_repeats_the_same_entry() {
    let mut document = json!({ "mode": "random", "entries": [{}, {}, {}], "currentIndex": 1 });
    for _ in 0..50 {
        let before = current_index(&document);
        assert!(advance_doc(&mut document, 1));
        assert_ne!(current_index(&document), before);
    }
}
