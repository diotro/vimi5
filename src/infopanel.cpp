// Copyright 2009 cwk

#include "collection.h"
#include "infopanel.h"
#include <QDesktopServices>
#include <QDebug>
#include <QScrollArea>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

InfoPanel::InfoPanel(QWidget *parent) :
    QGroupBox(parent)
{
    hide();

    //setTitle("Info");

    m_title = new QLabel("");
    m_title->setWordWrap(true);
    m_cover = new CoverLabel(this);

    m_tags = new QLabel("");
    m_files = new QLabel("");
    m_path = new QLabel("");
    QPushButton *tagEditButton = new QPushButton("&Edit tags...");
    QPushButton *tagFetchButton = new QPushButton("&Fetch tags...");
    QPushButton *createCoversButton = new QPushButton("&Create covers...");

    setLayout(new QVBoxLayout);
    layout()->addWidget(m_title);
    layout()->addWidget(m_cover);
    layout()->addWidget(m_tags);
    layout()->addWidget(m_files);
    layout()->addWidget(m_path);
    layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    layout()->addWidget(tagEditButton);
    layout()->addWidget(tagFetchButton);
    layout()->addWidget(createCoversButton);

    connect(m_files, SIGNAL(linkActivated(QString)), SLOT(launchFile(QString)));
    connect(m_path, SIGNAL(linkActivated(QString)), SLOT(launchFile(QString)));
    connect(m_tags, SIGNAL(linkActivated(QString)), SIGNAL(selectedTag(QString)));
    connect(tagEditButton, SIGNAL(clicked()), SIGNAL(editTags()));
    connect(tagFetchButton, SIGNAL(clicked()), SIGNAL(fetchTags()));
    connect(createCoversButton, SIGNAL(clicked()), SIGNAL(createCovers()));
}

void InfoPanel::setInfo(const QString &title)
{
    QStringList tags = Collection::getTags(title);
    m_videoName = title;

    m_title->setText("<h3>" + title + "</h3>");

    if (!tags.empty()) {
        // List of tags
        QString tagHtml = "<b>Tags:</b><ul>";
        foreach (QString tag, tags) tagHtml += "<li><a href='" + tag + "'>" + tag + "</a></li>";
        tagHtml += "</ul>";
        m_tags->setText(tagHtml);
    } else {
        m_tags->clear();
    }

    // List of files
    QStringList files = Collection::getFiles(title);
    QString path = Collection::getPath(title);

    QString fileHtml = "<b>Files:</b><ul>";
    foreach (QString file, files)
        fileHtml += "<li><a href='" + QUrl::toPercentEncoding(path + "/" + file) + "'>" + file + "</a></li>";
    fileHtml += "</ul>";
    m_files->setText(fileHtml);

    m_path->setText("<br /> <a href='" + QUrl::toPercentEncoding(path) + "'>Click to open in file manager</a>");

    m_cover->setVideo(title);
    m_cover->repaint();

    show();
}

void InfoPanel::launchFile(const QString &file)
{
    QUrl url = QUrl::fromLocalFile(QUrl::fromPercentEncoding(file.toLocal8Bit()));
    QDesktopServices::openUrl(url);//QUrl::fromPercentEncoding(file.toLocal8Bit()));
}
