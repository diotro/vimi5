// Copyright 2009 cwk

#ifndef COLLECTION_H
#define COLLECTION_H

#include "config.h"
#include "video.h"
#include "coverloader.h"
#include <QDir>
#include <QImage>
#include <QAbstractTableModel>
#include <QHash>
#include <QSet>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

class Collection : public QAbstractTableModel
{

    Q_OBJECT

public:
    Collection(QObject *parent);
    ~Collection();

    static void addTag(const QString &video, const QString &tag);
    static void removeTag(const QString &video, const QString &tag);
    static QStringList getTags(const QString& videoName = QString());
    static QStringList getFiles(const QString& videoName);
    static QString getPath(const QString &videoName);
    static QPixmap getCover(const QString &videoName, int maxSize = Config::maxCoverSize());
    static void scanForCovers(const QString &videoName);

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex & = QModelIndex()) const { return m_videoNames.size(); }
    int columnCount(const QModelIndex &) const { return 3; }
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    static QMutex launchMutex;
    static QWaitCondition launchWaiter;
    static bool launched;

signals:
    void updated();
    void statusUpdated(const QString &text);
    void repaintCover(int row, const QModelIndex &parent);

public slots:
    void rescan();
private slots:
    void loadCache();
    void coverLoaded(const QString &name);

private:
    void scan(QDir directory);
    void addVideo(Video *video);


    static QHash<QString, Video*> m_videos;
    static QStringList m_videoNames;
    QThread *m_thread;

    QStringList m_cachedVideoDirectories;
    CoverLoader *m_coverLoader;
    //QHash<int, QModelIndex&> m_indices;
};

#endif // COLLECTION_H
