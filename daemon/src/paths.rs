use std::path::{Path, PathBuf};

pub fn engine_binary() -> String {
    if let Ok(p) = std::env::var("NYAA_ENGINE") {
        if !p.is_empty() {
            return p;
        }
    }

    let bundled = "/app/bin/linux-wallpaperengine";
    if Path::new(bundled).exists() {
        return bundled.to_string();
    }
    "linux-wallpaperengine".to_string()
}

#[allow(dead_code)] //***
pub fn state_dir() -> PathBuf {
    if let Ok(p) = std::env::var("NYAA_STATE_DIR") {
        return PathBuf::from(p);
    }

    match std::env::var("HOME") {
        Ok(h) => PathBuf::from(h).join(".config/nyaa-paper"),
        Err(_) => PathBuf::from("/etc/nyaa-paper"),
    }
}

