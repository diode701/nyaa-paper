#pragma once

#include <QObject>
#include <QTimer>

#include "models/WallpaperModel.hpp"

class OrgNyaaPaperLibraryInterface;

/// Fills the model from nyaa-paperd's Library interface.
///
/// The daemon reports facts only (id, basePath, title, type, file, mtime); the
/// preview path is resolved here because images are read from disk, never over
/// D-Bus. Rescans are debounced so a burst of Steam writes costs one refresh.
class DbusLibrary : public QObject
{
    Q_OBJECT

public:
    explicit DbusLibrary(QObject *parent = nullptr);

    /// Ask the daemon for the whole library (page 0, large limit) and parse it.
    void refresh();

Q_SIGNALS:
    void wallpapersLoaded(const QVector<nyaa::domain::Wallpaper> &items);
    void failed(const QString &message);

private:
    void onJson(const QString &json);

    OrgNyaaPaperLibraryInterface *m_library = nullptr;
    QTimer m_debounce;
};
