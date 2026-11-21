# Changelog

All notable changes to Nyaa Paper will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.3.0] — 2026-11-21

Complete rewrite of the JavaFX app as two native processes shipped as a Flatpak: a
Rust daemon that owns the wallpaper engines, and a C++/Qt6 QML client. Behaviour
follows 0.2.0, with the bugs found along the way fixed rather than reproduced.
The Flatpak bundles a fork of linux-wallpaperengine with the fixes it needed.

### Added

- **Two-process architecture** — `nyaa-paperd` (Rust) owns the engines, scans the
  workshop and runs playlists; `nyaa-paper-qt` (C++/QML) is the window. D-Bus
  contract `org.nyaa.Paper` with `Engine`, `Library` and `Playlist` interfaces
  (`interfaces/*.xml`); the client builds the complete engine command line and
  the daemon only spawns, schedules and reaps
- **Per-monitor _and_ per-wallpaper persistence** — every wallpaper keeps its own
  settings per monitor with an explicit "current" marker. **Load last** works for
  any wallpaper, not only the last applied
- **Wallpapers survive the window** — closing or crashing the GUI never stops a
  wallpaper; reopening lists what is running. One daemon per session
- **Playlist runner rebuilt from Java's PlaylistRunner** — one task and channel per
  monitor; sequential/random, interval, loop, play/pause/next/prev/reset
- **Title screen** — the Java launcher's intro grown into a menu; skippable in Settings
- **Redesigned library** — Java grid geometry, animated previews (`always` / `hover` /
  `off`), search on `/`, slim status bar, `?` key overlay, pages turned at the edges
- **C++ thumbnail cache** with an async image provider; neighbours prefetched — 45/45
  cards ready about 40 ms after a page flip (was 5/45)
- **Properties** — Wallpaper/Engine tabs on `[` `]`, monitor map drawn from real screen
  geometry, Load last, add to playlist, stop
- **Engine options** per monitor: volume/mute, FPS limit, scaling, clamping, mouse,
  parallax, audio processing, fullscreen pause, particles, screenshot on apply
- **GPU per wallpaper**: Default, DRI_PRIME (with device number), NVIDIA PRIME offload,
  plus the GL threaded-optimisations toggle; NVIDIA is detected inside the Flatpak too
- **Theme system** — nine palettes, theme from the current wallpaper (in-app extraction
  or pywal) or from the system; hover styles `java`/`lift`/`off`, selection styles
  `brackets`/`outline`/`quiet`, wallpaper backdrop, tint, UI scale
- **Tray**, autostart on login, `--tray` to start hidden
- **Flatpak** — self-contained bundle; the daemon is the sandbox's main process, so `q`
  closes the window and the wallpapers keep running
- **Engine process ownership** — SIGTERM, then 3 s, then the whole process tree;
  an engine exiting on its own is reported per monitor

### Changed

- Java runtime → Rust daemon (zbus) and C++/Qt6 client; no JVM, no heap sizing
- Config lives in `~/.config/nyaa-paper/`: `config.json`, `wallpapers.json`,
  `playlists/<monitor>.json`
- Java's separate launcher app → title screen and Settings view; paths auto-discovered
- One QML control per `Setting` subclass (the port of `createControl`); `GlobalOption`
  is a `Setting`, and `Renderer` turns all settings into argv
- Library scanning skips Wallpaper Engine "Asset" items and folders without a valid
  `project.json`
- Bundled JetBrains Mono; flat, sharp UI with a hidden-until-needed search

### Fixed

- **Load last** only remembered the last wallpaper per monitor (Java bug)
- An empty library wiped saved configs and playlists (Java bug)
- A corrupt `config.json` crashed startup (Java bug) — now falls back to defaults
- Grid selection never matched the delegate (index was undefined)
- Workshop rescan loop; playlist saves could arrive out of order (now serialised)
- Property parsing now matches Java's `PropertyParser`: author order, HTML stripping,
  missing `type`, integer combo values, hex colours
- Fresh installs sent `--layer bottom` to an engine that has no such flag, so no
  wallpaper started; the option now defaults to off

### Removed

- Java runtime and the Gradle/fat-JAR install path
- The launcher setup wizard (paths are found automatically)
- The AUR package for the Java app (branch `java` keeps it)

### Engine — bundled fork of linux-wallpaperengine

Published at `github.com/diode701/nyaa-wallpaperengine`, based on Gaiser147's text
rendering work.

- Rendering no longer blocks on Wayland frame callbacks; hidden-group ancestors are
  honoured; script globals are built once per frame (heavy scenes 5 → 30 fps)
- Mouse input works: pointer events, hit-testing up the parent chain, cursor events,
  `engine.canvasSize`, `thisLayer.size`
- Text uses the fonts shipped with the engine assets
- Web wallpapers render (message pump, texture binding, clean shutdown); Chromium's
  platform follows the session; `LWE_WEB_DEBUG` prints page console output, load
  errors and renderer crashes are always logged
- Audio: resampler heap overflow on mono→stereo fixed (the `double free` crashes);
  per-stream packet accounting; clamped copies; per-object volume; random mode plays
  the whole folder; the decode loop was rewritten (drain frames, never block the device
  callback) which ends the crackles and beeps; auto-mute ignores other engine instances
  and paused streams (it silenced itself in Flatpak and with one engine per monitor)
- Runs from any install location and inside Flatpak: `$ORIGIN` RUNPATH, static kissfft,
  libcef kept ahead of libc, Chromium's sandbox off under `/.flatpak-info`, CEF profile
  under the XDG cache and removed on exit
- Swap interval 0: no freeze while the screen is locked or the display is off
- Chromium's compiler flags no longer leak into the engine library

### Packaging & releases

- Flatpak `io.github.diode701.NyaaPaper` on `org.kde.Platform` 6.11, with GLEW (EGL,
  2.3.1), GLU, freeglut, libass, libplacebo, mpv, GLFW, glm, a pinned CEF archive and
  the engine built from the public fork at a pinned commit; `libcef.so` stripped
- Permissions: Wayland/X11, PulseAudio, DRI, own `org.nyaa.Paper`, read-only access to
  Steam's workshop folder and engine assets (never bundled), Pictures for screenshots
- `scripts/flatpak.sh [bundle]` — ccache and a targeted build, so a one-file engine
  change rebuilds in minutes instead of ~15
- `v0.3.0-preview` GitHub pre-release with the `.flatpak` bundle
- README rewritten for the port with fresh screenshots; `docs/CLIENT-API.md`,
  `docs/TESTING.md`, `docs/AUDIT-JAVA-VS-CPP.md`

### Known issues

- Presets (workshop items that only reference another wallpaper) do not start yet
- Light objects and a few script APIs are unsupported; a few scenes render blank
- Screenshots of web and video wallpapers come out black (library previews are fine)
- Flatpak Steam needs a manual `flatpak override` (in the release notes)
- Interactive web wallpapers only play sound after a click

---

## [0.2.0]

*Original JavaFX app — branch `java`.*

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
