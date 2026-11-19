# Testing

## Automated

```
make test
```

Runs `cargo test` in the daemon and the four C++ test binaries through CTest:

| Test | What it proves |
|---|---|
| `domain_test` | the ten setting kinds produce the exact argv, restore parses, renderer ordering and id-vs-path, GPU env |
| `projectjson_test` | `general.properties` parsing: order, HTML stripped, hex colour, combo options, unknown types skipped |
| `config_test` | `config.json` round trip, defaults, per-monitor memory round trip |
| `videoduration_test` | ffprobe helper returns -1 without hanging on missing/non-video files |

```
make smoke
```

Starts a throwaway daemon with a fake workshop and `/bin/sleep` as the engine,
then asserts: scan count, the watcher noticing a new folder, Apply, ListRunning,
playlist play/stop, and that the client renders a frame against it. Everything
lives in `scripts/smoke.sh`; it cleans up after itself.

## Manual, client against the daemon

```
./daemon/target/debug/nyaa-paperd &     # terminal 1
./gui/build/nyaa-paper-qt               # terminal 2
```

1. **Library** — 187 cards, GIFs animating (press `G` to cycle always → hover →
   off; the rail shows the mode). Arrows move the ring; at a page edge the deck
   slides and the counter changes.
2. **Search** — `/`, type `ore`, the count drops, `Esc` restores.
3. **Properties** — `Enter` opens the selected wallpaper: title, preview, id,
   and its real properties. Edit a slider: the value updates in place.
4. **Apply** — pick a monitor, `Ctrl+Enter`. A wallpaper appears on that
   monitor. The monitor pill in the library lights up afterwards.
5. **Replace** — apply a second wallpaper to the same monitor: the first
   disappears, no crash toast.
6. **Stop** — `S` stops that monitor; the pill goes dark.
7. **Playlist** — `A` in the properties view adds the wallpaper; `M` opens the
   playlist; `Play` cycles entries, `Pause` freezes, `Stop` returns to idle.
   Video entries show their probed duration.
8. **Settings** — `S` in the library: change the workshop path, `Save`; the
   toast stays quiet and the library reloads from the new folder.
9. **Load last** — change a slider, `Apply`, move away, come back, `L`: the
   values return without re-applying.
10. **Daemon survives** — with a wallpaper running, close the window (or kill
    the client) and confirm the wallpaper keeps playing.

## Manual, daemon alone

```
busctl --user introspect org.nyaa.Paper /org/nyaa/Paper
busctl --user call org.nyaa.Paper /org/nyaa/Paper org.nyaa.Paper.Library Count
busctl --user call org.nyaa.Paper /org/nyaa/Paper org.nyaa.Paper.Library List uu 0 2
busctl --user call org.nyaa.Paper /org/nyaa/Paper org.nyaa.Paper.Engine ListRunning
busctl --user monitor org.nyaa.Paper        # watch EngineExited / LibraryChanged
```

Fake engine (no GPU needed):

```
NYAA_ENGINE=/bin/sleep ./daemon/target/debug/nyaa-paperd
busctl --user call org.nyaa.Paper /org/nyaa/Paper org.nyaa.Paper.Engine \
  Apply ssasas eDP-1 1234 1 5 0
```

## Known limits of headless testing

- `MultiEffect` (card shadows, selection glow) does not render under the
  offscreen platform, so screenshots validate layout, not effects.
- Monitor names come from `QGuiApplication::screens()`; headless runs fall back
  to `eDP-1`/`HDMI-1`.
- A real wallpaper cannot be applied headlessly: it would draw on the session.
