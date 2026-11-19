use std::path::{Path, PathBuf};

use serde_json::Value;

/// The shared config file the GUI writes. Same path, same schema.
pub fn config_path() -> PathBuf {
    if let Ok(xdg) = std::env::var("XDG_CONFIG_HOME") {
        if !xdg.is_empty() {
            return PathBuf::from(xdg).join("nyaa-paper/config.json");
        }
    }
    match std::env::var("HOME") {
        Ok(home) => PathBuf::from(home).join(".config/nyaa-paper/config.json"),
        Err(_) => PathBuf::from("/tmp/nyaa-paper/config.json"),
    }
}

fn config_value() -> Option<Value> {
    let raw = std::fs::read_to_string(config_path()).ok()?;
    serde_json::from_str::<Value>(&raw).ok()
}

pub fn config_string(section: &str, key: &str) -> Option<String> {
    let value = config_value()?;
    value.get(section)?.get(key)?.as_str().map(str::to_string)
}

pub fn engine_binary() -> String {
    if let Some(configured) = config_string("paths", "engine") {
        if !configured.is_empty() {
            return configured;
        }
    }
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


pub fn workshop_root() -> PathBuf {
    if let Some(configured) = config_string("paths", "workshop") {
        if !configured.is_empty() {
            return PathBuf::from(configured);
        }
    }
    if let Ok(p) = std::env::var("NYAA_WORKSHOP") {
        if !p.is_empty() {
            return PathBuf::from(p);
        }
    }
    let home = std::env::var("HOME").unwrap_or_else(|_| "/root".to_string());
    PathBuf::from(home).join(".local/share/Steam/steamapps/workshop/content/431960")
}
