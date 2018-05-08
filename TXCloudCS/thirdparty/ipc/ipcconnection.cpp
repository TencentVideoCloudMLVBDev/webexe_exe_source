/**************************************************************************
    Copyright:      Copyright ? 1998 - 2017 Tencent. All Rights Reserved
    Created:        2017-4-6 11:30:44
    Filename:       ipcconnection.cpp

    Description:
***************************************************************************/

#include "ipcconnection.h"

#include <memory>

/**************************************************************************/

IPCConnection::IPCConnection(IIPCConnectionCallback* callback)
    : m_callback(callback)
    , m_sender(NULL)
    , m_reciver(NULL)
{
    assert(NULL != m_callback);
}

IPCConnection::~IPCConnection()
{
    closeReciever();
    closeSender(false);

    if (NULL != m_reciver)
    {
        delete m_reciver;
        m_reciver = NULL;
    }

    if (NULL != m_sender)
    {
        delete m_sender;
        m_sender = NULL;
    }
}

DWORD IPCConnection::send(const void* data, size_t dataSize, DWORD timeout)
{
    if (NULL != m_sender)
    {
        return m_sender->send(data, dataSize, timeout);
    }
    else
    {
        m_callback->onLog(LOG_INFO_LEVEL, "IPCConnection::send, m_sender is NULL.");

        return EcFailed;
    }
}

DWORD IPCConnection::close()
{
    closeReciever();
    closeSender(true);

    return ERROR_SUCCESS;
}

DWORD IPCConnection::openReciever(DWORD appID, DWORD aRecieverPid, DWORD aSenderPid, DWORD timestamp, DWORD suggestSize)
{
    if (NULL != m_reciver)
    {
        delete m_reciver;
        m_reciver = NULL;
    }

    m_callback->onLog(LOG_INFO_LEVEL, "IPCConnection::openReciever");

    std::auto_ptr<IPCRecieverConnection> reciever(new IPCRecieverConnection(this, suggestSize));
    if (NULL == reciever.get())
    {
        m_callback->onLog(LOG_ERROR_LEVEL, "IPCConnection::openReciever, out of memory.");
        return ERROR_OUTOFMEMORY;
    }

    DWORD ret = reciever->listen(appID, aRecieverPid, aSenderPid, timestamp);
    if (ERROR_SUCCESS != ret)
    {
        m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCConnection::openReciever, listen failed: %lu.", ret).c_str());
        return ret;
    }

    m_reciver = reciever.release();

    return ERROR_SUCCESS;
}

DWORD IPCConnection::closeReciever()
{
    m_callback->onLog(LOG_INFO_LEVEL, "IPCConnection::closeReciever");

    if (NULL != m_reciver)
    {
        m_reciver->close();
    }

    return ERROR_SUCCESS;
}

DWORD IPCConnection::openSender(DWORD appID, DWORD aRecieverPid, DWORD aSenderPid, DWORD timestamp, DWORD suggestSize)
{
    if (NULL != m_sender)
    {
        delete m_sender;
        m_sender = NULL;
    }

    m_callback->onLog(LOG_INFO_LEVEL, "IPCConnection::openSender");

    std::auto_ptr<IPCSenderConnection> sender(new IPCSenderConnection(this, suggestSize));
    if (NULL == sender.get())
    {
        m_callback->onLog(LOG_ERROR_LEVEL, "IPCConnection::openSender, out of memory.");
        return ERROR_OUTOFMEMORY;
    }

    DWORD ret = sender->connect(appID, aRecieverPid, aSenderPid, timestamp);
    if (ERROR_SUCCESS != ret)
    {
        m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCConnection::openSender, connect failed: %lu.", ret).c_str());
        return ret;
    }

    m_sender = sender.release();

    return ERROR_SUCCESS;
}

DWORD IPCConnection::closeSender(bool notifyPeer)
{
    m_callback->onLog(LOG_INFO_LEVEL, "IPCConnection::closeSender");

    if (NULL != m_sender)
    {
        m_sender->close(notifyPeer);
    }

    return ERROR_SUCCESS;
}

void IPCConnection::onIPCSenderLog(LogLevel level, const char* content)
{
    m_callback->onLog(level, content);
}

void IPCConnection::onIPCSenderClose()
{
    m_callback->onClose(this);
}

void IPCConnection::onIPCRecieverLog(LogLevel level, const char* content)
{
    m_callback->onLog(level, content);
}

void IPCConnection::onIPCRecieverClose()
{
    m_callback->onClose(this);
}

DWORD IPCConnection::onIPCReciverRecv(const void* data, size_t dataSize)
{
    return m_callback->onRecv(this, data, dataSize);
}

/**************************************************************************/
