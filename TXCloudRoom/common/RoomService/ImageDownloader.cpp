#include "ImageDownloader.h"

#include <QPixmap>

ImageDownloader::ImageDownloader()
    : m_networkManager(this)
    , m_request()
    , m_timerID(-1)
{
    connect(&m_networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onReplyFinished(QNetworkReply*)));
}

ImageDownloader::~ImageDownloader()
{
    close();
}

void ImageDownloader::download(const QString& url, long ms)
{
    m_request.setUrl(url);
    m_networkManager.get(m_request);

    m_timerID = startTimer(ms);
}

void ImageDownloader::close()
{
    if (-1 != m_timerID)
    {
        killTimer(m_timerID);
        m_timerID = -1;
    }
}

void ImageDownloader::onReplyFinished(QNetworkReply *reply)
{
    close();

    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();

        emit downloadFinished(true, bytes);
    }
    else
    {
        emit downloadFinished(false, QByteArray());
    }
}

void ImageDownloader::timerEvent(QTimerEvent *event)
{
    close();

    emit downloadFinished(false, QByteArray());
}
