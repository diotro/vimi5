/*
 * Main collection model, holds list of all videos and tags
 * Copyright (C) 2009-2014 cwk <coolwk@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COLLECTION_H
#define COLLECTION_H

#include "config.h"

#include <QAbstractListModel>
#include <QHash>

class QDir;
class Video;

class Video
{
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        CoverRole,
        FilesRole,
        TagsRole,
        LastPositionRole,
        LastFileRole,
        BookmarksRole,
        ScreenshotsRole
    };

    Video(QString name, QString path, QString cover, QStringList files, QStringList tags, int lastPosition, QString lastFile, QVariantMap bookmarks, QStringList screenshots) :
        name(name), path(path), cover(cover), files(files), tags(tags), lastPosition(lastPosition), lastFile(lastFile), bookmarks(bookmarks), screenshots(screenshots)
    { }

    Video() {}

    friend class Collection;
    friend QDataStream &operator<<(QDataStream &, const Video &);
    friend QDataStream &operator>>(QDataStream &, Video &);

    QString name;
    QString path;
    QString cover;
    QStringList files;
    QStringList tags;
    int lastPosition;
    QString lastFile;
    QVariantMap bookmarks;
    QStringList screenshots;

    bool operator==(const Video &other) const { return (path == other.path); }
    bool operator<(const Video &other) const { return (name < other.name); }
};

class Collection : public QAbstractListModel
{

    Q_OBJECT
    Q_PROPERTY(QStringList allTags READ allTags NOTIFY tagsUpdated)
    Q_PROPERTY(QString status READ getStatus NOTIFY statusUpdated)
    Q_PROPERTY(QStringList actresses READ actresses NOTIFY actressesChanged)
    Q_PROPERTY(bool rescanning READ isRescanning NOTIFY rescanningChanged)
    Q_PROPERTY(bool creatingScreenshots READ isCreatingScreenshots NOTIFY creatingScreenshotsChanged)
    Q_PROPERTY(bool random READ isRandom WRITE setRandom NOTIFY randomChanged)
    Q_PROPERTY(bool empty READ isEmpty() NOTIFY emptyChanged)


public:
    static Collection *instance() { static Collection col; return &col; }
    ~Collection();

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & = QModelIndex()) const { return m_filteredVideos.size(); }
    QHash<int, QByteArray> roleNames() const;

public slots:
    void rescan();
    void writeCache();

    void addTag(int row, QString tag);
    void removeTag(int row, QString tag);

    void addBookmark(int row, QString file, int bookmark );
    void removeBookmark(int row, QString file, int bookmark);


    void setLastFile(int row, QString file);
    void setLastPosition(int row, int position);

    void renameVideo(int row, QString newName);

    void setFilter(QString text);
    void addFilterTag(QString tag);
    void removeFilterTag(QString tag);
    bool filterTagsContains(QString tag);

    void setTagFilter(QString text) { m_tagFilter = text; emit tagsUpdated(); }

    QStringList allTags();

    const QStringList &actresses() { return m_actresses; }

    QString getStatus() { return m_status; }

    void createCover(QString file, qint64 position);
    void createScreenshots(QUrl file);
    void screenshotsCreated(QString path);
    void coverCreated(QString path);

    bool isRescanning() { return m_rescanning; }
    bool isCreatingScreenshots() { return m_creatingScreenshots; }
    bool isRandom() { return m_isRandom; }
    void setRandom(bool random);

    bool isEmpty() { return m_videos.isEmpty(); }

signals:
    void tagsUpdated();
    void statusUpdated();
    void actressesChanged();
    void rescanningChanged();
    void creatingScreenshotsChanged();
    void randomChanged();
    void emptyChanged();

private slots:
    void setStatus(QString status) { m_status = status; emit statusUpdated(); }
    void setRescanning(bool isRescanning) { if (m_rescanning != isRescanning) {m_rescanning = isRescanning; emit rescanningChanged(); } }
    void setCreatingScreenshots(bool creating) { if (m_creatingScreenshots != creating) { m_creatingScreenshots = creating; emit creatingScreenshotsChanged(); } }
    void updateActresses();

private:
    Collection();

    void scan(QDir directory);
    void loadCache();
    void writeTagCache(int index);
    void writeBookmarkCache(int index);
    void updateFilteredVideos();

    QList <Video> m_videos;
    QList <Video*> m_filteredVideos;
    QString m_filter;
    QString m_tagFilter;
    QStringList m_filterTags;
    QStringList m_actresses;
    QString m_status;
    bool m_rescanning;
    bool m_creatingScreenshots;
    bool m_isRandom;
};

#endif // COLLECTION_H
