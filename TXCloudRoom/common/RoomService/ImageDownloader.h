#pragma once

#include <QObject>
#include <memory>
#include <thread>
#include <string>

#include "HttpClient.h"

class ImageDownloader : public QObject
{
    Q_OBJECT

public:
    ImageDownloader();
    virtual ~ImageDownloader();

    void setProxy(const std::string& ip, unsigned short port);

    void download(const std::wstring& url, long ms);
    void close();
signals:
    void downloadFinished(bool success, QByteArray image);
protected:
    void onRequestImage();
protected:
    virtual void timerEvent(QTimerEvent *event);
private:
    HttpClient m_httpClient;
    std::wstring m_url;
    std::unique_ptr<std::thread> m_thread;
    int m_timerID;
};

