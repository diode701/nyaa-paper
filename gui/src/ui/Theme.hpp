#pragma once

#include <QColor>
#include <QObject>
#include <QVariantList>
#include <QVector>

class ConfigBridge;

/// The app's colours. One palette is active at a time; it comes from one of
/// four sources (Settings → Appearance):
///   palette    one of the built-in palettes (Nyaa = Java's colours, darker,
///              plus the eight existentialTracker themes)
///   wallpaper  derived from the selected / applied wallpaper's preview
///   pywal      read from <pywal cache>/colors.json
///   system     the Qt / KDE colour scheme
/// "Tint from wallpaper" makes any source follow the selected wallpaper.
class Theme : public QObject
{
    Q_OBJECT
    // tokens
    Q_PROPERTY(QColor bg READ bg NOTIFY changed)
    Q_PROPERTY(QColor panel READ panel NOTIFY changed)
    Q_PROPERTY(QColor card READ card NOTIFY changed)
    Q_PROPERTY(QColor raise READ raise NOTIFY changed)
    Q_PROPERTY(QColor line READ line NOTIFY changed)
    Q_PROPERTY(QColor text READ text NOTIFY changed)
    Q_PROPERTY(QColor muted READ muted NOTIFY changed)
    Q_PROPERTY(QColor dim READ dim NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor accent2 READ accent2 NOTIFY changed)
    Q_PROPERTY(QColor ok READ ok NOTIFY changed)
    Q_PROPERTY(QColor warn READ warn NOTIFY changed)
    Q_PROPERTY(QColor danger READ danger NOTIFY changed)
    // names the older QML still uses
    Q_PROPERTY(QColor panelAlt READ card NOTIFY changed)
    Q_PROPERTY(QColor accentHover READ accent2 NOTIFY changed)
    Q_PROPERTY(QColor accentDim READ accentDim NOTIFY changed)
    Q_PROPERTY(QColor sleekAccent READ accent NOTIFY changed)
    Q_PROPERTY(QColor success READ ok NOTIFY changed)
    Q_PROPERTY(QColor warning READ warn NOTIFY changed)
    Q_PROPERTY(QColor control READ raise NOTIFY changed)
    Q_PROPERTY(QColor controlHover READ line NOTIFY changed)
    // type and shape
    Q_PROPERTY(QString font READ font CONSTANT)
    Q_PROPERTY(QString mono READ font CONSTANT)
    Q_PROPERTY(int radius READ radius CONSTANT)
    Q_PROPERTY(int fast READ fast CONSTANT)
    Q_PROPERTY(int med READ med CONSTANT)
    // for Settings
    Q_PROPERTY(QVariantList palettes READ palettes CONSTANT)

public:
    struct Palette {
        QString key, name, subtitle;
        QColor bg, panel, card, raise, line, text, muted, dim, accent, accent2, ok, warn, danger;
    };

    explicit Theme(const ConfigBridge &config, QObject *parent = nullptr);

    QColor bg() const { return m_p.bg; }
    QColor panel() const { return m_p.panel; }
    QColor card() const { return m_p.card; }
    QColor raise() const { return m_p.raise; }
    QColor line() const { return m_p.line; }
    QColor text() const { return m_p.text; }
    QColor muted() const { return m_p.muted; }
    QColor dim() const { return m_p.dim; }
    QColor accent() const { return m_p.accent; }
    QColor accent2() const { return m_p.accent2; }
    QColor ok() const { return m_p.ok; }
    QColor warn() const { return m_p.warn; }
    QColor danger() const { return m_p.danger; }
    QColor accentDim() const;
    QString font() const { return QStringLiteral("JetBrains Mono"); }
    int radius() const { return 2; }
    int fast() const { return 140; }
    int med() const { return 220; }
    QVariantList palettes() const;

    /// The wallpaper the window should take its colours from (selected in
    /// the browser, open in Properties). Used when the source is "wallpaper"
    /// or tint is on.
    Q_INVOKABLE void followWallpaper(const QString &previewUrl);

    static const QVector<Palette> &builtIn();
    /// Colours derived from an image: background hue from where most of the
    /// picture is, accent from its most vivid colour. Empty key on failure.
    static Palette fromImage(const QString &path);
    static Palette fromPywal(const QString &cacheDir);
    static Palette fromSystem();

Q_SIGNALS:
    void changed();

private:
    void rebuild();

    const ConfigBridge &m_config;
    Palette m_p;
    QString m_wallpaper;
    Palette m_fromWallpaper;
};
