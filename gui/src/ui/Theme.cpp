#include "Theme.hpp"

#include <array>
#include <cmath>

#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QUrl>
#include <QVariantMap>

#include "bridge/ConfigBridge.hpp"

namespace {

QColor hsl(double hue, double s, double l)
{
    return QColor::fromHslF(std::fmod(hue + 360.0, 360.0) / 360.0, s, l);
}

QColor mix(const QColor &a, const QColor &b, double t)
{
    return QColor::fromRgbF(a.redF() * (1 - t) + b.redF() * t, a.greenF() * (1 - t) + b.greenF() * t,
                            a.blueF() * (1 - t) + b.blueF() * t);
}

Theme::Palette make(const char *key, const char *name, const char *subtitle,
                    std::array<const char *, 13> c)
{
    return {key, name, subtitle, QColor(c[0]), QColor(c[1]), QColor(c[2]), QColor(c[3]), QColor(c[4]),
            QColor(c[5]), QColor(c[6]), QColor(c[7]), QColor(c[8]), QColor(c[9]), QColor(c[10]),
            QColor(c[11]), QColor(c[12])};
}

/// The parts every derived palette shares: neutrals from a hue, fixed states.
Theme::Palette derived(const char *key, double groundHue, const QColor &accent, const QColor &accent2)
{
    Theme::Palette p;
    p.key = key;
    p.bg = hsl(groundHue, 0.18, 0.07);
    p.panel = hsl(groundHue, 0.16, 0.10);
    p.card = hsl(groundHue, 0.15, 0.12);
    p.raise = hsl(groundHue, 0.14, 0.15);
    p.line = hsl(groundHue, 0.14, 0.19);
    p.text = hsl(groundHue, 0.20, 0.93);
    p.muted = hsl(groundHue, 0.10, 0.66);
    p.dim = hsl(groundHue, 0.08, 0.44);
    p.accent = accent;
    p.accent2 = accent2;
    p.ok = QColor("#5fa86b");
    p.warn = QColor("#d9b25a");
    p.danger = QColor("#d9605a");
    return p;
}

} // namespace

const QVector<Theme::Palette> &Theme::builtIn()
{
    //                          bg         panel      card       raise      line       text       muted      dim        accent     accent2    ok         warn       danger
    static const QVector<Palette> all{
        make("nyaa", "Nyaa", "default · Java colours, darker charcoal",
             {"#121212", "#191919", "#1e1e1e", "#262626", "#2c2c2c", "#f0f0f0", "#a6a6a6", "#6e6e6e", "#4a90e2", "#6aa9ff", "#4e9a5a", "#f0ad4e", "#d9534f"}),
        make("sumi_e", "Sumi-e", "墨絵 · ink wash",
             {"#131211", "#1a1817", "#201e1c", "#282522", "#302c29", "#e8e2d8", "#a49c91", "#6f6860", "#b8645e", "#5f87af", "#6f9a6f", "#a8a070", "#c4564f"}),
        make("ukiyo_e", "Ukiyo-e", "浮世絵 · woodblock",
             {"#101219", "#171a24", "#1c1f2b", "#242838", "#2c3042", "#ece5d3", "#a49d8b", "#6d6a62", "#c8822b", "#d7372f", "#3c9a4c", "#c8a43a", "#d7372f"}),
        make("cherry_blossom", "Cherry Blossom", "桜 · spring",
             {"#161214", "#1e181b", "#241d21", "#2d2429", "#3a2e34", "#f4eaef", "#bba9b1", "#7b6b73", "#ff6fb5", "#87afff", "#87d787", "#ffd7af", "#ff8787"}),
        make("sakura_red", "Sakura Red", "紅桜 · deep sakura",
             {"#170f12", "#20151a", "#271a20", "#312129", "#3d2831", "#f6ecef", "#c3a9b2", "#806670", "#ff4f7b", "#87d7ff", "#5fd75f", "#ffd787", "#ff5f5f"}),
        make("zen_garden", "Zen Garden", "枯山水 · moss and sand",
             {"#111312", "#181b19", "#1d201e", "#252926", "#2c312d", "#e4e6e3", "#9ba59e", "#66706a", "#87af87", "#af8787", "#87af87", "#afaf87", "#af6f6f"}),
        make("samurai", "Samurai", "鎧 · lacquer and bronze",
             {"#100e0e", "#181414", "#1e1818", "#271f1f", "#322828", "#ddd5ce", "#9a908a", "#665e5a", "#c0392b", "#c27a2c", "#2f9a74", "#c27a2c", "#c0392b"}),
        make("wabi_sabi", "Wabi-Sabi", "侘寂 · weathered",
             {"#141312", "#1c1a18", "#211f1c", "#2a2724", "#34302b", "#dcd9d3", "#a29b90", "#6c665d", "#c9705f", "#87afaf", "#6f9a6f", "#afaf87", "#d75f5f"}),
        make("kyoto_twilight", "Kyoto Twilight", "京都の黄昏 · dusk",
             {"#12111a", "#191724", "#1f1c2c", "#282438", "#302b42", "#e4def2", "#a49db8", "#6c6680", "#9b6bff", "#d75f5f", "#6fb33c", "#c9c08a", "#d75f5f"}),
    };
    return all;
}

Theme::Theme(const ConfigBridge &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
    connect(&m_config, &ConfigBridge::changed, this, &Theme::rebuild);
    rebuild();
}

QColor Theme::accentDim() const
{
    return mix(m_p.bg, m_p.accent, 0.35);
}

QVariantList Theme::palettes() const
{
    QVariantList out;
    for (const Palette &p : builtIn()) {
        QVariantMap row;
        row["key"] = p.key;
        row["name"] = p.name;
        row["subtitle"] = p.subtitle;
        row["swatch"] = QVariantList{p.bg, p.card, p.accent, p.accent2, p.ok, p.warn, p.text};
        out << row;
    }
    return out;
}

void Theme::followWallpaper(const QString &previewUrl)
{
    if (previewUrl == m_wallpaper)
        return;
    m_wallpaper = previewUrl;
    const QString path = QUrl(previewUrl).isLocalFile() ? QUrl(previewUrl).toLocalFile() : previewUrl;
    m_fromWallpaper = path.isEmpty() ? Palette{} : fromImage(path);
    const bool follows = m_config.tint() || m_config.themeSource() == QStringLiteral("wallpaper");
    if (follows)
        rebuild();
}

void Theme::rebuild()
{
    const QString source = m_config.themeSource();
    Palette next;
    if ((m_config.tint() || source == QStringLiteral("wallpaper")) && !m_fromWallpaper.key.isEmpty())
        next = m_fromWallpaper;
    else if (source == QStringLiteral("pywal"))
        next = fromPywal(m_config.pywalPath());
    else if (source == QStringLiteral("system"))
        next = fromSystem();

    if (next.key.isEmpty()) {   // palette, or a source that had nothing to give
        next = builtIn().first();
        for (const Palette &p : builtIn())
            if (p.key == m_config.palette())
                next = p;
    }
    if (next.bg == m_p.bg && next.accent == m_p.accent && next.text == m_p.text && next.key == m_p.key)
        return;
    m_p = next;
    emit changed();
}

Theme::Palette Theme::fromImage(const QString &path)
{
    QImageReader reader(path);
    reader.setScaledSize(QSize(48, 48));   // decode small: this runs on selection
    const QImage image = reader.read().convertToFormat(QImage::Format_RGB32);
    if (image.isNull())
        return {};

    // two hue histograms: where the picture is (ground) and where it is vivid (accent)
    std::array<double, 36> ground{}, vivid{};
    double grey = 0.0, total = 0.0;
    for (int y = 0; y < image.height(); ++y) {
        const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QColor c(row[x]);
            const double s = c.hsvSaturationF(), v = c.valueF();
            total += 1.0;
            if (s < 0.12 || v < 0.08) {
                grey += 1.0;
                continue;
            }
            const int bin = qBound(0, c.hsvHue() / 10, 35);
            ground[bin] += v;
            // vividness cubed: a small bright patch beats a large dull area,
            // which is what an accent should be
            const double vividness = s * v;
            vivid[bin] += vividness * vividness * vividness;
        }
    }
    auto peak = [](const std::array<double, 36> &h, int avoid) {
        int best = 0;
        double weight = -1;
        for (int i = 0; i < 36; ++i) {
            const int distance = std::min(std::abs(i - avoid), 36 - std::abs(i - avoid));
            if (avoid >= 0 && distance < 6)   // at least 60° away
                continue;
            // smooth over neighbours so one stray bin does not win
            const double w = h[(i + 35) % 36] * 0.5 + h[i] + h[(i + 1) % 36] * 0.5;
            if (w > weight) {
                weight = w;
                best = i;
            }
        }
        return std::pair<int, double>(best, weight);
    };
    const auto [groundBin, groundWeight] = peak(ground, -1);
    const auto [accentBin, accentWeight] = peak(vivid, -1);
    const auto [secondBin, secondWeight] = peak(vivid, accentBin);
    if (accentWeight <= 0.0 || grey / total > 0.92) {
        // a grey picture: a neutral window with a quiet accent
        return derived("wallpaper", groundBin * 10 + 5, hsl(groundBin * 10 + 5, 0.25, 0.62),
                       hsl(groundBin * 10 + 185, 0.25, 0.60));
    }
    Q_UNUSED(groundWeight);
    Q_UNUSED(secondWeight);
    return derived("wallpaper", groundBin * 10 + 5, hsl(accentBin * 10 + 5, 0.68, 0.62),
                   hsl(secondBin * 10 + 5, 0.55, 0.60));
}

Theme::Palette Theme::fromPywal(const QString &cacheDir)
{
    QFile file(cacheDir + QStringLiteral("/colors.json"));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject special = root.value("special").toObject();
    const QJsonObject colors = root.value("colors").toObject();
    const QColor background(special.value("background").toString());
    const QColor foreground(special.value("foreground").toString());
    if (!background.isValid() || !foreground.isValid())
        return {};

    // the two most saturated of color1..color6 become the accents
    QVector<QColor> picks;
    for (int i = 1; i <= 6; ++i) {
        const QColor c(colors.value(QStringLiteral("color%1").arg(i)).toString());
        if (c.isValid())
            picks << c;
    }
    std::sort(picks.begin(), picks.end(), [](const QColor &a, const QColor &b) {
        return a.hsvSaturationF() * a.valueF() > b.hsvSaturationF() * b.valueF();
    });

    Palette p;
    p.key = QStringLiteral("pywal");
    // keep the window dark even if wal picked a light background
    p.bg = background.lightnessF() > 0.12 ? QColor::fromHslF(background.hslHueF(), background.hslSaturationF(), 0.08)
                                          : background;
    p.panel = p.bg.lighter(125);
    p.card = p.bg.lighter(145);
    p.raise = p.bg.lighter(175);
    p.line = p.bg.lighter(210);
    p.text = foreground;
    p.muted = mix(foreground, p.bg, 0.35);
    p.dim = mix(foreground, p.bg, 0.6);
    p.accent = picks.value(0, QColor("#4a90e2"));
    p.accent2 = picks.value(1, p.accent.lighter(120));
    p.ok = QColor(colors.value("color2").toString("#5fa86b"));
    p.warn = QColor(colors.value("color3").toString("#d9b25a"));
    p.danger = QColor(colors.value("color1").toString("#d9605a"));
    return p;
}

Theme::Palette Theme::fromSystem()
{
    const QPalette sys = QGuiApplication::palette();
    Palette p;
    p.key = QStringLiteral("system");
    p.panel = sys.color(QPalette::Window);
    p.bg = p.panel.darker(118);
    p.card = sys.color(QPalette::Base);
    p.raise = sys.color(QPalette::Button);
    p.line = sys.color(QPalette::Mid);
    p.text = sys.color(QPalette::WindowText);
    p.muted = mix(p.text, p.bg, 0.35);
    p.dim = mix(p.text, p.bg, 0.6);
    p.accent = sys.color(QPalette::Highlight);
    p.accent2 = sys.color(QPalette::Link);
    p.ok = QColor("#5fa86b");
    p.warn = QColor("#d9b25a");
    p.danger = QColor("#d9605a");
    return p;
}
