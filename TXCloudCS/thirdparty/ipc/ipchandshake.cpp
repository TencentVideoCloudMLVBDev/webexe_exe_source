/**************************************************************************
    Copyright:      Copyright ? 1998 - 2017 Tencent. All Rights Reserved
    Created:        2017-4-6 11:30:44
    Filename:       ipcconnection.cpp

    Description:
***************************************************************************/

#include "ipchandshake.h"

#include <memory>

#pragma pack(push)
#pragma pack(1)

struct IPCHandShakeData
{
    DWORD           m_flag;         // 标记三次握手的顺序
    DWORD           m_serverPid;
    DWORD           m_clientPid;
    DWORD           m_timestamp;
    DWORD           m_suggestSize;  // 推荐的共享内存大小
};

#pragma pack(pop)

#if defined(_DEBUG) || defined(DEBUG)

static const DWORD g_timetout = 1000 * 60 * 5;  // 设置大点，方便调试

#else // _DEBUG

static const DWORD g_timetout = 500;

#endif

/**************************************************************************/

IPCHandShake::IPCHandShake(DWORD appID, IIPCHandShakeCallback* callback)
    : m_appID(appID)
    , m_callback(callback)
    , m_listener(NULL)
{
    assert(NULL != m_callback);
}

IPCHandShake::~IPCHandShake()
{
    if (NULL != m_listener)
    {
        m_listener->close();

        delete m_listener;
        m_listener = NULL;
    }
}

DWORD IPCHandShake::listen()
{
    assert(NULL == m_listener);

    m_callback->onLog(LOG_INFO_LEVEL, "IPCHandShake::listen");

    std::auto_ptr<IPCRecieverConnection> reciver(new IPCRecieverConnection(this, sizeof(IPCHandShakeData)));
    if (NULL == reciver.get())
    {
        m_callback->onLog(LOG_ERROR_LEVEL, "IPCHandShake::listen, out of memory.");
        return ERROR_OUTOFMEMORY;
    }

    DWORD ret = reciver->listen(m_appID, 0, 0, 0);  // 握手Connection监听者
    if (ERROR_SUCCESS != ret)
    {
        m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::listen, listen failed: %lu.", ret).c_str());
        return ret;
    }

    m_listener = reciver.release();

    // 握手连接监听

    return ERROR_SUCCESS;
}

DWORD IPCHandShake::connect(DWORD suggestSize)
{
    m_callback->onLog(LOG_INFO_LEVEL, "IPCHandShake::connect");

    std::auto_ptr<IPCSenderConnection> sender(new IPCSenderConnection(this, sizeof(IPCHandShakeData)));
    if (NULL == sender.get())
    {
        m_callback->onLog(LOG_ERROR_LEVEL, "IPCHandShake::connect, out of memory.");
        return ERROR_OUTOFMEMORY;
    }

    DWORD ret = sender->connect(m_appID, 0, 0, 0);
    if (ERROR_SUCCESS != ret)
    {
        m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::connect, connect failed: %lu.", ret).c_str());
        return ret;
    }

    std::auto_ptr<IPCRecieverConnection> reciever(new IPCRecieverConnection(this, sizeof(IPCHandShakeData)));
    if (NULL == reciever.get())
    {
        m_callback->onLog(LOG_ERROR_LEVEL, "IPCHandShake::connect, out of memory.");
        return ERROR_OUTOFMEMORY;
    }

    ret = reciever->listen(m_appID, 1, 1, 1);
    if (ERROR_SUCCESS != ret)
    {
        m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::connect, listen failed: %lu.", ret).c_str());
        return ret;
    }

    IPCHandShakeData handShakeData = { 0 };
    handShakeData.m_flag = 0;
    handShakeData.m_serverPid = 0;
    handShakeData.m_clientPid = ::GetCurrentProcessId();
    handShakeData.m_timestamp = ::GetTickCount();
    handShakeData.m_suggestSize = suggestSize;

    // 握手连接开始

    ret = sender->send(&handShakeData, sizeof(handShakeData), g_timetout);
    if (ERROR_SUCCESS != ret)
    {
        m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::connect, send failed: %lu.", ret).c_str());
        return ret;
    }

    // 握手连接结束

    return ERROR_SUCCESS;
}

DWORD IPCHandShake::close()
{
    if (NULL != m_listener)
    {
        m_listener->close();
    }

    return ERROR_SUCCESS;
}

void IPCHandShake::onIPCSenderLog(LogLevel level, const char* content)
{
    m_callback->onLog(level, content);
}

void IPCHandShake::onIPCSenderClose()
{

}

void IPCHandShake::onIPCRecieverLog(LogLevel level, const char* content)
{
    m_callback->onLog(level, content);
}

void IPCHandShake::onIPCRecieverClose()
{

}

DWORD IPCHandShake::onIPCReciverRecv(const void* data, size_t dataSize)
{
    IPCHandShakeData handShakeData = *reinterpret_cast<const IPCHandShakeData*>(data);
    switch (handShakeData.m_flag)
    {
    case 0:
    {
        // Server 一次握手

        m_callback->onLog(LOG_INFO_LEVEL, "IPCHandShake::onIPCReciverRecv, first handshake.");

        std::auto_ptr<IPCSenderConnection> sender(new IPCSenderConnection(this, sizeof(IPCHandShakeData)));
        if (NULL == sender.get())
        {
            m_callback->onLog(LOG_ERROR_LEVEL, "IPCHandShake::onIPCReciverRecv, out of memory.");
            return ERROR_OUTOFMEMORY;
        }

        DWORD ret = sender->connect(m_appID, 1, 1, 1);
        if (ERROR_SUCCESS != ret)
        {
            m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::onIPCReciverRecv, connect failed: %lu.", ret).c_str());
            return ret;
        }

        IPCConnection* newConnection = m_callback->onCreateBegin();
        if (NULL == newConnection)
        {
            m_callback->onCreateEnd(false, newConnection);

            m_callback->onLog(LOG_INFO_LEVEL, "IPCHandShake::onIPCReciverRecv, onCreateBegin return NULL.");
            return ret;
        }

        handShakeData.m_flag = 1;
        handShakeData.m_serverPid = ::GetCurrentProcessId();

        ret = newConnection->openReciever(m_appID, handShakeData.m_serverPid, handShakeData.m_clientPid, handShakeData.m_timestamp, handShakeData.m_suggestSize);
        if (ERROR_SUCCESS != ret)
        {
            m_callback->onCreateEnd(false, newConnection);

            m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::onIPCReciverRecv, openReciever failed: %lu.", ret).c_str());
            return ret;
        }

        ret = sender->send(&handShakeData, sizeof(handShakeData), g_timetout);    // 二次握手开始
        if (ERROR_SUCCESS != ret)   // 二次握手结束
        {
            m_callback->onCreateEnd(false, newConnection);

            m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::onIPCReciverRecv, send failed: %lu.", ret).c_str());
            return ret;
        }

        ret = newConnection->openSender(m_appID, handShakeData.m_clientPid, handShakeData.m_serverPid, handShakeData.m_timestamp, handShakeData.m_suggestSize);
        if (ERROR_SUCCESS != ret)
        {
            m_callback->onCreateEnd(false, newConnection);

            m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::onIPCReciverRecv, openSender failed: %lu.", ret).c_str());
            return ret;
        }

        m_callback->onCreateEnd(true, newConnection);

        sender->close(false);
    }
        break;
    case 1:
    {
        // Client 二次握手

        m_callback->onLog(LOG_INFO_LEVEL, "IPCHandShake::onIPCReciverRecv, second handshake.");

        IPCConnection* newConnection = m_callback->onCreateBegin();
        if (NULL == newConnection)
        {
            m_callback->onCreateEnd(false, newConnection);

            m_callback->onLog(LOG_ERROR_LEVEL, "IPCHandShake::onIPCReciverRecv, out of memory.");
            return ERROR_SUCCESS;
        }

        DWORD ret = newConnection->openReciever(m_appID, handShakeData.m_clientPid, handShakeData.m_serverPid, handShakeData.m_timestamp, handShakeData.m_suggestSize);
        if (ERROR_SUCCESS != ret)
        {
            m_callback->onCreateEnd(false, newConnection);

            m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::onIPCReciverRecv, openReciever failed: %lu.", ret).c_str());
            return ret;
        }

        ret = newConnection->openSender(m_appID, handShakeData.m_serverPid, handShakeData.m_clientPid, handShakeData.m_timestamp, handShakeData.m_suggestSize);
        if (ERROR_SUCCESS != ret)
        {
            m_callback->onCreateEnd(false, newConnection);

            m_callback->onLog(LOG_ERROR_LEVEL, ipcFormat("IPCHandShake::onIPCReciverRecv, openSender failed: %lu.", ret).c_str());
            return ret;
        }

        m_callback->onCreateEnd(true, newConnection);
    }
        break;
    default:
        break;
    }

    return ERROR_SUCCESS;
}

/**************************************************************************/
