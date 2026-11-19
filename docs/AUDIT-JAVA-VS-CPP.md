# Java → C++ audit

Read against `~/nyaa-paper-java` (branch `java`), class by class. Every row was
checked in the source, not from memory.

Legend: **same** = behaviour reproduced; **better** = deliberately different and
justified; **gap** = Java does something v2 does not; **dropped** = intentionally
not ported.

## Keyboard

| Where | Java | v2 | Status |
|---|---|---|---|
| Browser | arrows | arrows | same |
| Browser | `PageUp` / `N` | `PgUp` / `N` | same (added) |
| Browser | `PageDown` / `P` | `PgDown` / `P` | same (added) |
| Browser | `Enter` | `Enter` | same |
| Browser | `M` playlist manager | `M` | same |
| Browser | `Q` quit | `Q`, `Ctrl+Q` | same (added) |
| Browser | — | `/` search | better |
| Browser | — | `G` animation mode | better |
| Browser | — | `S` settings | better |
| Browser | — | `A` apply to first monitor | better |
| Properties | `Ctrl+Enter` apply | `Ctrl+Enter` | same |
| Properties | `Ctrl+Up` / `Ctrl+Down` cycle monitor | same | same (added) |
| Properties | digits `1..9`, `0` = tenth | same | same (added) |
| Properties | `A` add to playlist | `A` | same |
| Properties | `S` stop | `S` | same |
| Properties | `L` load saved settings | `L` | same |
| Properties | `Ctrl+L` toggle mini playlist panel | same | same (added) |
| Properties | `Esc` back | `Esc` | same |
| Playlist | `Esc` / `B` back | `Esc` / `B` | same (added) |
| Playlist | `Up` / `Down` select entry | same | same (added) |
| Playlist | `Delete` remove | same | same (added) |
| Playlist | `Space` play/pause | same | same |
| Playlist | `N` next / `P` previous | same, via new `Next`/`Prev` methods | same (added) |

## Library

| Java | v2 | Status |
|---|---|---|
| `WpprRtrv` reads the workshop folder on a background task | daemon scans and serves `Library.List` | better: the UI never touches the disk |
| `WorkshopWatcher` (WatchService) rebuilds on any event | daemon `notify` + 300 ms debounce | better, and see the loop bug below |
| pages of `gridCols × gridRows`, prefetch next page | page deck, no prefetch | **gap**: page switch can show blank cards for a moment |
| selection ring, status rail | same | same |
| previews: GIF via `Image` with a timer | `AnimatedImage`, mode `always/hover/off` | better |
| search | search overlay | better |
| monitor pills in the rail | pills + `Engine.running` | same |

## Properties

| Java | v2 | Status |
|---|---|---|
| `PropertyParser` → `Setting`s | `io/ProjectJson` → `SettingFactory` | same, unit-tested |
| sliders, combos, bools, colours, text, files, groups | same, plus a texture delegate | same |
| Global tab: volume+mute, fps, scaling, clamp, flags | same, bound to `config.json` | same |
| monitor combo | pills + digits + `Ctrl+Up/Down` | better |
| `MiniPlaylistPanel` (`Ctrl+L`) | — | **gap** |
| "Load last" from `WallpaperConfig` | `L` + `io/WallpaperStore` | same |
| Apply writes saved values | Apply also saves values | better |

## Playlist

| Java | v2 | Status |
|---|---|---|
| `PlaylistManagerView`: entries, monitor selector, play/pause/stop | same | same |
| `PlaylistRunner` in the UI process | `playlist.rs` in the daemon | better: survives the UI |
| entries carry duration from `VideoDurationUtil` | same (`util/VideoDuration`) | same |
| `next` / `prev` | `Playlist.Next` / `Playlist.Prev` | same |
| reorder by drag | — | **gap** |
| `MiniPlaylistPanel` in the properties view | `Ctrl+L` panel | same |

## Wallpaper / engine

| Java | v2 | Status |
|---|---|---|
| `WallpaperInstance.getStrings()` | `domain::Renderer` | same, unit-tested |
| `GpuConfig` (driprime, threaded) | `GpuConfig` | same |
| `--screenshot` support | `Screenshot` struct, no UI toggle | **gap** (UI) |
| `--assets-dir` | config `paths.assets` | same |
| `--layer` | config `engine.layer` | better (Wayland) |
| engine child dies with the app | daemon outlives the UI | better |
| no crash detection | `EngineExited` + flag | better |
| hard exit kills children | `StopAll` on shutdown | same |

## Config

| Java | v2 | Status |
|---|---|---|
| `SettingsManager` (static) | `io/ConfigStore` injected | better for tests |
| `WallpaperConfigManager` per wallpaper | `io/WallpaperStore` per monitor | better |
| `PlaylistManager` files | daemon `state/playlists/*.json` | same idea, daemon-owned |
| paths hardcoded / scattered | one `config.json`, shared | better |
| `ParseMD`, `Wppr`, `WpprLib` metadata | daemon reads `project.json` top level | same |

## Launcher (`NyaaLauncher`, `setup`)

Not ported. Java shipped a first-run wizard (Steam path, workshop pick, engine
path) with a second scene. v2 replaces it with auto-discovery + the `S` settings
overlay. **Dropped**, and the reason is defensible: the wizard existed because
the Java app could not discover paths; the daemon can.

## Bugs found by this audit

1. **Rescan loop** (also reproducible in the Java app): inotify reports our own
   reads of `project.json` as `Access` events, which set the dirty flag, which
   rescans, which reads again. Fixed in `library.rs` by ignoring `Access` events
   and by not emitting when the id set did not change. Verified: 0 lines when
   idle, 1 line per real change.
2. The Global tab had been replaced with a text placeholder — fixed.
3. Role name `value` shadowed `Slider.value`/`SpinBox.value` in delegates —
   fixed (`settingValue`).
4. The properties preview collapsed because its height came from
   `Layout.preferredWidth` in the same attachment — fixed (`leftWidth`).

## Remaining priority

1. Page prefetch in the library.
2. Screenshot toggle in the UI.
3. Drag reorder in the playlist.
4. First-run launcher (Java has a separate setup app; v2 auto-discovers paths).
