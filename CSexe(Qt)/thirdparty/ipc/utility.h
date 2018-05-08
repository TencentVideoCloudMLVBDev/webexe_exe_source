/**************************************************************************
    Copyright:      Copyright ? 1998 - 2015 Tencent. All Rights Reserved
    Created:        2015-9-8 15:24:53
    Filename:       utility.h

    Description:    
***************************************************************************/

#ifndef __UTILITY_H__
#define __UTILITY_H__

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <MMSystem.h>
#include <assert.h>
#include <string>

/**************************************************************************/

template <class T>
class ReleaseFunction
{
public:
    void operator()(T& res) const
    {
        assert(FALSE);
    }
};

// ReleaseHandle模板特化
template <>
class ReleaseFunction<HANDLE>
{
public:
    void operator()(HANDLE& res) const
    {
        if (NULL != res && INVALID_HANDLE_VALUE != res)
        {
            ::CloseHandle(res);
            res = NULL;
        }
    }
};

template <class T>
class CResource
{
public:
    inline CResource() : m_res(NULL)
    {
    }

    inline CResource(CResource& res) : m_res(NULL)
    {
        Attach(res.Detach());
    }

    inline CResource(T res) : m_res(res)
    {
    }

    inline ~CResource()
    {
        if (NULL != m_res)
        {
            Close();
        }
    }

    inline CResource& operator =(CResource& res)
    {
        if (this != &res)
        {
            if (NULL != m_res)
            {
                Close();
            }

            Attach(res.Detach());
        }

        return (*this);
    }

    inline operator T() const
    {
        return (m_res);
    }

    inline T* operator &()
    {
        return (&m_res);
    }

    inline void Attach(T res)
    {
        assert(NULL == m_res);
        m_res = res;  // Take ownership
    }

    inline T Detach()
    {
        T res = m_res;  // Release ownership
        m_res = NULL;

        return (res);
    }

    inline void Close()
    {
        ReleaseFunction<T> rf;
        rf(m_res);
    }

private:
    T m_res;
};

// 自定义错误码
enum ErrorCode
{
    // 大于系统错误码的最大值，小于GDPLoader允许的最大值，
    // 故有效范围设置为[0x4000, 0x7000]
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms681384(v=vs.85).aspx

    EcSuccess                                               = ERROR_SUCCESS,
    EcFailed                                                = 0x7000 + 0x1,             // 28673
    EcWrongParameter                                        = 0x7000 + 0x2,
    EcTimeout                                               = 0x7000 + 0x3,
    EcWaitAbandoned                                         = 0x7000 + 0x4,             // 内核对象被异常释放，导致wait失败
};

static std::string ipcFormat(const char* pszFormat, ...)
{
    char buffer[MAX_PATH * 4] = { 0 };

    va_list ap;
    va_start(ap, pszFormat);
    int nCount = ::vsprintf_s(buffer, _countof(buffer), pszFormat, ap);
    va_end(ap);

    if (nCount < 0)
    {
        assert(false);
        return pszFormat;
    }

    return buffer;
}

/**************************************************************************/
#endif /* __UTILITY_H__ */
