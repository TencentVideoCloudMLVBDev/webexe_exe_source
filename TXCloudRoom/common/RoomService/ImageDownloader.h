#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QObject>

class ImageDownloader : public QObject
{
    Q_OBJECT

public:
    ImageDownloader();
    virtual ~ImageDownloader();

    void download(const QString& url, long ms);
    void close();
signals:
    void downloadFinished(bool success, QByteArray image);
protected slots:
    void onReplyFinished(QNetworkReply *reply);
protected:
    virtual void timerEvent(QTimerEvent *event);
private:
    QNetworkAccessManager m_networkManager;
    QNetworkRequest m_request;
    int m_timerID;
};

