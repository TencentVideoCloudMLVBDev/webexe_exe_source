#include "ImageDownloader.h"
#include "log.h"

#include <QPixmap>
#include <assert.h>

ImageDownloader::ImageDownloader()
    : m_httpClient(L"User-Agent")
    , m_url(L"")
    , m_thread(nullptr)
    , m_timerID(-1)
{

}

ImageDownloader::~ImageDownloader()
{
    close();

    assert(true == m_thread->joinable());
    m_thread->join();
}

void ImageDownloader::setProxy(const std::string& ip, unsigned short port)
{
    m_httpClient.setProxy(ip, port);
}

void ImageDownloader::download(const std::wstring& url, long ms)
{
    m_url = url;
    m_timerID = startTimer(ms);
    m_thread.reset(new std::thread(&ImageDownloader::onRequestImage, this));
}

void ImageDownloader::close()
{
    m_httpClient.http_close();

    if (-1 != m_timerID)
    {
        killTimer(m_timerID);
        m_timerID = -1;
    }
}

void ImageDownloader::onRequestImage()
{
    std::string respData;
    DWORD ret = m_httpClient.http_get(m_url, std::vector<std::wstring>(), respData);
    close();

    LINFO(L"m_url: %s, ret: %lu", m_url.c_str(), ret);

    if (ERROR_SUCCESS == ret)
    {
        emit downloadFinished(true, QByteArray(respData.c_str(), respData.size()));
    }
    else
    {
        emit downloadFinished(false, QByteArray());
    }
}

void ImageDownloader::timerEvent(QTimerEvent *event)
{
    if (-1 != m_timerID)
    {
        close();

        emit downloadFinished(false, QByteArray());
    }
}
