#include "MockLibrary.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace {

QString previewFor(const QDir &dir)
{
    for (const auto *name : {"preview.jpg", "preview.gif", "preview.png"}) {
        if (QFileInfo::exists(dir.filePath(name)))
            return QUrl::fromLocalFile(dir.filePath(name)).toString();
    }
    return {};
}

} // namespace

QVector<WallpaperItem> MockLibrary::fromWorkshop(const QString &root)
{
    QVector<WallpaperItem> items;
    const QDir dir(root);
    if (!dir.exists())
        return items;

    const QStringList ids = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &id : ids) {
        const QDir wallpaperDir(dir.filePath(id));
        QString title = id;
        QString kind;

        QFile project(wallpaperDir.filePath("project.json"));
        if (project.open(QIODevice::ReadOnly)) {
            const QJsonObject obj = QJsonDocument::fromJson(project.readAll()).object();
            title = obj.value("title").toString(title);
            kind = obj.value("type").toString();
        }

        const QString preview = previewFor(wallpaperDir);
        if (preview.isEmpty() && kind.isEmpty())
            continue; // not a wallpaper folder

        items.push_back({id, title, kind, preview});
    }
    return items;
}

QVector<WallpaperItem> MockLibrary::sample()
{
    const QStringList titles{
        "neon alley", "kitsune dream", "orbital drift", "rain on glass", "paper crane",
        "deep water", "signal lost", "midnight diner", "static garden", "train window",
        "lantern street", "soft fog", "cassette sky", "hollow pines"};
    QVector<WallpaperItem> items;
    items.reserve(titles.size());
    for (int i = 0; i < titles.size(); ++i) {
        const QString kind = (i % 3 == 0) ? QStringLiteral("video")
                            : (i % 3 == 1) ? QStringLiteral("scene")
                                           : QStringLiteral("web");
        items.push_back({QStringLiteral("sample-%1").arg(i + 1), titles[i], kind, {}});
    }
    return items;
}

QVector<WallpaperItem> MockLibrary::load()
{
    const QString root = qEnvironmentVariable(
        "NYAA_WORKSHOP",
        QDir::homePath() + QStringLiteral("/.local/share/Steam/steamapps/workshop/content/431960"));
    QVector<WallpaperItem> items = fromWorkshop(root);
    if (items.isEmpty())
        items = sample();
    return items;
}
