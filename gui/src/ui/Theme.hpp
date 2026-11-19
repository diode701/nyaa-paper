#pragma once

#include <QColor>
#include <QObject>

/// Colours taken from the Java app's dark-theme.css, value for value.
class Theme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor bg READ bg CONSTANT)              // #1e1e1e root
    Q_PROPERTY(QColor panel READ panel CONSTANT)        // #2d2d2d bars
    Q_PROPERTY(QColor panelAlt READ panelAlt CONSTANT)  // #2a2a2a cards / entries
    Q_PROPERTY(QColor line READ line CONSTANT)          // #3a3a3a borders
    Q_PROPERTY(QColor text READ text CONSTANT)          // #ffffff
    Q_PROPERTY(QColor muted READ muted CONSTANT)        // #cccccc status bar
    Q_PROPERTY(QColor dim READ dim CONSTANT)            // #aaaaaa labels / gray
    Q_PROPERTY(QColor accent READ accent CONSTANT)      // #4a90e2
    Q_PROPERTY(QColor accentHover READ accentHover CONSTANT) // #6aa9ff
    Q_PROPERTY(QColor accentDim READ accentDim CONSTANT)
    Q_PROPERTY(QColor success READ success CONSTANT)    // #3a7d44 add to playlist
    Q_PROPERTY(QColor warning READ warning CONSTANT)    // #f0ad4e pause
    Q_PROPERTY(QColor danger READ danger CONSTANT)      // #d9534f stop
    Q_PROPERTY(QColor control READ control CONSTANT)    // #444 buttons
    Q_PROPERTY(QColor controlHover READ controlHover CONSTANT) // #555
    Q_PROPERTY(int radius READ radius CONSTANT)
    Q_PROPERTY(int fast READ fast CONSTANT)
    Q_PROPERTY(int med READ med CONSTANT)
    Q_PROPERTY(QString mono READ mono CONSTANT)

public:
    explicit Theme(QObject *parent = nullptr) : QObject(parent) {}

    QColor bg() const { return QColor("#1e1e1e"); }
    QColor panel() const { return QColor("#2d2d2d"); }
    QColor panelAlt() const { return QColor("#2a2a2a"); }
    QColor line() const { return QColor("#3a3a3a"); }
    QColor text() const { return QColor("#ffffff"); }
    QColor muted() const { return QColor("#cccccc"); }
    QColor dim() const { return QColor("#aaaaaa"); }
    QColor accent() const { return QColor("#4a90e2"); }
    QColor accentHover() const { return QColor("#6aa9ff"); }
    QColor accentDim() const { return QColor("#2f4f73"); }
    QColor success() const { return QColor("#3a7d44"); }
    QColor warning() const { return QColor("#f0ad4e"); }
    QColor danger() const { return QColor("#d9534f"); }
    QColor control() const { return QColor("#444444"); }
    QColor controlHover() const { return QColor("#555555"); }
    int radius() const { return 8; }
    int fast() const { return 140; }
    int med() const { return 220; }
    QString mono() const { return QStringLiteral("monospace"); }
};
