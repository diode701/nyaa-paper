#pragma once

#include <QColor>
#include <QObject>

/// Presentation constants in one place. QML never hardcodes a colour.
class Theme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor bg READ bg CONSTANT)
    Q_PROPERTY(QColor panel READ panel CONSTANT)
    Q_PROPERTY(QColor panelAlt READ panelAlt CONSTANT)
    Q_PROPERTY(QColor line READ line CONSTANT)
    Q_PROPERTY(QColor text READ text CONSTANT)
    Q_PROPERTY(QColor muted READ muted CONSTANT)
    Q_PROPERTY(QColor dim READ dim CONSTANT)
    Q_PROPERTY(QColor accent READ accent CONSTANT)
    Q_PROPERTY(QColor accentDim READ accentDim CONSTANT)
    Q_PROPERTY(QColor danger READ danger CONSTANT)
    Q_PROPERTY(int radius READ radius CONSTANT)
    Q_PROPERTY(int fast READ fast CONSTANT)
    Q_PROPERTY(int med READ med CONSTANT)
    Q_PROPERTY(QString mono READ mono CONSTANT)

public:
    explicit Theme(QObject *parent = nullptr) : QObject(parent) {}

    QColor bg() const { return QColor("#0B0D10"); }
    QColor panel() const { return QColor("#11151B"); }
    QColor panelAlt() const { return QColor("#171C24"); }
    QColor line() const { return QColor("#232A34"); }
    QColor text() const { return QColor("#E8EDF2"); }
    QColor muted() const { return QColor("#9AA6B2"); }
    QColor dim() const { return QColor("#5C6673"); }
    QColor accent() const { return QColor("#5AD2FF"); }
    QColor accentDim() const { return QColor("#1B3A48"); }
    QColor danger() const { return QColor("#FF6B6B"); }
    int radius() const { return 10; }
    int fast() const { return 140; }
    int med() const { return 220; }
    QString mono() const { return QStringLiteral("JetBrains Mono, monospace"); }
};
