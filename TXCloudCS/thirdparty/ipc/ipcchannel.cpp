/**************************************************************************
    Copyright:      Copyright ? 1998 - 2017 Tencent. All Rights Reserved
    Created:        2017-4-7 10:13:28
    Filename:       ipcchannel.cpp

    Description:
***************************************************************************/

#include "ipcchannel.h"

#include <process.h>

/**************************************************************************/

IPCSenderConnection::IPCSenderConnection(IIPCSenderCallback* callback, size_t maxMemorySize)
    : m_callback(callback)
    , m_targetProcess()
    , m_closeEvent()
    , m_fileMap()
    , m_sharedMemory(NULL)
    , m_maxMemorySize(maxMemorySize + sizeof(DWORD))
    , m_mutex()
    , m_reqEvent()
    , m_respEvent()
{
    assert(NULL != m_callback);
}

IPCSenderConnection::~IPCSenderConnection()
{
    close(false);

    if (NULL != m_sharedMemory)
    {
        ::UnmapViewOfFile(m_sharedMemory);
        m_sharedMemory = NULL;
    }
}

DWORD IPCSenderConnection::recieverPid() const
{
    return m_recieverPid;
}

DWORD IPCSenderConnection::senderPid() const
{
    return m_senderPid;
}

DWORD IPCSenderConnection::connect(DWORD appID, DWORD aRecieverPid, DWORD aSenderPid, DWORD timestamp)
{
    assert(NULL == m_targetProcess && NULL == m_sharedMemory);

    m_callback->onIPCSenderLog(LOG_INFO_LEVEL
        , ipcFormat("IPCSenderConnection::connect, appID: %lu, aRecieverPid: %lu, aSenderPid: %lu, timestamp: %lu."
            , appID, aRecieverPid, aSenderPid, timestamp).c_str());

    // 握手连接的Connection，监听自身进程
    DWORD targetPid = aSenderPid;
    if ((0 == aRecieverPid && 0 == aSenderPid && 0 == timestamp)
        || (1 == aRecieverPid && 1 == aSenderPid && 1 == timestamp))
    {
        targetPid = ::GetCurrentProcessId();
    }

    CResource<HANDLE> targetProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPid);
    if (NULL == targetProcess)
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL, ipcFormat("IPCSenderConnection::connect, OpenProcess failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    wchar_t nameBuffer[MAX_PATH] = { 0 };
    int count = ::swprintf_s(nameBuffer, L"close_event_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring closeEventName(nameBuffer, count);
    CResource<HANDLE> closeEvent = ::OpenEventW(EVENT_ALL_ACCESS, FALSE, closeEventName.c_str());
    if (NULL == closeEvent)
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL, ipcFormat("IPCSenderConnection::connect, OpenEventW failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    count = ::swprintf_s(nameBuffer, L"file_map_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring fileMapName(nameBuffer, count);
    CResource<HANDLE> fileMap = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, fileMapName.c_str());
    if (NULL == fileMap)
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL, ipcFormat("IPCSenderConnection::connect, OpenFileMappingW failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    count = ::swprintf_s(nameBuffer, L"mutex_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring mutexName(nameBuffer, count);
    CResource<HANDLE> mutex = ::CreateMutexW(NULL, FALSE, mutexName.c_str());   // 由Client端创建锁，对共享内存锁保护
    if (NULL == mutex)
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL, ipcFormat("IPCSenderConnection::connect, CreateMutexW failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    count = ::swprintf_s(nameBuffer, L"req_event_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring reqEventName(nameBuffer, count);
    CResource<HANDLE> reqEvent = ::OpenEventW(EVENT_ALL_ACCESS, FALSE, reqEventName.c_str());
    if (NULL == reqEvent)
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL, ipcFormat("IPCSenderConnection::connect, OpenEventW failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    count = ::swprintf_s(nameBuffer, L"resp_event_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring respEventName(nameBuffer, count);
    CResource<HANDLE> respEvent = ::OpenEventW(EVENT_ALL_ACCESS, FALSE, respEventName.c_str());
    if (NULL == respEvent)
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL, ipcFormat("IPCSenderConnection::connect, OpenEventW failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    LPVOID sharedMemory = ::MapViewOfFile(fileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, m_maxMemorySize);
    if (NULL == sharedMemory)
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL, ipcFormat("IPCSenderConnection::connect, MapViewOfFile failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    m_targetProcess.Attach(targetProcess.Detach());
    m_closeEvent.Attach(closeEvent.Detach());
    m_fileMap.Attach(fileMap.Detach());
    m_mutex.Attach(mutex.Detach());
    m_reqEvent.Attach(reqEvent.Detach());
    m_respEvent.Attach(respEvent.Detach());
    m_sharedMemory = sharedMemory;

    m_recieverPid = aRecieverPid;
    m_senderPid = aSenderPid;

    m_callback->onIPCSenderLog(LOG_INFO_LEVEL, "IPCSenderConnection::connect successfully.");

    return ERROR_SUCCESS;
}

DWORD IPCSenderConnection::close(bool notifyPeer)
{
    m_callback->onIPCSenderLog(LOG_INFO_LEVEL
        , ipcFormat("IPCSenderConnection::close, m_recieverPid: %lu, m_senderPid: %lu, notifyPeer: %d", m_recieverPid, m_senderPid, notifyPeer).c_str());

    if (true == notifyPeer && NULL != m_closeEvent)
    {
        ::SetEvent(m_closeEvent);
    }

    return ERROR_SUCCESS;
}

DWORD IPCSenderConnection::send(const void* data, size_t dataSize, DWORD timeout)
{
    if (dataSize + sizeof(DWORD) > m_maxMemorySize || NULL == m_sharedMemory)
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL
            , ipcFormat("IPCSenderConnection::send, m_maxMemorySize: %lu, dataSize: %lu, m_sharedMemory: 0x%08X.", m_maxMemorySize, dataSize, m_sharedMemory).c_str());

        return ERROR_INVALID_PARAMETER;
    }

    // 发送数据，持有保护锁，直到接收端读完数据，通知释放锁

    HANDLE waitMutex[] = { m_targetProcess, m_closeEvent, m_mutex };
    DWORD waitMutexRet = ::WaitForMultipleObjects(_countof(waitMutex), waitMutex, FALSE, timeout);
    switch (waitMutexRet)
    {
    case WAIT_OBJECT_0 + 0:
    case WAIT_OBJECT_0 + 1:
    {
        // 目标程序退出

        m_callback->onIPCSenderLog(LOG_INFO_LEVEL
            , ipcFormat("IPCSenderConnection::send, waitMutexRet: %lu, GetLastError: %lu.", waitMutexRet, ::GetLastError()).c_str());

        m_callback->onIPCSenderClose();

        return ::GetLastError();
    }
    break;
    case WAIT_OBJECT_0 + 2:
    {
        // 获得锁
    }
    break;
    case WAIT_TIMEOUT:
    {
        m_callback->onIPCSenderLog(LOG_INFO_LEVEL
            , ipcFormat("IPCSenderConnection::send, wait mutex timeout: %lu.", timeout).c_str());

        return EcTimeout;
    }
    break;
    case WAIT_ABANDONED_0 + 0:
    case WAIT_ABANDONED_0 + 1:
    case WAIT_ABANDONED_0 + 2:
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL
            , ipcFormat("IPCSenderConnection::send, waitMutexRet: %lu, GetLastError: %lu.", waitMutexRet, ::GetLastError()).c_str());

        return EcWaitAbandoned;
    }
    break;
    case WAIT_FAILED:
    default:
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL
            , ipcFormat("IPCSenderConnection::send, waitMutexRet: %lu, GetLastError: %lu.", waitMutexRet, ::GetLastError()).c_str());

        return ::GetLastError();
    }
    break;
    }

    DWORD cursor = 0;
    BYTE* memoryData = reinterpret_cast<BYTE*>(m_sharedMemory);

    ::memset(memoryData, 0, m_maxMemorySize);

    DWORD size = static_cast<DWORD>(dataSize);
    *reinterpret_cast<DWORD*>(memoryData + cursor) = size;
    cursor += sizeof(size);

    ::memcpy_s(memoryData + cursor, m_maxMemorySize - cursor, data, dataSize);

    ::SetEvent(m_reqEvent);

    DWORD ret = ERROR_SUCCESS;

    HANDLE waitEvent[] = { m_targetProcess, m_closeEvent, m_respEvent };
    DWORD waitEventRet = ::WaitForMultipleObjects(_countof(waitEvent), waitEvent, FALSE, timeout);
    switch (waitEventRet)
    {
    case WAIT_OBJECT_0 + 0:
    case WAIT_OBJECT_0 + 1:
    {
        // 目标程序退出

        m_callback->onIPCSenderLog(LOG_INFO_LEVEL
            , ipcFormat("IPCSenderConnection::send, waitEventRet: %lu, GetLastError: %lu.", waitEventRet, ::GetLastError()).c_str());

        m_callback->onIPCSenderClose();

        ret = ::GetLastError();
    }
    break;
    case WAIT_OBJECT_0 + 2:
    {
        // Event触发
    }
    break;
    case WAIT_TIMEOUT:
    {
        m_callback->onIPCSenderLog(LOG_INFO_LEVEL
            , ipcFormat("IPCSenderConnection::send, wait response event timeout: %lu.", timeout).c_str());

        ret = EcTimeout;
    }
    break;
    case WAIT_ABANDONED_0 + 0:
    case WAIT_ABANDONED_0 + 1:
    case WAIT_ABANDONED_0 + 2:
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL
            , ipcFormat("IPCSenderConnection::send, waitEventRet: %lu, GetLastError: %lu.", waitEventRet, ::GetLastError()).c_str());

        ret = EcWaitAbandoned;
    }
    break;
    case WAIT_FAILED:
    default:
    {
        m_callback->onIPCSenderLog(LOG_ERROR_LEVEL
            , ipcFormat("IPCSenderConnection::send, waitEventRet: %lu, GetLastError: %lu.", waitEventRet, ::GetLastError()).c_str());

        ret = ::GetLastError();
    }
    break;
    }

    ::ReleaseMutex(m_mutex);

    return ret;
}


IPCRecieverConnection::IPCRecieverConnection(IIPCRecieverCallback* callback, size_t maxMemorySize)
    : m_callback(callback)
    , m_recieverPid(0)
    , m_senderPid(0)
    , m_targetProcess()
    , m_closeEvent()
    , m_fileMap()
    , m_sharedMemory(NULL)
    , m_maxMemorySize(maxMemorySize + sizeof(DWORD)) // 头部DWORD存放共享内存大小size
    , m_recieverThread()
    , m_reqEvent()
    , m_respEvent()
{
    assert(NULL != m_callback);
}

IPCRecieverConnection::~IPCRecieverConnection()
{
    close();

    assert(::GetCurrentThreadId() != ::GetThreadId(m_recieverThread));

    ::WaitForSingleObject(m_recieverThread, INFINITE);

    if (NULL != m_sharedMemory)
    {
        ::UnmapViewOfFile(m_sharedMemory);
        m_sharedMemory = NULL;
    }
}

DWORD IPCRecieverConnection::recieverPid() const
{
    return m_recieverPid;
}

DWORD IPCRecieverConnection::senderPid() const
{
    return m_senderPid;
}

DWORD IPCRecieverConnection::listen(DWORD appID, DWORD aRecieverPid, DWORD aSenderPid, DWORD timestamp)
{
    assert(NULL == m_targetProcess && NULL == m_sharedMemory);

    m_callback->onIPCRecieverLog(LOG_INFO_LEVEL
        , ipcFormat("IPCRecieverConnection::listen, appID: %lu, aRecieverPid: %lu, aSenderPid: %lu, timestamp: %lu."
            , appID, aRecieverPid, aSenderPid, timestamp).c_str());

    // 握手连接的Connection，监听自身进程
    DWORD targetPid = aSenderPid;
    if ((0 == aRecieverPid && 0 == aSenderPid && 0 == timestamp)
        || (1 == aRecieverPid && 1 == aSenderPid && 1 == timestamp))
    {
        targetPid = ::GetCurrentProcessId();
    }

    CResource<HANDLE> targetProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPid);
    if (NULL == targetProcess)
    {
        m_callback->onIPCRecieverLog(LOG_ERROR_LEVEL, ipcFormat("IPCRecieverConnection::listen, OpenProcess failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    wchar_t nameBuffer[MAX_PATH] = { 0 };
    int count = ::swprintf_s(nameBuffer, L"close_event_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring closeEventName(nameBuffer, count);
    CResource<HANDLE> closeEvent = ::CreateEventW(NULL, TRUE, FALSE, closeEventName.c_str());
    if (NULL == closeEvent)
    {
        m_callback->onIPCRecieverLog(LOG_ERROR_LEVEL, ipcFormat("IPCRecieverConnection::listen, CreateEvent failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    count = ::swprintf_s(nameBuffer, L"file_map_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring fileMapName(nameBuffer, count);
    CResource<HANDLE> fileMap = ::CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE
        , 0, m_maxMemorySize, fileMapName.c_str());
    if (NULL == fileMap)
    {
        m_callback->onIPCRecieverLog(LOG_ERROR_LEVEL, ipcFormat("IPCRecieverConnection::listen, CreateFileMappingW failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    count = ::swprintf_s(nameBuffer, L"req_event_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring reqEventName(nameBuffer, count);
    CResource<HANDLE> reqEvent = ::CreateEventW(NULL, FALSE, FALSE, reqEventName.c_str());
    if (NULL == reqEvent)
    {
        m_callback->onIPCRecieverLog(LOG_ERROR_LEVEL, ipcFormat("reqEventName: %s, CreateEvent failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    count = ::swprintf_s(nameBuffer, L"resp_event_name_%lu_%lu_%lu_%lu", appID, aRecieverPid, aSenderPid, timestamp);
    std::wstring respEventName(nameBuffer, count);
    CResource<HANDLE> respEvent = ::CreateEventW(NULL, FALSE, FALSE, respEventName.c_str());
    if (NULL == respEvent)
    {
        m_callback->onIPCRecieverLog(LOG_ERROR_LEVEL, ipcFormat("IPCRecieverConnection::listen, CreateEvent failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    LPVOID sharedMemory = ::MapViewOfFile(fileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, m_maxMemorySize);
    if (NULL == sharedMemory)
    {
        m_callback->onIPCRecieverLog(LOG_ERROR_LEVEL, ipcFormat("IPCRecieverConnection::listen, MapViewOfFile failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    // 不要在此处添加代码，避免出现资源泄露

    CResource<HANDLE> reciever_thread = reinterpret_cast<HANDLE>(::_beginthreadex(NULL, 0, on_reciever_thread, this, CREATE_SUSPENDED, NULL));
    if (NULL == reciever_thread)
    {
        ::UnmapViewOfFile(sharedMemory);
        sharedMemory = NULL;

        m_callback->onIPCRecieverLog(LOG_ERROR_LEVEL, ipcFormat("IPCRecieverConnection::listen, _beginthreadex failed: %lu.", ::GetLastError()).c_str());
        return ::GetLastError();
    }

    m_targetProcess.Attach(targetProcess.Detach());
    m_closeEvent.Attach(closeEvent.Detach());
    m_fileMap.Attach(fileMap.Detach());
    m_recieverThread.Attach(reciever_thread.Detach());
    m_reqEvent.Attach(reqEvent.Detach());
    m_respEvent.Attach(respEvent.Detach());
    m_sharedMemory = sharedMemory;

    m_recieverPid = aRecieverPid;
    m_senderPid = aSenderPid;

    ::ResumeThread(m_recieverThread);

    m_callback->onIPCRecieverLog(LOG_INFO_LEVEL, "IPCRecieverConnection::listen successfully.");

    return ERROR_SUCCESS;
}

DWORD IPCRecieverConnection::close()
{
    m_callback->onIPCRecieverLog(LOG_INFO_LEVEL
        , ipcFormat("IPCRecieverConnection::close, m_recieverPid: %lu, m_senderPid: %lu", m_recieverPid, m_senderPid).c_str());

    if (NULL != m_closeEvent)
    {
        ::SetEvent(m_closeEvent);
    }

    return ERROR_SUCCESS;
}

unsigned __stdcall IPCRecieverConnection::on_reciever_thread(void* arg)
{
    IPCRecieverConnection* connection = reinterpret_cast<IPCRecieverConnection*>(arg);
    connection->handle_loop();

    return 0;
}

void IPCRecieverConnection::handle_loop()
{
    while (true)
    {
        HANDLE waitEvent[] = { m_targetProcess, m_closeEvent, m_reqEvent };
        DWORD waitEventRet = ::WaitForMultipleObjects(_countof(waitEvent), waitEvent, FALSE, INFINITE);
        if (WAIT_OBJECT_0 + 2 != waitEventRet)    // process, closeEvent or failed
        {
            m_callback->onIPCRecieverLog(LOG_INFO_LEVEL
                , ipcFormat("IPCRecieverConnection::handle_loop, waitEventRet: %lu, GetLastError: %lu.", waitEventRet, ::GetLastError()).c_str());

            m_callback->onIPCRecieverClose();

            break;
        }

        // 发送端已经加锁保护共享内存

        DWORD cursor = 0;
        BYTE* memoryData = reinterpret_cast<BYTE*>(m_sharedMemory);

        DWORD dataSize = *reinterpret_cast<DWORD*>(memoryData + cursor);
        cursor += sizeof(dataSize);
        if (dataSize + sizeof(dataSize) > m_maxMemorySize)
        {
            m_callback->onIPCRecieverLog(LOG_ERROR_LEVEL, ipcFormat("IPCRecieverConnection::handle_loop, wrong packet size: %lu.", dataSize).c_str());
        }
        else
        {
            m_callback->onIPCReciverRecv(memoryData + cursor, dataSize);
        }

        ::SetEvent(m_respEvent);
    }
}

/**************************************************************************/
