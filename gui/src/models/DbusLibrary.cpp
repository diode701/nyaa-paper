#include "DbusLibrary.hpp"
#include "library_proxy.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace {
constexpr auto kService = "org.nyaa.Paper";
constexpr auto kPath = "/org/nyaa/Paper";
constexpr int kPageLimit = 2000; // more wallpapers than anyone subscribes to

QString previewFor(const QString &basePath)
{
    const QDir dir(basePath);
    for (const auto *name : {"preview.jpg", "preview.gif", "preview.png"}) {
        const QString candidate = dir.filePath(QString::fromLatin1(name));
        if (QFileInfo::exists(candidate))
            return QUrl::fromLocalFile(candidate).toString();
    }
    return {};
}
} // namespace

DbusLibrary::DbusLibrary(QObject *parent)
    : QObject(parent)
{
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        emit failed(tr("cannot reach the session bus"));
        return;
    }

    m_library = new OrgNyaaPaperLibraryInterface(QString::fromLatin1(kService),
                                                 QString::fromLatin1(kPath), bus, this);

    m_debounce.setSingleShot(true);
    m_debounce.setInterval(500); // the contract allows one signal per burst
    connect(&m_debounce, &QTimer::timeout, this, &DbusLibrary::refresh);

    connect(m_library, &OrgNyaaPaperLibraryInterface::LibraryChanged,
            this, [this] { m_debounce.start(); });

    refresh();
}

void DbusLibrary::refresh()
{
    if (!m_library)
        return;

    auto *watcher = new QDBusPendingCallWatcher(m_library->List(0, kPageLimit), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QString> reply = *call;
                if (reply.isError())
                    emit failed(reply.error().message());
                else
                    onJson(reply.value());
                call->deleteLater();
            });
}

void DbusLibrary::onJson(const QString &json)
{
    const QJsonArray array = QJsonDocument::fromJson(json.toUtf8()).array();
    QVector<nyaa::domain::Wallpaper> items;
    items.reserve(array.size());

    for (const QJsonValue &value : array) {
        const QJsonObject obj = value.toObject();
        const QString basePath = obj.value(QStringLiteral("basePath")).toString();
        auto wallpaper = WallpaperModel::make(obj.value(QStringLiteral("id")).toString(),
                                              obj.value(QStringLiteral("title")).toString(),
                                              obj.value(QStringLiteral("type")).toString(),
                                              previewFor(basePath), basePath,
                                              obj.value(QStringLiteral("file")).toString());
        wallpaper.dependency = obj.value(QStringLiteral("dependency")).toString().toStdString();

        // preset values travel as project.json values (strings, numbers, bools);
        // the engine takes them the same way --set-property does
        const QJsonObject preset = obj.value(QStringLiteral("preset")).toObject();
        for (auto it = preset.begin(); it != preset.end(); ++it) {
            const QJsonValue &value = it.value();
            QString text;
            if (value.isBool())
                text = value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
            else if (value.isDouble())
                text = QString::number(value.toDouble(), 'g', 10);
            else if (value.isString())
                text = value.toString();
            else
                continue;

            wallpaper.preset.emplace_back(it.key().toStdString(), text.toStdString());
        }

        items.push_back(wallpaper);
    }

    emit wallpapersLoaded(items);
}
