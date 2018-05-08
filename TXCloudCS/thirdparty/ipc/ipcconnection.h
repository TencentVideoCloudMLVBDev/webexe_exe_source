/**************************************************************************
    Copyright:      Copyright ? 1998 - 2017 Tencent. All Rights Reserved
    Created:        2017-4-6 11:30:41
    Filename:       ipcconnection.h

    Description:
***************************************************************************/

#ifndef __IPCCONNECTION_H__
#define __IPCCONNECTION_H__

#include "ipcchannel.h"

/**************************************************************************/

class IPCConnection;

class IIPCConnectionCallback
{
public:
    virtual ~IIPCConnectionCallback() {}

    // 连接的另一方主动退出或者异常crash，均会触发该回调
    // 注意，不要在onClose回调中释放IPCConnection，否则引起死锁，也就是千万不要在onClose回调中delete connection
    virtual void onClose(IPCConnection* connection) = 0;

    // 接收到数据，不要做耗时操作
    virtual DWORD onRecv(IPCConnection* connection, const void* data, size_t dataSize) = 0;

    // 打印log
    virtual void onLog(LogLevel level, const char* content) = 0;
};

class IPCConnection
    : public IIPCSenderCallback
    , public IIPCRecieverCallback
{
    friend class IPCHandShake;
public:
    explicit IPCConnection(IIPCConnectionCallback* callback);
    virtual ~IPCConnection();

    DWORD send(const void* data, size_t dataSize, DWORD timeout);   // 阻塞函数
    DWORD close();
protected:
    DWORD openReciever(DWORD appID, DWORD aRecieverPid, DWORD aSenderPid, DWORD timestamp, DWORD suggestSize);
    DWORD closeReciever();

    DWORD openSender(DWORD appID, DWORD aRecieverPid, DWORD aSenderPid, DWORD timestamp, DWORD suggestSize);
    DWORD closeSender(bool notifyPeer);

    virtual void onIPCSenderLog(LogLevel level, const char* content);
    virtual void onIPCSenderClose();

    virtual void onIPCRecieverLog(LogLevel level, const char* content);
    virtual void onIPCRecieverClose();
    virtual DWORD onIPCReciverRecv(const void* data, size_t dataSize);
private:
    IIPCConnectionCallback*                 m_callback;
    IPCSenderConnection*                    m_sender;
    IPCRecieverConnection*                  m_reciver;
};

/**************************************************************************/
#endif /* __IPCCONNECTION_H__ */
