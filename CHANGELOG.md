# Changelog

All notable changes to Nyaa Paper will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.2.0]

Massive upgrade from v0.1.1. Adds wallpaper config persistence, full playlist system with video-duration detection, autostart, and a dozen parser/UI fixes.

### Added

- **Wallpaper config persistence** — settings auto-save per-monitor on apply. "Load last" button restores saved values without applying. Startup auto-restore controlled by `persistOnStart` in settings
- **Playlist system** — per-monitor playlists with sequential / random mode, configurable interval, loop toggle. Play / pause / stop / prev / next / reset
- **Mini playlist panel** — hover-triggered slide-out panel in the properties view for quick playlist control
- **Playlist manager view** — dedicated UI (`M` key from browser) with monitor list, entry reordering, thumbnails, playback controls
- **PlaylistRunner** — timer-based engine. Detects video duration via ffprobe so videos play to completion before advancing
- **VideoDurationUtil** — probes video files with ffprobe (5-second timeout, graceful fallback)
- **GlobalOptionsFactory** — centralized factory for all global options
- **`persistOnStart`** and **`autoStartOnBoot`** settings — restore wallpapers on launch and start on login
- **Launcher startup toggles** — "Restore wallpapers on start" and "Start on login" checkboxes in setup UI
- **Autostart `.desktop` entry** — launcher writes `~/.config/autostart/nyaa-paper-autostart.desktop` when enabled
- **Shutdown hook** — `Runtime.addShutdownHook` kills all wallpaper processes on SIGTERM
- **Clean tray exit** — multi-step shutdown: cancel tasks, stop watcher, stop all wallpapers, shutdown tray, exit platform
- **Stale config cleanup** — unsubscribed wallpapers get saved configs pruned on workshop scan
- **WallpaperConfig / WallpaperConfigManager** — JSON-backed per-monitor-per-wallpaper settings at `~/.config/NyaaPaper/wallpapers/`
- **Playlist / PlaylistManager** — JSON-backed per-monitor playlist files at `~/.config/NyaaPaper/playlist/`
- **`isPrePropertyFlag()`** on `Setting` — controls argument placement relative to `--set-property`
- **`getValueString()` / `setValueFromString()`** on all `Setting` subclasses — uniform serialization
- **Screenshot section** in properties view — checkbox, path field, delay spinner
- **New keyboard shortcuts** — `L` load last, `A` add to playlist, `M` playlist manager, `S` stop, `Ctrl+↑/↓` cycle monitors, digits `1–9,0` jump to monitor, `Ctrl+L` toggle mini panel
- **Editable monitor combo** — type custom monitor names directly
- **Grid cell transition tracking** — prevents flickering and memory leaks from stale FadeTransitions
- **Missing property types** — `TextInputProperty`, `FileProperty`, `DirectoryProperty`, `TextureProperty` classes

### Changed

- **`ClampingSetting` → `ClampSetting`** — LWE flag is `--clamp`, not `--clamping`
- **VolumeSetting** — mute checkbox integrated into the volume slider
- **GpuConfig** — uses `Mode` enum instead of string checks
- **WallpaperManager** — `apply()`, `stop()`, `stopAll()` are now synchronized
- **WallpaperBrowser** — auto-restores wallpapers on start if `persistOnStart` enabled. Tray collapse is default close. `hardExit()` procedure expanded
- **Builder** — calls `cleanupStale()` on `build()`
- **Settings** — `persistOnStart` (default `true`) and `autoStartOnBoot` (default `false`) added
- **dark-theme.css** — new styles for playlist panel, selected entries, status bar
- **PropertyParser** — handles all 10 types, orders by `order` field, strips HTML, graceful on missing `type`
- **Global options** — rebuilt as `GlobalOption` subclasses with proper UI controls and persistence methods

### Fixed

- **Parser NPE** — missing `type` fields no longer crash (treated as `text`)
- **Combo values** — integer values from JSON no longer break parsing
- **Color input** — hex values (e.g. `#FF8800`) now accepted alongside space-separated floats
- **Boolean values** — `1`/`0` and `"true"`/`"false"` both work
- **HTML labels** — 447 properties with HTML markup render as clean text instead of raw tags
- **Property ordering** — respects the `order` field the author set
- **Duplicate labels** — controls no longer print descriptions twice
- **Slider performance** — tick marks capped at 200, no more thousands-of-ticks warnings
- **Global option labels** — restored after layout refactor
- **Screenshot overwrite** — writes `<monitor>_nyaa_sc.png` instead of one shared filename
- **Volume / mute conflict** — `--volume` no longer re-enables audio when mute is active
- **Crash detection** — hooks `Process.onExit()`, reports which monitor crashed instead of silently saying "Applied"
- **Grid memory leak** — `cellTransitions` map now cleared on `rebuildGrid()`
- **Cross-page arrow navigation** — moving past page boundaries correctly loads adjacent page and positions cursor

### Removed

- `ClampingSetting.java` — replaced by `ClampSetting.java`
- Duplicate label rendering in global options layout

---

## [0.1.1] — 2026-05-01

Initial public release.

- Wallpaper browser with grid layout and keyboard navigation
- Custom property parser (bool, slider, color, combo, text, group types)
- Per-monitor wallpaper assignment with GPU mode selection
- System tray minimize-to-tray
- Fat JAR build (app + launcher)
- Launcher with monitor detection and memory settings
- AUR package
