> This file is kept as the layer contract the client was built against;
> the pieces marked "to implement" below are now implemented.

# Client API

What the UI needs from the layers underneath it. The QML has already been built
against this surface; everything here is what remains to be implemented.

## 1. The library model (already implemented, currently fed by MockLibrary)

```cpp
struct WallpaperItem { QString id, title, kind, preview; };
WallpaperModel::setAll(QVector<WallpaperItem>);
```

QML roles: `wallpaperId`, `title`, `kind`, `preview`.
The `kind` role carries the daemon's `type` field (`"scene"`, `"video"`, `"web"`).

`preview` is a **local file URL** to `preview.jpg|gif|png` inside the wallpaper
folder. Empty string means "draw the procedural fallback". The client reads the
file itself; images never travel over D-Bus.

Replacement point: `MockLibrary::load()` → `DbusLibrary` calling
`org.nyaa.Paper.Library.List(offset, limit)` and parsing its JSON array.

## 2. EngineBridge (implemented, QML-visible as the `Engine` singleton)

```cpp
Q_PROPERTY(QString version, enginePath)
Q_PROPERTY(QVariantMap running)     // monitor -> pid
Q_PROPERTY(QStringList monitors)    // from QGuiApplication::screens()
Q_PROPERTY(bool mock)
Q_INVOKABLE void refreshRunning();
Q_INVOKABLE void stop(const QString &monitor);
Q_INVOKABLE void stopAll();
signals: infoChanged, runningChanged, engineExited(monitor, code), error(message)
```

## 2b. Presentation preferences (implemented)

```cpp
class PreviewSettings {          // QML: Settings
    Q_PROPERTY(int animationMode)   // 0 off, 1 hover, 2 always
    Q_INVOKABLE void cycle();       // bound to G
};
```
Stored at `~/.config/nyaa-paper/gui.ini` so dev and flatpak share one file.

## 3. Domain layer — this is what you write

```cpp
enum class Kind { Bool, Slider, Color, Combo, Text, Group, TextInput, File, Directory, Texture };

class Setting {                      // domain/Setting.hpp, no Qt
public:
    virtual ~Setting() = default;
    const std::string &key() const;
    const std::string &label() const;
    virtual Kind kind() const = 0;
    virtual std::string argument() const = 0;      // "--set-property key=value"
    virtual std::string serialize() const = 0;
    virtual void restore(const std::string &) = 0;
    virtual std::string display() const = 0;
};
```

Then:

- `CustomSetting<T> : Setting` holding the typed value
- ten leaves: `BoolSetting, SliderSetting, ColorSetting, ComboSetting, TextSetting,
  GroupSetting, TextInputSetting, FileSetting, DirectorySetting, TextureSetting`
- `SettingFactory::create(const SettingDesc &) -> std::unique_ptr<Setting>`
- `SettingList` owning `std::vector<std::unique_ptr<Setting>>`, with
  `arguments()`, `serialize()`, `restore()`, `find()`
- `Renderer::render(const Wallpaper &, const SettingList &, const GpuConfig &,
   const Screenshot &) -> EngineCommand`
- global options: `VolumeOption, FpsOption, ScalingOption, ClampOption, FlagOption`

## 4. How it plugs in

```
QML grid ──► WallpaperModel ──► (DbusLibrary | MockLibrary)
QML apply ─► EngineBridge::apply(monitor, EngineCommand)
                                   ▲
                     Renderer::render(Wallpaper, SettingList, GpuConfig, Screenshot)
```

The apply path is the only place a wallpaper is started: QML never builds
arguments, and the daemon never interprets them.

## 5. Property view (not built yet)

Planned: `PropertyModel : QAbstractListModel` exposing one row per `Setting`:

| role | type | used by |
|---|---|---|
| `label` | QString | row text |
| `kind` | int (`Kind`) | delegate choice |
| `display` | QString | value readout |
| `value` | QVariant | editor binding |
| `minimum`, `maximum`, `step` | double | slider |
| `options` | QStringList | combo |
| `group` | bool | section header styling |

The view switches on `kind` in QML; the model never contains UI logic.

## 6. Contract notes paid for during the first build

- `Library.List` returns JSON (`s`), not a D-Bus struct: qdbusxml2cpp cannot
  marshal `a(ssx)` without annotations and metatype glue.
- `Engine.ListRunning` returns a JSON object (`s`) for the same reason.
- `Engine.Apply` takes `env` as `as` of `"KEY=VALUE"` strings, not `a{ss}`.
- XML files must not contain `--` inside comments (invalid XML, qdbusxml2cpp
  fails with a misleading error).
