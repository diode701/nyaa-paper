# Nyaa Paper

A Linux front end for Steam Workshop wallpapers. Browse your subscriptions,
assign them per monitor, build playlists, and close the window — the wallpapers
keep running.

Two processes:

```
nyaa-paperd   Rust daemon   owns the linux-wallpaperengine children, scans the
                            workshop folder, runs playlists, survives the GUI
nyaa-paper-qt C++/QML       the window: library, properties, playlist, settings
```

They talk over D-Bus (`org.nyaa.Paper`), using three interfaces described in
`interfaces/*.xml`: `Engine`, `Library`, `Playlist`. The client renders the full
engine argv; the daemon only spawns and reaps. That split is deliberate: the
daemon never learns what a wallpaper property is, and the UI can be replaced
without touching it.

## Status

Pre-release ([v0.3.0-preview](https://github.com/diode701/nyaa-paper/releases)).
Working: library grid with animated previews, per-monitor apply/stop, property
editing from `project.json`, playlists, per-monitor "Load last", tray, the
Flatpak (GUI, daemon and the bundled engine fork
[nyaa-wallpaperengine](https://github.com/diode701/nyaa-wallpaperengine)).
Known gaps: presets (workshop items that only reference another wallpaper),
Flatpak Steam needs a manual override.

Developer docs: `docs/CLIENT-API.md` (D-Bus contract), `docs/TESTING.md`,
`docs/AUDIT-JAVA-VS-CPP.md` (behaviour compared with the original Java app).

## Build

Requires Qt 6.5+, CMake, Ninja and Rust 1.89+.

```
make daemon        # cargo build in daemon/
make gui           # cmake configure + build in gui/
make test          # cargo test + the domain/config/projectjson tests
```

Run:

```
./daemon/target/debug/nyaa-paperd &
./gui/build/nyaa-paper-qt
```

`NYAA_MOCK=1 ./gui/build/nyaa-paper-qt` runs the UI without a daemon, reading the
workshop folder directly.

## Configuration

One file, shared by both processes: `~/.config/nyaa-paper/config.json`.

| Field | Meaning |
|---|---|
| `paths.workshop` | workshop content folder (`…/content/431960`) |
| `paths.engine` | engine binary; empty = `linux-wallpaperengine` on PATH |
| `paths.assets` | engine assets; empty = the engine's built-in assets |
| `paths.screenshots`, `paths.pywal` | for screenshot consumers |
| `preview.animation` | `always` / `hover` / `off` |
| `engine.*` | volume, fps, scaling, clamp, Wayland layer, extra flags |

The GUI writes it (press `S` in the library); the daemon re-reads it on
`Library.Rescan`, so path changes apply without a restart.

## Keyboard

```
←↑↓→ move      ENTER open      / search      M playlists
G gif mode     A apply        S settings     CTRL+Q quit
```

In the properties view: `CTRL+ENTER` apply, `A` add to playlist, `S` stop,
`L` load last, `ESC` back.

## License

AGPL-3.0-or-later. The previous JavaFX implementation lives on the `java`
branch.
