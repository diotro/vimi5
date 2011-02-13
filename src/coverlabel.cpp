#include "coverlabel.h"
#include "collection.h"
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

CoverLabel::CoverLabel(QWidget *parent) :
    QLabel(parent),
    m_dir(0)
{
    setCursor(Qt::PointingHandCursor);
    connect(&m_timer, SIGNAL(timeout()), SLOT(nextImage()));
}
CoverLabel::~CoverLabel()
{
    delete m_dir;
}

void CoverLabel::setVideo(const QString &video)
{
    m_videoName = video;
    setPixmap(Collection::getCover(m_videoName, 250));
    m_path = Collection::getPath(m_videoName);
}

void CoverLabel::leaveEvent(QEvent *)
{
    m_timer.stop();
    setPixmap(Collection::getCover(m_videoName, 250));
}

void CoverLabel::enterEvent(QEvent *)
{
    if (m_timer.isActive())
        return; // WTF

    QStringList nameFilters;
    nameFilters << "*.jpg" << "*.png";
    delete m_dir;
    m_dir = new QDirIterator(m_path, nameFilters, QDir::Files, QDirIterator::NoIteratorFlags);
    m_timer.setInterval(0);
    m_timer.start();
}

void CoverLabel::nextImage()
{
    QImage image;
    int height, width, ratio;
    // Sanity check sizes
    do {
        image = QImage(m_dir->next());
        height = image.height();
        width = image.width();
        if (height == 0 || width == 0) continue;

        ratio = qMax(width, height) / qMin(width, height);
    } while (m_dir->hasNext() &&
            (height > 1024 || width > 1024 ||
            height < 100 || width < 100 ||
            ratio > 2));

    if (!m_dir->hasNext()) { // restart
        m_timer.stop();
        enterEvent();
        return;
    }

    if (image.width() > this->width()) {
        float factor = (float)this->width() / image.width();
        image = Video::quickScale(image, image.width() * factor, image.height() * factor);
    }
    setPixmap(QPixmap::fromImage(image));
    m_timer.setInterval(1000);
}

void CoverLabel::mousePressEvent(QMouseEvent *)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_dir->filePath()));
}
