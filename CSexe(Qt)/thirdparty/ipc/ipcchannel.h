/**************************************************************************
    Copyright:      Copyright ? 1998 - 2017 Tencent. All Rights Reserved
    Created:        2017-4-6 11:30:41
    Filename:       ipcreciver.h

    Description:
***************************************************************************/

#ifndef __IPCCHANNEL_H__
#define __IPCCHANNEL_H__

#include "utility.h"

/**************************************************************************/

enum LogLevel
{
    LOG_TRACK_LEVEL = 0,
    LOG_INFO_LEVEL = 1,
    LOG_WARNING_LEVEL = 2,
    LOG_ERROR_LEVEL = 3,
};

class IIPCSenderCallback
{
public:
    virtual ~IIPCSenderCallback() {};
    virtual void onIPCSenderLog(LogLevel level, const char* content) = 0;
    virtual void onIPCSenderClose() = 0;
};

class IPCSenderConnection
{
public:
    explicit IPCSenderConnection(IIPCSenderCallback* callback, size_t maxMemorySize);
    virtual ~IPCSenderConnection();

    DWORD recieverPid() const;
    DWORD senderPid() const;

    DWORD connect(DWORD appID, DWORD aRecieverPid, DWORD aSenderPid, DWORD timestamp);
    DWORD close(bool notifyPeer);

    DWORD send(const void* data, size_t dataSize, DWORD timeout);   // ×èÈûº¯Êý
private:
    IIPCSenderCallback*                 m_callback;
    DWORD                               m_recieverPid;
    DWORD                               m_senderPid;
    CResource<HANDLE>                   m_targetProcess;
    CResource<HANDLE>                   m_closeEvent;

    CResource<HANDLE>                   m_fileMap;
    LPVOID                              m_sharedMemory;
    const size_t                        m_maxMemorySize;
    CResource<HANDLE>                   m_mutex;
    CResource<HANDLE>                   m_reqEvent;
    CResource<HANDLE>                   m_respEvent;
};

class IIPCRecieverCallback
{
public:
    virtual ~IIPCRecieverCallback() {};
    virtual void onIPCRecieverLog(LogLevel level, const char* content) = 0;
    virtual void onIPCRecieverClose() = 0;
    virtual DWORD onIPCReciverRecv(const void* data, size_t dataSize) = 0;
};

class IPCRecieverConnection
{
public:
    explicit IPCRecieverConnection(IIPCRecieverCallback* callback, size_t maxMemorySize);
    virtual ~IPCRecieverConnection();

    DWORD recieverPid() const;
    DWORD senderPid() const;

    DWORD listen(DWORD appID, DWORD aRecieverPid, DWORD aSenderPid, DWORD timestamp);
    DWORD close();
protected:
    static unsigned __stdcall on_reciever_thread(void* arg);
    void handle_loop();
private:
    IIPCRecieverCallback*               m_callback;
    DWORD                               m_recieverPid;
    DWORD                               m_senderPid;
    CResource<HANDLE>                   m_targetProcess;
    CResource<HANDLE>                   m_closeEvent;

    CResource<HANDLE>                   m_fileMap;
    LPVOID                              m_sharedMemory;
    const size_t                        m_maxMemorySize;
    CResource<HANDLE>                   m_recieverThread;
    CResource<HANDLE>                   m_reqEvent;
    CResource<HANDLE>                   m_respEvent;
};

/**************************************************************************/
#endif /* __IPCCHANNEL_H__ */
