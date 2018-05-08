/**************************************************************************
    Copyright:      Copyright ? 1998 - 2018 Tencent. All Rights Reserved
    Created:        2017-4-6 11:30:41
    Filename:       ipchandshake.h

    Description:
***************************************************************************/

#ifndef __IPCHANDSHAKE_H__
#define __IPCHANDSHAKE_H__

#include "ipcconnection.h"

/**************************************************************************/

class IIPCHandShakeCallback
{
public:
    virtual ~IIPCHandShakeCallback() {}

    // 派生类负责创建IPCConnection实例，若要取消连接，则返回NULL
    virtual IPCConnection* onCreateBegin() = 0;

    // connection由onCreateBegin创建得到，若创建连接成功，则success是true，否则success是false
    virtual void onCreateEnd(bool success, IPCConnection* connection) = 0;

    // 打印log
    virtual void onLog(LogLevel level, const char* content) = 0;
};

class IPCHandShake
    : public IIPCSenderCallback
    , public IIPCRecieverCallback
{
public:
    explicit IPCHandShake(DWORD appID, IIPCHandShakeCallback* callback);    // appID，区分不同应用的IPC
    virtual ~IPCHandShake();

    DWORD listen();                     // 监听握手连接的请求
    DWORD connect(DWORD suggestSize);   // 请求握手连接, suggestSize表示推荐的共享内存大小，单位Byte
    DWORD close();
protected:
    virtual void onIPCSenderLog(LogLevel level, const char* content);
    virtual void onIPCSenderClose();

    virtual void onIPCRecieverLog(LogLevel level, const char* content);
    virtual void onIPCRecieverClose();
    virtual DWORD onIPCReciverRecv(const void* data, size_t dataSize);
private:
    DWORD                                   m_appID;
    IIPCHandShakeCallback*                  m_callback;
    IPCRecieverConnection*                  m_listener;
};

/**************************************************************************/
#endif /* __IPCHANDSHAKE_H__ */
