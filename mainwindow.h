// Copyright 2009 cwk

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "collection.h"
#include "infopanel.h"
#include "videofilterproxymodel.h"
#include <QLineEdit>
#include <QListView>
#include <QMainWindow>
#include <QSqlQueryModel>
#include <QStandardItemModel>
#include <QTreeView>

class MainWindow : public QMainWindow {

    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void updateVideoFilter(QStandardItem *tag);
    void updateTagModel();
    void updateInfoPanel(const QModelIndex&);
    void getCollectionPath();

private:
    QListView *m_tagView;
    QTreeView *m_videoView;
    QStandardItemModel *m_tagModel;
    QSortFilterProxyModel *m_tagFilterModel;
    QLineEdit *m_tagFilterEdit;
    QLineEdit *m_videoFilterEdit;
    VideoFilterProxyModel *m_videoModel;
    Collection *m_collection;
    InfoPanel *m_infoPanel;
};

#endif // MAINWINDOW_H
