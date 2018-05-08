#include "Application.h"
#include "MainDialog.h"
#include "log.h"
#include "Base.h"
#include "jsoncpp/json.h"

#include <assert.h>
#include <combaseapi.h>
#include <shellapi.h>
#include <QApplication>

static unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

static unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

static std::string URLEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

static std::string URLDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

Application::Application()
{
    ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
}

Application::~Application()
{
    ::CoUninitialize();
}

Application& Application::instance()
{
    static Application uniqueInstance;
    return uniqueInstance;
}

int Application::run(int &argc, char **argv)
{
    LOGGER;

    QApplication app(argc, argv);

    bool regRet = regProtol();
    LINFO(L"Register protol ret: %s", (true == regRet ? L"true" : L"false"));

    int ret = 0;

    MainDialog m_mainDialog;
    if (1 == argc)
    {
        m_mainDialog.show();

        ret = app.exec();
    }
    else
    {
        showNormalLiveByCommandLine(argc, argv);
    }

    return ret;
}

void Application::quit(int retcode)
{
    LOGGER;

    QApplication::exit(retcode);
}

bool Application::openAndWait(const std::string& jsonUTF8)
{
    assert(true != jsonUTF8.empty());

    LINFO(L"jsonUTF8: %s", UTF82Wide(jsonUTF8).c_str());

    std::string encodeJson = URLEncode(jsonUTF8);

    std::wstring cmd = L"\"txcloudroom://liteav/params?json=";
    cmd.append(UTF82Wide(encodeJson));
    cmd.append(L"\"");

    LINFO(L"cmd: %s", cmd.c_str());

    DWORD pathValueType = 0;
    DWORD pathValueLength = MAX_PATH + 32;
    wchar_t pathValue[MAX_PATH + 32] = { 0 };
    LONG regRet = ::RegGetValueW(HKEY_CLASSES_ROOT, L"TXCloudRoom\\shell\\open\\command", L"", RRF_RT_REG_SZ, &pathValueType, pathValue, &pathValueLength);
    if (ERROR_SUCCESS != regRet)
    {
        LERROR(L"RegGetValueW failed, regRet: %lu", regRet);
        return false;
    }

    if (0 == pathValueLength)
    {
        LERROR(L"pathLength = 0");
        return false;
    }

    LPCWSTR lpIndex = ::wcschr(pathValue + 1, L'\"');
    if (L'\"' != pathValue[0] || NULL == lpIndex)   // "path" "%1"的格式
    {
        LERROR(L"pathValue invalid: %s", pathValue);
        return false;
    }

    std::wstring path(pathValue + 1, (lpIndex - pathValue - 1));

    SHELLEXECUTEINFOW sei = { 0 };
    sei.cbSize = sizeof(SHELLEXECUTEINFOW);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = L"open";
    sei.lpFile = path.c_str();
    sei.lpParameters = cmd.c_str();
    sei.lpDirectory = NULL;
    sei.nShow = SW_SHOWNORMAL;
    sei.hInstApp = NULL;
    sei.lpIDList = NULL;
    sei.lpClass = NULL;
    sei.hkeyClass = NULL;
    sei.dwHotKey = NULL;
    sei.hIcon = NULL;
    sei.hProcess = NULL;

    BOOL ret = ::ShellExecuteExW(&sei);
    if (FALSE == ret)
    {
        LERROR(L"ShellExecuteExW failed: %lu", ::GetLastError());
        return false;
    }

    ::WaitForSingleObject(sei.hProcess, INFINITE);

    DWORD exitCode = 0;
    ::GetExitCodeProcess(sei.hProcess, &exitCode);
    if (0 != exitCode)
    {
        LERROR(L"GetExitCodeProcess: %d", static_cast<int>(exitCode));
        return false;
    }

    ::CloseHandle(sei.hProcess);

    return true;
}

void Application::showNormalLiveByCommandLine(int &argc, char **argv)
{
    std::string pushURL = "";
    int cameraIndex = 0;
    int width = 640;
    int height = 480;
    int rotation = 1;
    int bitrate = 900;

    for (int index = 1; index < argc; index++)
    {
        std::string field = argv[index];
        if (0 == field.find("pushURL="))
        {
            if (field.size() > ::strlen("pushURL="))
            {
                pushURL = field.substr(::strlen("pushURL="));
            }
        }
        else if (0 == field.find("camera="))
        {
            if (field.size() > ::strlen("camera="))
            {
                cameraIndex = ::atoi(field.substr(::strlen("camera=")).c_str());
            }
        }
        else if (0 == field.find("width="))
        {
            if (field.size() > ::strlen("width="))
            {
                width = ::atoi(field.substr(::strlen("width=")).c_str());
            }
        }
        else if (0 == field.find("height="))
        {
            if (field.size() > ::strlen("height="))
            {
                width = ::atoi(field.substr(::strlen("height=")).c_str());
            }
        }
        else if (0 == field.find("rotation="))
        {
            if (field.size() > ::strlen("rotation="))
            {
                width = ::atoi(field.substr(::strlen("rotation=")).c_str());
            }
        }
        else if (0 == field.find("bitrate="))
        {
            if (field.size() > ::strlen("bitrate="))
            {
                width = ::atoi(field.substr(::strlen("bitrate=")).c_str());
            }
        }
    }

    Json::Value root;
    root["type"] = "NormalLive";
    root["action"] = "commandLine";
    root["pushURL"] = pushURL;
    root["cameraIndex"] = cameraIndex;
    root["width"] = width;
    root["height"] = height;
    root["rotation"] = rotation;
    root["bitrate"] = bitrate;
    root["port"] = ((::time(NULL) % 15000) + 50000);    // 建议范围限制[50000, 65535]

    Json::FastWriter writer;
    std::string jsonUTF8 = writer.write(root);
    bool openRet = Application::instance().openAndWait(jsonUTF8);

    LINFO(L"openRet: %s, pushURL: %hhs, cameraIndex: %d, width: %d, height: %d, rotation: %d, bitrate: %d"
        , (true == openRet ? L"true" : L"false"), pushURL, cameraIndex, width, height, rotation, bitrate);
}

bool Application::regProtol()
{
    wchar_t pathBuff[MAX_PATH] = { 0 };
    DWORD count = ::GetModuleFileNameW(NULL, pathBuff, _countof(pathBuff));
    if (0 == count)
    {
        LERROR(L"GetModuleFileNameW failed: %lu", ::GetLastError());
        return false;
    }

    LPCWSTR lpszLastSlash = ::wcsrchr(pathBuff, _W('\\'));
    if (NULL == lpszLastSlash)
    {
        LERROR(L"Wrong pathBuff: %s", pathBuff);
        return false;
    }

    std::wstring path(pathBuff, lpszLastSlash - pathBuff + 1);
    path.append(L"TXCloudRoom.exe");

    // 自动释放HKEY
    auto deleter = [](HKEY* key) {
        if (key && *key)
        {
            ::RegCloseKey(*key);
        }
    };

    HKEY root = NULL;
    LSTATUS ret = ::RegCreateKeyEx(HKEY_CLASSES_ROOT, L"TXCloudRoom", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &root, NULL);
    if (ERROR_SUCCESS != ret)
    {
        LPWSTR msg = NULL;
        ::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, ret,
            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), msg, 0, NULL);
        LERROR(L"RegCreateKeyEx failed, ret: %lu, msg: %s", ret, msg);

        ::LocalFree(msg);
        return false;
    }

    std::unique_ptr<HKEY, decltype(deleter)> txcloudLiteAVPtr(&root, deleter);

    std::wstring value = L"TXCloud LiteAV Protocol";
    ::RegSetValueExW(*txcloudLiteAVPtr, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), (value.size() + 1) * 2);

    value = path;
    ::RegSetValueExW(*txcloudLiteAVPtr, L"URL Protocol", 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), (value.size() + 1) * 2);

    HKEY defaultIcon = NULL;
    ret = ::RegCreateKeyEx(*txcloudLiteAVPtr, L"DefaultIcon", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &defaultIcon, NULL);
    if (ERROR_SUCCESS != ret)
    {
        LPWSTR msg = NULL;
        ::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, ret,
            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), msg, 0, NULL);
        LERROR(L"RegCreateKeyEx failed, ret: %lu, msg: %s", ret, msg);

        ::LocalFree(msg);
        return false;
    }

    std::unique_ptr<HKEY, decltype(deleter)> defaultIconPtr(&defaultIcon, deleter);

    value = format(L"%s,1", path.c_str());
    ::RegSetValueExW(*defaultIconPtr, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), (value.size() + 1) * 2);

    HKEY shell = NULL;
    ret = ::RegCreateKeyEx(*txcloudLiteAVPtr, L"shell", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &shell, NULL);
    if (ERROR_SUCCESS != ret)
    {
        LPWSTR msg = NULL;
        ::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, ret,
            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), msg, 0, NULL);
        LERROR(L"RegCreateKeyEx failed, ret: %lu, msg: %s", ret, msg);

        ::LocalFree(msg);
        return false;
    }

    std::unique_ptr<HKEY, decltype(deleter)> shellPtr(&shell, deleter);

    HKEY open = NULL;
    ret = ::RegCreateKeyEx(*shellPtr, L"open", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &open, NULL);
    if (ERROR_SUCCESS != ret)
    {
        LPWSTR msg = NULL;
        ::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, ret,
            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), msg, 0, NULL);
        LERROR(L"RegCreateKeyEx failed, ret: %lu, msg: %s", ret, msg);

        ::LocalFree(msg);
        return false;
    }

    std::unique_ptr<HKEY, decltype(deleter)> openPtr(&open, deleter);

    HKEY command = NULL;
    ret = ::RegCreateKeyEx(*openPtr, L"command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &command, NULL);
    if (ERROR_SUCCESS != ret)
    {
        LPWSTR msg = NULL;
        ::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, ret,
            MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), msg, 0, NULL);
        LERROR(L"RegCreateKeyEx failed, ret: %lu, msg: %s", ret, msg);

        ::LocalFree(msg);
        return false;
    }

    std::unique_ptr<HKEY, decltype(deleter)> commandPtr(&command, deleter);

    value = format(L"\"%s\",\"%%1\"", path.c_str());
    ::RegSetValueExW(*commandPtr, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), (value.size() + 1) * 2);

    return true;
}
