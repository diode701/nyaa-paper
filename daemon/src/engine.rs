use std::collections::HashMap;
use std::process::Stdio;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use nix::sys::signal::{kill, Signal};
use nix::unistd::Pid;
use tokio::process::{Child, Command};
use tokio::task::JoinHandle;
use tracing::{info, warn};
use zbus::Connection;
use crate::{paths, OBJECT_PATH};

const IFACE: &str = "org.nyaa.Paper.Engine";
const STOP_GRACE: Duration = Duration::from_secs(3);

// One live engine child. Java equivalent: a WallpaperInstance field set.
struct Running {
    pid: u32,
    stop_requested: Arc<AtomicBool>,
    reaper: JoinHandle<()>,
}

#[derive(Clone)]
pub struct Engine {
    inner: Arc<Inner>,
}

struct Inner {
    running: Mutex<HashMap<String, Running>>,
    conn: Mutex<Option<Connection>>,
}

impl Engine {
    pub fn new() -> Self {
        Self {
            inner: Arc::new(Inner {
                running: Mutex::new(HashMap::new()),
                conn: Mutex::new(None),
            }),
        }
    }

    pub fn attach(&self, conn: &Connection) {
        *self.inner.conn.lock().unwrap() = Some(conn.clone());
    }

    pub async fn shutdown_all(&self) {
        let all: Vec<Running> = {
            let mut map = self.inner.running.lock().unwrap();
            map.drain().map(|(_, r)| r).collect()
        };
        for running in all {
            running.stop_requested.store(true, Ordering::SeqCst);
            Self::terminate(running).await;
        }
    }

    async fn stop_one(&self, monitor: &str) {
        // Set the flag while we still hold the lock, so a child that exits on its
        // own in the same microsecond can never be reported as a crash.
        let running = {
            let mut map = self.inner.running.lock().unwrap();
            map.remove(monitor).map(|running| {
                running.stop_requested.store(true, Ordering::SeqCst);
                running
            })
        };
        let Some(running) = running else { return };
        Self::terminate(running).await;
    }

    // The Java WallpaperInstance.stop(): SIGTERM, wait up to 3 s, then
    // force-kill the whole process tree (descendants first, then the engine).
    async fn terminate(running: Running) {
        let Running { pid, reaper, .. } = running;
        if let Err(e) = kill(Pid::from_raw(pid as i32), Signal::SIGTERM) {
            warn!(pid, %e, "could not send SIGTERM");
        }
        if tokio::time::timeout(STOP_GRACE, reaper).await.is_err() {
            warn!(pid, "engine ignored SIGTERM, killing its process tree");
            kill_tree(pid as i32);
        }
    }
}

impl Engine {
    /// Stop whatever runs on this monitor. Used by the playlist runner.
    pub async fn stop_monitor(&self, monitor: &str) {
        self.stop_one(monitor).await;
    }

    /// Spawn an engine child from a fully rendered command. Used by the D-Bus
    /// Apply method and by the playlist runner.
    pub async fn apply_command(
        &self,
        monitor: &str,
        wallpaper_id: &str,
        argv: Vec<String>,
        env: Vec<String>,
    ) -> zbus::fdo::Result<u32> {
        if monitor.trim().is_empty() {
            return Err(zbus::fdo::Error::InvalidArgs(
                "monitor must not be empty".to_string(),
            ));
        }
        if argv.is_empty() {
            return Err(zbus::fdo::Error::InvalidArgs(
                "argv must not be empty".to_string(),
            ));
        }

        self.stop_one(monitor).await;

        let binary = paths::engine_binary();
        let mut cmd = Command::new(&binary);
        cmd.args(&argv).stdin(Stdio::null());
        for pair in &env {
            if let Some((key, value)) = pair.split_once('=') {
                cmd.env(key, value);
            }
        }

        let child = cmd
            .spawn()
            .map_err(|e| zbus::fdo::Error::Failed(format!("cannot start {binary}: {e}")))?;
        let pid = child
            .id()
            .ok_or_else(|| zbus::fdo::Error::Failed("child has no pid".to_string()))?;

        let stop_requested = Arc::new(AtomicBool::new(false));
        let reaper = spawn_reaper(
            self.inner.clone(),
            monitor.to_string(),
            pid,
            stop_requested.clone(),
            child,
        );

        self.inner.running.lock().unwrap().insert(
            monitor.to_string(),
            Running {
                pid,
                stop_requested,
                reaper,
            },
        );

        info!(monitor, wallpaper_id, pid, "engine started");
        Ok(pid)
    }
}

#[zbus::interface(name = "org.nyaa.Paper.Engine")]
impl Engine {
    async fn apply(
        &self,
        monitor: String,
        wallpaper_id: String,
        argv: Vec<String>,
        env: Vec<String>,
    ) -> zbus::fdo::Result<u32> {
        self.apply_command(&monitor, &wallpaper_id, argv, env).await
    }

    async fn stop(&self, monitor: String) {
        self.stop_one(&monitor).await;
    }

    async fn stop_all(&self) {
        self.shutdown_all().await;
    }

    fn list_running(&self) -> String {
        let running = self.inner.running.lock().unwrap();
        let map: HashMap<&str, u32> = running
            .iter()
            .map(|(monitor, running)| (monitor.as_str(), running.pid))
            .collect();
        serde_json::to_string(&map).unwrap_or_else(|_| "{}".to_string())
    }

    fn info(&self) -> (String, String) {
        (
            env!("CARGO_PKG_VERSION").to_string(),
            paths::engine_binary(),
        )
    }
}

fn spawn_reaper(
    inner: Arc<Inner>,
    monitor: String,
    pid: u32,
    stop_requested: Arc<AtomicBool>,
    mut child: Child,
) -> JoinHandle<()> {
    tokio::spawn(async move {
        let status = child.wait().await;
        let requested = stop_requested.load(Ordering::SeqCst);

        // Forget ourselves, but only if we are still the current child for
        // this monitor (a replace may have put a newer one in the map).
        {
            let mut map = inner.running.lock().unwrap();
            if map.get(&monitor).map(|r| r.pid) == Some(pid) {
                map.remove(&monitor);
            }
        }

        if requested {
            info!(%monitor, "engine stopped");
            return;
        }

        let code = exit_code(status);
        info!(%monitor, code, "engine exited on its own");

        let conn = inner.conn.lock().unwrap().clone();
        if let Some(conn) = conn {
            if let Err(e) = conn
                .emit_signal(
                    None::<&str>,
                    OBJECT_PATH,
                    IFACE,
                    "EngineExited",
                    &(monitor.as_str(), code),
                )
                .await
            {
                warn!(%e, "could not emit EngineExited");
            }
        }
    })
}

/// Every process below `root`, children before grandchildren. Read from
/// /proc so it works without the engine's cooperation.
fn descendants(root: i32) -> Vec<i32> {
    let mut parents: Vec<(i32, i32)> = Vec::new(); // (pid, ppid)
    if let Ok(dir) = std::fs::read_dir("/proc") {
        for entry in dir.flatten() {
            let Ok(pid) = entry.file_name().to_string_lossy().parse::<i32>() else { continue };
            let Ok(stat) = std::fs::read_to_string(entry.path().join("stat")) else { continue };
            // "pid (comm) state ppid ...": comm may contain spaces, so split after ')'
            let Some(rest) = stat.rsplit_once(')').map(|(_, rest)| rest) else { continue };
            if let Some(ppid) = rest.split_whitespace().nth(1).and_then(|p| p.parse().ok()) {
                parents.push((pid, ppid));
            }
        }
    }
    let mut found = Vec::new();
    let mut frontier = vec![root];
    while let Some(parent) = frontier.pop() {
        for &(pid, ppid) in &parents {
            if ppid == parent && !found.contains(&pid) {
                found.push(pid);
                frontier.push(pid);
            }
        }
    }
    found
}

/// Java: handle.descendants().forEach(destroyForcibly); handle.destroyForcibly().
/// The tree is collected before anything dies, because orphans get reparented.
fn kill_tree(root: i32) {
    for pid in descendants(root) {
        let _ = kill(Pid::from_raw(pid), Signal::SIGKILL);
    }
    let _ = kill(Pid::from_raw(root), Signal::SIGKILL);
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn kill_tree_takes_the_children_too() {
        // an "engine" that ignores SIGTERM and has two children
        let mut engine = std::process::Command::new("bash")
            .args(["-c", "trap '' TERM; sleep 300 & sleep 300 & wait"])
            .spawn()
            .unwrap();
        let root = engine.id() as i32;
        let mut children = Vec::new();
        for _ in 0..50 {
            children = descendants(root);
            if children.len() >= 2 {
                break;
            }
            std::thread::sleep(std::time::Duration::from_millis(20));
        }
        assert_eq!(children.len(), 2, "both sleeps should be found");

        kill_tree(root);
        engine.wait().unwrap();
        std::thread::sleep(std::time::Duration::from_millis(100));
        for pid in children {
            let alive = std::fs::read_to_string(format!("/proc/{pid}/stat"))
                .map(|s| !s.rsplit_once(')').map(|(_, r)| r.trim_start().starts_with('Z')).unwrap_or(true))
                .unwrap_or(false);
            assert!(!alive, "child {pid} survived");
        }
    }
}

// Exit code, or negated signal number when it died from a signal (-11 = SIGSEGV).
fn exit_code(status: std::io::Result<std::process::ExitStatus>) -> i32 {
    use std::os::unix::process::ExitStatusExt;
    match status {
        Ok(status) => status
            .code()
            .unwrap_or_else(|| -(status.signal().unwrap_or(0) as i32)),
        Err(_) => -1,
    }
}