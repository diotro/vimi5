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

#include <QByteArray>
#include <QStandardPaths>
#include <QSet>
#include <QThread>
#include <QThreadPool>
#include <QDir>
#include <QGuiApplication>

#include <algorithm>

extern "C" {
#define __STDC_CONSTANT_MACROS
#include <libavformat/avformat.h>
}

#include "collection.h"
#include "videoframedumper.h"

QDataStream &operator<<(QDataStream &datastream, const Video &video) {
    datastream << video.name << video.path << video.cover << video.files << video.tags << video.lastPosition << video.lastFile << video.bookmarks << video.screenshots << video.resolution << video.framerate << video.description;
    return datastream;
}

QDataStream &operator>>(QDataStream &datastream, Video &video) {
    datastream >> video.name >> video.path >> video.cover >> video.files >> video.tags >> video.lastPosition >> video.lastFile >> video.bookmarks >> video.screenshots >> video.resolution >> video.framerate >> video.description;
    return datastream;
}


bool compareVideos(const Video *a, const Video *b)
{
    return a->name < b->name;
}

Collection::Collection()
    : QAbstractListModel(), m_rescanning(false), m_creatingScreenshots(false), m_isRandom(false)
{
    loadCache();

    // Check if cache load was successful
    if (m_videos.count() == 0)
        rescan();

    updateActresses();
    connect(Config::instance(), SIGNAL(actressPathChanged()), SLOT(updateActresses()));

    QFile synonyms("/home/test/pornstarnames.txt");
    if (!synonyms.open(QIODevice::ReadOnly)) {
        return;
    }

    foreach(const QByteArray &line, synonyms.readAll().split('\n')) {
        QList<QByteArray> nameAliases = line.split(':');
        if (nameAliases.length() != 2) continue;
        foreach(const QByteArray &aliasB, nameAliases[1].split(',')) {
            QString alias = QString::fromUtf8(aliasB).toLower().trimmed();
            QString tag = QString::fromUtf8(nameAliases[0]).toLower().trimmed();
            m_synonyms[alias] = tag;
        }
    }
}

void Collection::updateActresses()
{
    QDir dir(QUrl(Config::instance()->actressesPath()).toLocalFile());
    QFileInfoList fileList = dir.entryInfoList(QStringList() << "*.jpg" << "*.png", QDir::Files, QDir::Name);

    QStringList tags = allTags();
    QStringList newActressList;
    QSet<QString> allActresses;

    foreach (const QFileInfo &file, fileList) {
        allActresses.insert(file.baseName());
    }

    foreach (const QString &tag, tags) {
        if (allActresses.contains(tag)) {
            newActressList.append(tag);
        }
    }

    if (newActressList != m_actresses) {
        m_actresses = newActressList;
        emit actressesChanged();
    }
}


QHash<int, QByteArray> Collection::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Video::NameRole] = "name";
    roleNames[Video::PathRole] = "path";
    roleNames[Video::CoverRole] = "coverPath";
    roleNames[Video::FilesRole] = "files";
    roleNames[Video::TagsRole] = "tags";
    roleNames[Video::LastPositionRole] = "lastPosition";
    roleNames[Video::LastFileRole] = "lastFile";
    roleNames[Video::BookmarksRole] = "bookmarks";
    roleNames[Video::ScreenshotsRole] = "screenshots";
    roleNames[Video::ResolutionRole] = "resolution";
    roleNames[Video::FramerateRole] = "framerate";
    roleNames[Video::DescriptionRole] = "description";
    return roleNames;
}

Collection::~Collection()
{
    writeCache();
}

void Collection::writeCache()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir(path).mkpath(path);
    QFile file(path + "/videos.db");

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << file.errorString();
        return;
    }
    QByteArray buffer;
    QDataStream bufferWriter(&buffer, QIODevice::WriteOnly);
    bufferWriter << m_videos;
    QByteArray compressed = qCompress(buffer);
    file.write(compressed);
}

void Collection::loadCache()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir(path).mkpath(path);
    QFile file(path + "/videos.db");

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to load cache!: " << file.errorString() << " (file path:" << file.fileName() << ")";
        return;
    }

    beginResetModel();
    m_videos.clear();
    QByteArray compressed = file.readAll();
    if (compressed.isEmpty()) {
        return;
    }
    QByteArray uncompressed = qUncompress(compressed);
    QDataStream in(uncompressed);
    in >> m_videos;
    qSort(m_videos);

    foreach(const Video &video, m_videos) {
        foreach(const QString &tag, video.tags) {
            if (m_synonyms.contains(tag))
                qDebug() << video.name << tag;

        }
    }

    updateFilteredVideos();
    endResetModel();

    emit emptyChanged();
}


QVariant Collection::data(const QModelIndex &item, int role) const
{
    if (!item.isValid()) {
        qWarning() << "Invalid item!";
        return QVariant();
    }

    int row = item.row();

    if (row > m_videos.count()) {
        qWarning() << "row out of bounds" << row;
        return QVariant();
    }

    switch(role){
    case Video::NameRole:
        return m_filteredVideos[row]->name;
    case Video::PathRole:
        return m_filteredVideos[row]->path;
    case Video::CoverRole:
        return m_filteredVideos[row]->cover;
    case Video::FilesRole:
        return m_filteredVideos[row]->files;
    case Video::TagsRole:
        return m_filteredVideos[row]->tags;
    case Video::LastPositionRole:
        return m_filteredVideos[row]->lastPosition;
    case Video::LastFileRole:
        return m_filteredVideos[row]->lastFile.isEmpty() ? m_filteredVideos[row]->files.first() :  m_filteredVideos[row]->lastFile;
    case Video::BookmarksRole:
        return m_filteredVideos[row]->bookmarks;
    case Video::ScreenshotsRole:
        return m_filteredVideos[row]->screenshots;
    case Video::ResolutionRole:
        return m_filteredVideos[row]->resolution;
    case Video::FramerateRole:
        return m_filteredVideos[row]->framerate;
    case Video::DescriptionRole:
        return m_filteredVideos[row]->description;
    default:
        qWarning() << "Unknown role" << role;
        return QVariant();
    }
}

void Collection::writeBookmarkCache(int index)
{
    QString filename = m_filteredVideos[index]->path + "/bookmarks.dat";
    QFile file(filename + ".tmp");
    QDataStream out(&file);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        out << m_filteredVideos[index]->bookmarks;
        QFile::remove(filename);
        file.rename(filename + ".tmp", filename);
    } else {
        qWarning() << "Unable to open tag cache file for writing!:" << filename;
    }
}


void Collection::addBookmark(int row, QString file, int bookmark)
{
    QList<QVariant> bookmarks = m_filteredVideos[row]->bookmarks[file].toList();
    bookmarks.append(bookmark);
    qSort(bookmarks);
    m_filteredVideos[row]->bookmarks[file] = bookmarks;

    emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>() << Video::BookmarksRole);
    writeBookmarkCache(row);
}

void Collection::removeBookmark(int row, QString file, int bookmark)
{
    if (!m_filteredVideos[row]->bookmarks.contains(file))
        return;

    QList<QVariant> bookmarks = m_filteredVideos[row]->bookmarks[file].toList();
    bookmarks.removeAll(bookmark);
    m_filteredVideos[row]->bookmarks[file] = bookmarks;

    emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>() << Video::BookmarksRole);
    writeBookmarkCache(row);
}

void Collection::writeTagCache(Video *video)
{
    QString filename = video->path + "/tags.txt";
    QFile file(filename + ".tmp");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QByteArray data = video->tags.join("\n").toUtf8() + "\n";
        if (file.write(data) == data.size()) {
            QFile::remove(filename);
            file.rename(filename + ".tmp", filename);
        }
    } else {
        qWarning() << "Unable to open tag cache file for writing!:" << filename;
    }
}

void Collection::addTag(int row, QString tag)
{
    tag = tag.trimmed();
    if (m_filteredVideos[row]->tags.contains(tag))
        return;
    if (tag.isEmpty())
        return;

    m_filteredVideos[row]->tags.append(tag);
    qSort(m_filteredVideos[row]->tags);
    emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>() << Video::TagsRole);
    emit tagsUpdated();
    writeTagCache(m_filteredVideos[row]);
    updateActresses();
}

void Collection::removeTag(int row, QString tag)
{
    m_filteredVideos[row]->tags.removeAll(tag);
    emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>() << Video::TagsRole);
    emit tagsUpdated();
    if (m_filterTags.contains(tag)) {
        removeFilterTag(tag);
    }
    writeTagCache(m_filteredVideos[row]);
}


void Collection::setLastFile(int row, QString file)
{
    if (file.isEmpty() || row == -1)
        return;

    m_filteredVideos[row]->lastFile = file;
    emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>() << Video::LastFileRole);
}


void Collection::setLastPosition(int row, int position)
{
    if (row == -1 || position == -1)
        return;

    m_filteredVideos[row]->lastPosition = position;
    emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>() << Video::LastPositionRole);
}

void Collection::renameVideo(int row, QString newName)
{
    if (row == -1 || newName.isEmpty())
        return;

    newName = newName.replace('\n', '-');

    Video *oldVideo = m_filteredVideos[row];

    QDir dir(oldVideo->path);
    dir.cdUp();

    beginRemoveRows(QModelIndex(), row, row);
    dir.rename(oldVideo->name, newName);
    m_filteredVideos.removeAt(row);
    endRemoveRows();
    m_videos.removeAll(*oldVideo);

    beginInsertRows(QModelIndex(), row, row);
    dir.cd(newName);
    scan(dir);
    endInsertRows();
}

bool videoPointerCompare(Video *first, Video *second) {
    return *first < *second;
}

void Collection::rescan()
{
    setStatus("Starting scan...");
    setRescanning(true);

    beginResetModel();
    m_filteredVideos.clear();
    endResetModel();
    m_videos.clear();
    scan(QDir(Config::instance()->collectionPath()));
    qSort(m_filteredVideos.begin(), m_filteredVideos.end(), videoPointerCompare);

    writeCache();
    m_filterTags.clear();
    emit tagsUpdated();
    setRescanning(false);
    emit emptyChanged();
}

static QString scanForCovers(QString path)
{
    QDir dir(path);

    // Check if there is a separate folder with covers
    QStringList names;
    names << "*cover*" << "*Cover*";
    dir.setNameFilters(names);
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    if (dir.entryList().count() > 0) {
        dir.cd(dir.entryList().first());
    }

    QString coverPath;
    // Try to either find *front*.jpg, *cover*.jpg or just any plain *.jpg
    dir.setFilter(QDir::Files);

    if (dir.entryInfoList(QStringList("*front*.jpg")).count() > 0) {
        coverPath = dir.entryInfoList(QStringList("*front*.jpg")).first().absoluteFilePath();
    } else if (dir.entryInfoList(QStringList("*cover*.jpg")).count() > 0) {
        coverPath = dir.entryInfoList(QStringList("*cover*.jpg")).first().absoluteFilePath();
    } else if (dir.entryInfoList(QStringList(dir.dirName() + ".jpg")).count() > 0) {
        coverPath = dir.entryInfoList(QStringList(dir.dirName() + ".jpg")).first().absoluteFilePath();
    } else if (dir.exists(".vimicover.jpg")) {
        coverPath = dir.absoluteFilePath(".vimicover.jpg");
    } else if (dir.entryInfoList(QStringList("*" + dir.dirName() + "*.jpg")).count() > 0) {
        coverPath = dir.entryInfoList(QStringList("*" + dir.dirName() + "*.jpg")).first().absoluteFilePath();
    } else if (dir.entryInfoList(QStringList("*.jpg")).count() > 0) {
        coverPath = dir.entryInfoList(QStringList("*.jpg")).first().absoluteFilePath();
    } else if (dir.entryInfoList(QStringList("*.png")).count() > 0) {
        coverPath = dir.entryInfoList(QStringList("*.png")).first().absoluteFilePath();
    }

    return coverPath;
}

static QStringList scanForScreenshots(QString path)
{
    QDir dir(path);
    QMap <qint64, QString> fileMap;
    foreach(const QString &file, dir.entryList(QStringList() << ".vimiframe_*_*.jpg", QDir::Files | QDir::Hidden)) {
        fileMap[file.split("_")[1].toLong()] = file;
    }
    return fileMap.values();
}

void Collection::scan(QDir dir)
{
    QGuiApplication::instance()->processEvents();

    // QDir::AllDirs ignores name filter
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::AllDirs | QDir::Executable);

    foreach(const QFileInfo &subdir, dir.entryInfoList()) {
        scan(QDir(subdir.filePath())); // Recursively scan
    }

    dir.setNameFilters(Config::instance()->movieSuffixes());
    dir.setFilter(QDir::Files);

    // No files
    if (dir.count() == 0) {
        return;
    }

    m_videos.append(Video());
    Video &video = m_videos.last();

    video.name = dir.dirName().replace("- ", "\n");
    video.path = dir.path();
    video.cover = scanForCovers(video.path);
    video.files = dir.entryList();

    setStatus(tr("Found video:\n")  + video.name);

    // If cover not found, automatically create one
    if (video.cover.isEmpty()) {
        createCover(QUrl::fromLocalFile(video.path + "/" + video.files.first()), -1);
    }

    // Load description
    if (dir.exists("description.txt")) {
        QFile file(dir.filePath("description.txt"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            video.description = QString::fromLocal8Bit(file.readAll().trimmed());
        }
    }

    // Load tag cache
    if (dir.exists("tags.txt")) {
        QFile file(dir.filePath("tags.txt"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!file.atEnd()) {
                QString tag = QString::fromUtf8(file.readLine().simplified().toLower());
                if (!tag.isEmpty()) {
                    video.tags.append(tag); // One tag per line
                }
            }
        }
    }
    video.tags.removeDuplicates();
    qSort(video.tags);

    // Load bookmarks
    if (dir.exists("bookmarks.dat")) {
        QFile file(dir.filePath("bookmarks.dat"));
        QDataStream in(&file);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            in >> video.bookmarks;
        }
    }

    // Search for screenshots
    video.screenshots = scanForScreenshots(video.path);

    if (video.screenshots.isEmpty()) {
        foreach (QString file, video.files) {
            createScreenshots(QUrl::fromLocalFile(video.path + '/' + file));
        }
    }

    int i = 0;
    foreach(Video *video, m_filteredVideos) {
        if (video->name > m_videos.last().name) {
            beginInsertRows(QModelIndex(), i, i);
            m_filteredVideos.insert(i, &m_videos.last());
            endInsertRows();
            break;
        }
        i++;
    }
    if (i >= m_filteredVideos.size()) {
        beginInsertRows(QModelIndex(), i, i);
        m_filteredVideos.append(&m_videos.last());
        endInsertRows();
    }

    // Check if resolution already available
    if (dir.exists("resolution.txt")) {
        QFile file(dir.filePath("resolution.txt"));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            video.resolution = file.readLine().trimmed().toInt();
            video.framerate = file.readLine().trimmed().toInt();
            if (video.resolution && video.framerate) {
            } else {
                // Get resolution from file
                AVFormatContext *formatContext = 0;
                if (avformat_open_input(&formatContext, dir.entryInfoList().first().absoluteFilePath().toLocal8Bit().constData(), NULL, NULL) < 0) {
                    qWarning() << "Unable to open input" << dir.absolutePath();
                    return;
                }
                if (avformat_find_stream_info(formatContext, 0) < 0) {
                    avformat_close_input(&formatContext);
                    qWarning() << "unable to get stream info";
                    return;
                }
                int videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
                if (videoStreamIndex < 0) {
                    avformat_close_input(&formatContext);
                    qWarning() << "unable to open codec context";
                    return;
                }
                AVCodec *decoder = avcodec_find_decoder(formatContext->streams[videoStreamIndex]->codec->codec_id);
                if (!decoder) {
                    avformat_close_input(&formatContext);
                    qWarning() << "Unable to find decoder!";
                    return;
                }
                if (avcodec_open2(formatContext->streams[videoStreamIndex]->codec, decoder, 0) < 0) {
                    avcodec_close(formatContext->streams[videoStreamIndex]->codec);
                    avformat_close_input(&formatContext);
                    qWarning() << "Unable to initialize codec context";
                    return;
                }

                video.resolution = formatContext->streams[videoStreamIndex]->codec->height;
                video.framerate = int(av_q2d(formatContext->streams[videoStreamIndex]->r_frame_rate) + 0.5);

                QFile file(dir.filePath("resolution.txt"));
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    file.write(QByteArray::number(video.resolution) + '\n');
                    file.write(QByteArray::number(video.framerate) + '\n');
                }

                avcodec_close(formatContext->streams[videoStreamIndex]->codec);
                avformat_close_input(&formatContext);
            }
        }
    }

    if (video.resolution > 700) {
        addTag(m_filteredVideos.indexOf(&video), "hd");
    }
    if (video.resolution == 720) {
        addTag(m_filteredVideos.indexOf(&video), "720p");
    }
    if (video.resolution == 1080) {
        addTag(m_filteredVideos.indexOf(&video), "1080p");
    }

    if (video.resolution == 1280) {
        addTag(m_filteredVideos.indexOf(&video), "1280p");
    }
    if (video.resolution == 1920 && !video.tags.contains("1920p")) {
        addTag(m_filteredVideos.indexOf(&video), "1920p");
    }

    if (video.framerate > 50 && !video.tags.contains("60fps")) {
        addTag(m_filteredVideos.indexOf(&video), "60fps");
    }

    qSort(video.tags);

}


bool videoContainsTags(const Video &video, const QStringList &tags)
{
    foreach (const QString &tag, tags)
        if (!video.tags.contains(tag)) return false;

    return true;
}

const QStringList Collection::allTags()
{
    m_tagCounts.clear();
    foreach(const Video &video, m_videos) {
        if (!videoContainsTags(video, m_filterTags))
            continue;

        foreach(const QString &tag, video.tags) {
            if (!m_tagFilter.isEmpty() && !tag.contains(m_tagFilter)) continue;

            if (!m_tagCounts.contains(tag)) m_tagCounts[tag] = 1;
            else m_tagCounts[tag]++;
        }
    }
    QMultiMap<int, QString> reverse;
    foreach(const QString &key, m_tagCounts.keys()) {
        reverse.insert(m_tagCounts[key], key);
    }

    QStringList ret = reverse.values();
    std::reverse(ret.begin(), ret.end());
    return ret;
}

const QStringList Collection::actresses() const
{
    QStringList list;
    foreach(const QString &actress, m_actresses) {
        if (actress.contains(m_actressFilter)) list.append(actress);
    }
    return list;
}

void Collection::updateFilteredVideos()
{
    QList<Video*> filtered;
    for (int i=0; i<m_videos.count(); i++) {
        if (!m_filter.isEmpty() && !m_videos[i].name.contains(m_filter, Qt::CaseInsensitive)) {
            continue;
        }

        if (m_filterTags.isEmpty() || videoContainsTags(m_videos[i], m_filterTags)) {
            filtered.append(&m_videos[i]);
        }
    }
    if (filtered == m_filteredVideos) {
        return;
    }

    qSort(filtered);

    //TODO intelligently merge and notify about updates
    beginResetModel();
    m_filteredVideos = filtered;
    endResetModel();
}

void Collection::setFilter(QString text)
{
    m_filter = text;
    updateFilteredVideos();
}


void Collection::addFilterTag(QString tag)
{
    m_filterTags.append(tag);
    for (int i=m_filteredVideos.count()-1; i>=0; --i) {
        if (!m_filter.isEmpty() && !m_filteredVideos[i]->name.contains(m_filter))
            continue;

        if (!m_filteredVideos[i]->tags.contains(tag)) {
            beginRemoveRows(QModelIndex(), i, i);
            m_filteredVideos.removeAt(i);
            endRemoveRows();
        }
    }

    emit tagsUpdated();
    updateActresses();
}


void Collection::removeFilterTag(QString tag)
{
    m_filterTags.removeAll(tag);

    for (int i=m_videos.count()-1; i>=0; --i) {
        if (!m_filter.isEmpty() && !m_videos[i].name.contains(m_filter))
            continue;

        if (m_filteredVideos.contains(&m_videos[i]))
            continue;

        if (videoContainsTags(m_videos[i], m_filterTags)) {
            int idx=0;
            foreach(Video *video, m_filteredVideos) {
                if (video->name > m_videos[i].name)
                    break;
                else idx++;
            }

            beginInsertRows(QModelIndex(), idx, idx);
            m_filteredVideos.insert(idx, &m_videos[i]);
            endInsertRows();
        }
    }


    emit tagsUpdated();
    updateActresses();
}

bool Collection::filterTagsContains(QString tag) {
    tag = tag.left(tag.indexOf(" ("));

    return m_filterTags.contains(tag);
}

void Collection::createCover(QUrl file, qint64 position)
{
    VideoFrameDumper *dumper = new VideoFrameDumper(file);
    dumper->seek(position);
    dumper->setNum(-1);
    dumper->setAutoDelete(false);
    connect(dumper, SIGNAL(coverCreated(QString, QString)), SLOT(coverCreated(QString, QString)));
    connect(dumper, SIGNAL(statusUpdated(QString)), SLOT(setStatus(QString)));
    connect(dumper, SIGNAL(error(QString)), SLOT(snapshotError()));
    //QMetaObject::invokeMethod(dumper, "createSnapshots");
    QThreadPool::globalInstance()->start(dumper);
}

static int toCreate = 0;

void Collection::createScreenshots(QUrl file)
{
    toCreate++;
    setCreatingScreenshots(true);
    VideoFrameDumper *dumper = new VideoFrameDumper(file);
    dumper->setAutoDelete(false);
    connect(dumper, SIGNAL(screenshotsCreated(QString)), SLOT(screenshotsCreated(QString)));
    connect(dumper, SIGNAL(statusUpdated(QString)), SLOT(setStatus(QString)));
    connect(dumper, SIGNAL(error(QString)), SLOT(snapshotError()));
    //QMetaObject::invokeMethod(dumper, "createSnapshots");
    QThreadPool::globalInstance()->start(dumper);
}

void Collection::screenshotsCreated(QString path)
{
    qDebug() << "Queued:" << toCreate--;
    setCreatingScreenshots(false);
    for (int row=0; row<m_filteredVideos.count(); row++) {
        if (m_filteredVideos[row]->path == path) {
            m_filteredVideos[row]->screenshots = scanForScreenshots(path);
            emit dataChanged(index(row, 0), index(row, 0), QVector<int>() << Video::ScreenshotsRole);
            return;
        }
    }
    sender()->deleteLater();
}

void Collection::coverCreated(QString videoPath, QString coverPath)
{
    for (int row=0; row<m_filteredVideos.count(); row++) {
        if (m_filteredVideos[row]->path == videoPath) {
            m_filteredVideos[row]->cover = QString();
            emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>() << Video::CoverRole);
            m_filteredVideos[row]->cover = coverPath;
            emit dataChanged(createIndex(row, 0), createIndex(row, 0), QVector<int>() << Video::CoverRole);
            return;
        }
    }
    sender()->deleteLater();
}

void Collection::snapshotError()
{
    setCreatingScreenshots(false);
}

void Collection::setRandom(bool random)
{
    if (random == m_isRandom) return;

    beginResetModel();
    if (random) {
        for (int index = m_filteredVideos.count() - 1; index > 0; --index) {
            const int swapIndex = qrand() % (index + 1);//getLong(index + 1);
            qSwap(m_filteredVideos[index], m_filteredVideos[swapIndex]);
        }
    } else {
        qSort(m_filteredVideos);
    }
    endResetModel();
    m_isRandom = random;
    emit randomChanged();
}

void Collection::setShowOnlyUntagged(bool show)
{
    if (show == m_showOnlyUntagged) return;

    beginResetModel();

    m_filteredVideos.clear();
    if (show) {
        for (int i = 0; i<m_videos.size(); i++) {
            if (m_videos[i].tags.isEmpty()) {
                m_filteredVideos.append(&m_videos[i]);
            }
        }
    } else {
        updateFilteredVideos();
    }
    endResetModel();
}

static int diff(QString string1, QString string2)
{
    int d[string1.length()+1][string2.length() + 1];
    int cost = 0;
    for (int i=0; 0<string1.length(); i++) {

    }
}

void Collection::renameTag(QString tag, QString newTag)
{
    beginResetModel();
    for (int i=0; i<m_videos.length(); i++) {
        if (m_videos[i].tags.contains(tag)) {
            if (newTag.isEmpty()) {
                m_videos[i].tags.removeAll(tag);
            } else {
                m_videos[i].tags = m_videos[i].tags.replaceInStrings(tag, newTag);
            }
            writeTagCache(&m_videos[i]);
        }
    }
    if (m_filterTags.contains(tag)) {
        removeFilterTag(tag);
        if (!newTag.isEmpty()) {
            addFilterTag(newTag);
        }
    }
    endResetModel();
    emit tagsUpdated();
}
