#include "Application.h"
#include "log.h"
#include "Base.h"
#include "DialogMessage.h"
#include "curl.h"

#ifndef min
#define min
#endif

#ifndef max
#define max
#endif

#include <gdiplus.h>
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

Application::Application(QObject *parent)
    : QObject(parent)
    , m_gdiplusToken(NULL)
    , m_normalLive(NULL)
    , m_csLive(NULL)
    , m_liveDemo(NULL)
    , m_RTCDemo(NULL)
    , m_httpPort(0)
    , m_httpServer(this)
    , m_dataList()
    , m_maxDataListCount(500)
    , m_mutex()
{
    ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput = { 0 };
    Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

    // �л������߳�
    qRegisterMetaType<ApplicationFunction>("ApplicationFunction");
    connect(this, SIGNAL(dispatch(ApplicationFunction)), this, SLOT(handle(ApplicationFunction)), Qt::QueuedConnection);
}

Application::~Application()
{
    m_httpServer.close();

    Gdiplus::GdiplusShutdown(m_gdiplusToken);

    ::CoUninitialize();
}

Application& Application::instance()
{
    static Application uniqueInstance(Q_NULLPTR);
    return uniqueInstance;
}

int Application::run(int &argc, char **argv)
{
    LOGGER;

    //::MessageBoxW(NULL, NULL, NULL, MB_OK);   // ���㸽�ӵ���

	bool regRet = regProtol();
	LINFO(L"Register protol ret: %s", (true == regRet ? L"true" : L"false"));

    curl_global_init(CURL_GLOBAL_ALL);

    QApplication app(argc, argv);

    if (argc <= 1)
    {
        DialogMessage::exec(QStringLiteral("ͨ��Portal.exe��Webҳ������"), DialogMessage::OK);
        return -1;
    }

    std::string cmd = QString::fromWCharArray(::GetCommandLineW()).toStdString();  // ע�⣬��argv������������е����ţ�����json�ַ������Ϸ�����ʹ��GetCommandLine����

    std::string prefix = "txcloudroom://liteav/params?json=";
    size_t index = cmd.find(prefix);  // ���Դ�Сд
    if (std::string::npos == index)
    {
        DialogMessage::exec(QStringLiteral("���в�����ʽ����"), DialogMessage::OK);
        return -1;
    }

    std::string json = cmd.substr(index + prefix.size());

    int ret = 0;

    if (true == resolveProtol(json))
    {
        ret = app.exec();
    }
    else
    {
        ret = -1;
    }

    if (nullptr != m_normalLive)
    {
        delete m_normalLive;
        m_normalLive = nullptr;
    }

    if (nullptr != m_csLive)
    {
        delete m_csLive;
        m_csLive = nullptr;
    }

    if (nullptr != m_liveDemo)
    {
        delete m_liveDemo;
        m_liveDemo = nullptr;
    }

    if (nullptr != m_RTCDemo)
    {
        delete m_RTCDemo;
        m_RTCDemo = nullptr;
    }

    LINFO(L"run ret: %d", ret);

    curl_global_cleanup();

    return ret;
}

void Application::quit(int retcode)
{
    LOGGER;

    QApplication::exit(retcode);
}

void Application::pushSDKEvent(int eventID, const std::map<std::string, std::string>& params)
{
    Json::Value paramsValue;
    for (std::map<std::string, std::string>::const_iterator it = params.begin(); params.end() != it; ++it)
    {
        Json::Value item;
        item["name"] = Wide2UTF8(Ansi2Wide(it->first));
        item["value"] = Wide2UTF8(Ansi2Wide(it->second));

        paramsValue.append(item);
    }

    Json::Value root;
    root["event"] = "sdkEventCallback";
    root["eventID"] = eventID;
    root["params"] = paramsValue;

    std::lock_guard<std::mutex> lock(m_mutex);

    m_dataList.push_back(root);
    if (m_dataList.size() > m_maxDataListCount)
    {
        m_dataList.pop_front();
    }
}

void Application::pushRoomStatus(int code, const std::string& msgUTF8)
{
    LTRACE(L"code: %d, msgUTF8: %s", code, UTF82Wide(msgUTF8).c_str());

    Json::Value root;
    root["event"] = "roomStatus";
    root["code"] = code;
    root["msg"] = msgUTF8.c_str();

    std::lock_guard<std::mutex> lock(m_mutex);

    m_dataList.push_back(root);
    if (m_dataList.size() > m_maxDataListCount)
    {
        m_dataList.pop_front();
    }
}

void Application::pushRoomTextMsg(const std::string& roomID, const std::string& userID, const std::string& userName
    , const std::string& userAvatar, const std::string& msg)
{
    Json::Value root;
    root["event"] = "roomTextMsg";
    root["roomID"] = roomID;
    root["userID"] = userID;
    root["userName"] = userName;
    root["userAvatar"] = userAvatar;
    root["msg"] = msg;

    std::lock_guard<std::mutex> lock(m_mutex);

    m_dataList.push_back(root);
    if (m_dataList.size() > m_maxDataListCount)
    {
        m_dataList.pop_front();
    }
}

void Application::pushMemberChange(const std::list<MemberItem>& members)
{
    Json::Value list;
    for (std::list<MemberItem>::const_iterator it = members.begin(); members.end() != it; ++it)
    {
        Json::Value item;
        item["userID"] = it->userID;
        item["userName"] = it->userName;
        item["userAvatar"] = it->userAvatar;
        item["role"] = static_cast<int>(it->role);
        list.append(item);
    }

    Json::Value root;
    root["event"] = "menberChange";
    root["list"] = list;

    std::lock_guard<std::mutex> lock(m_mutex);

    m_dataList.push_back(root);
    if (m_dataList.size() > m_maxDataListCount)
    {
        m_dataList.pop_front();
    }
}

void Application::executeInMainThread(ApplicationFunction func)
{
    emit dispatch(func);
}

void Application::handle(ApplicationFunction func)
{
    func();
}

void Application::onGetRequest(const std::wstring& absPath, DWORD& statusCode, std::string& respDataUTF8)
{
    if (0 == absPath.find(L"/query"))
    {
        handleReqQuery(absPath, statusCode, respDataUTF8);
    }
    else if (0 == absPath.find(L"/quit"))
    {
        handleReqQuit(absPath, statusCode, respDataUTF8);
    }
}

void Application::onLog(HSLogLevel level, const std::string& content)
{
    LINFO(L"level: %d, content: %s", level, Ansi2Wide(content).c_str());
}

void Application::onClose(ULONGLONG requestId)
{
    LINFO(L"requestId: %lu", requestId);
}

void Application::handleReqQuery(const std::wstring& absPath, DWORD& statusCode, std::string& respDataUTF8)
{
    std::wstring callbackValue(L"");

    std::wstring prefix(L"callback=");
    size_t beginIndex = absPath.find(prefix, 0);
    if (std::wstring::npos != beginIndex)
    {
        beginIndex += prefix.size();
        size_t endIndex = absPath.find(L"&", beginIndex);
        if (endIndex > beginIndex)
        {
            callbackValue = absPath.substr(beginIndex, endIndex - beginIndex);
        }
    }

    Json::Value data;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        for (std::list<Json::Value>::iterator it = m_dataList.begin(); m_dataList.end() != it; ++it)
        {
            data.append(*it);
        }

        m_dataList.clear();
    }

    Json::Value root;
    root["code"] = 0;
    root["message"] = "";
    root["port"] = m_httpPort;
    root["data"] = data;

    Json::FastWriter writer;
    std::string jsonUTF8 = writer.write(root);

    statusCode = 200;
    if (true == callbackValue.empty())
    {
        respDataUTF8 = jsonUTF8;
    }
    else
    {
        respDataUTF8 = Wide2UTF8(callbackValue);
        respDataUTF8.append("(");
        respDataUTF8.append(jsonUTF8);
        respDataUTF8.append(")");
    }
}

void Application::handleReqQuit(const std::wstring& absPath, DWORD& statusCode, std::string& respDataUTF8)
{
    executeInMainThread([=] {
        if (nullptr != m_normalLive)
        {
            m_normalLive->quit();
            delete m_normalLive;
            m_normalLive = nullptr;
        }

        if (nullptr != m_csLive)
        {
            m_csLive->quit();
            delete m_csLive;
            m_csLive = nullptr;
        }

        if (nullptr != m_liveDemo)
        {
            m_liveDemo->leaveRoom();
            delete m_liveDemo;
            m_liveDemo = nullptr;
        }

        if (nullptr != m_RTCDemo)
        {
            m_RTCDemo->leaveRoom();
            delete m_RTCDemo;
            m_RTCDemo = nullptr;
        }
    });
}

bool Application::resolveProtol(const std::string& json)
{
    LINFO(L"json: %s", Ansi2Wide(json).c_str());

    std::string jsonDecode = URLDecode(json);

    Json::Value root;
    Json::Reader reader;
    if (false == reader.parse(jsonDecode, root))
    {
        return false;
    }

    std::string type;
    if (root.isMember("type"))
    {
        type = root["type"].asString();
    }

    if (type == "NormalLive")
    {
        return resolveNormalLiveProtol(root);
    }
    else if (type == "CustomServiceLive")
    {
        return resolveCSLiveProtol(root);
    }
    else if (type == "LiveRoom")
    {
        return resolveLiveRoomProtol(root);
    }
    else if (type == "RTCRoom")
    {
        return resolveRTCRoomProtol(root);
    }
    else
    {
        return false;
    }

    return true;
}

bool Application::resolveNormalLiveProtol(const Json::Value& root)
{
    std::string action;
    if (root.isMember("action"))
    {
        action = root["action"].asString();
    }

    if (root.isMember("port"))
    {
        m_httpPort = root["port"].asInt();
        if (-1 != m_httpPort)
        {
            std::wstring queryURL = format(L"http://localhost:%d/query", m_httpPort);
            std::wstring leaveRoomURL = format(L"http://localhost:%d/quit", m_httpPort);
            std::vector<std::wstring> urls = { queryURL, leaveRoomURL };
            DWORD hsRet = m_httpServer.listen(urls);

            LINFO(L"Http server listen ret: %lu, port: %d", hsRet, m_httpPort);
        }
    }

    // todo ������������

    if (true == action.empty())
    {
        m_normalLive = new PushPlayDemo();
    }
    else
    {
        std::string pushURL = "";
        if (root.isMember("pushURL"))
        {
            pushURL = root["pushURL"].asString();
        }

        int cameraIndex = 0;
        if (root.isMember("cameraIndex"))
        {
            cameraIndex = root["cameraIndex"].asInt();
        }

        int width = 640;
        if (root.isMember("width"))
        {
            width = root["width"].asInt();
        }

        int height = 480;
        if (root.isMember("height"))
        {
            height = root["height"].asInt();
        }

        int rotation = 1;
        if (root.isMember("rotation"))
        {
            rotation = root["rotation"].asInt();
        }

        int bitrate = 900;
        if (root.isMember("bitrate"))
        {
            bitrate = root["bitrate"].asInt();
        }

        m_normalLive = new PushPlayDemo(pushURL.c_str(), width, height, cameraIndex, rotation, bitrate);
    }

    std::string ip;
    int proxyPort = 0;
    if (root.isMember("proxy"))
    {
        Json::Value proxy = root["proxy"];

        if (proxy.isMember("ip"))
        {
            ip = proxy["ip"].asString();
        }

        if (proxy.isMember("port"))
        {
            proxyPort = proxy["port"].asInt();
        }
    }

    m_normalLive->setProxy(ip, static_cast<unsigned short>(proxyPort));

    m_normalLive->show();

    return true;
}

bool Application::resolveCSLiveProtol(const Json::Value& root)
{
    // todo ������������

    std::string action;
    if (root.isMember("action"))
    {
        action = root["action"].asString();
    }

    if (root.isMember("port"))
    {
        m_httpPort = root["port"].asInt();
        if (-1 != m_httpPort)
        {
            std::wstring queryURL = format(L"http://localhost:%d/query", m_httpPort);
            std::wstring leaveRoomURL = format(L"http://localhost:%d/quit", m_httpPort);
            std::vector<std::wstring> urls = { queryURL, leaveRoomURL };
            DWORD hsRet = m_httpServer.listen(urls);

            LINFO(L"Http server listen ret: %lu, port: %d", hsRet, m_httpPort);
        }
    }

    if (action == "stop")
    {
        if (NULL != m_csLive)
        {
            m_csLive->close();

            delete m_csLive;
            m_csLive = NULL;
        }
    }
    else if (action == "start")
    {
        std::string pushURL;
        if (root.isMember("pushURL"))
        {
            pushURL = root["pushURL"].asString();
        }

        std::string playURL;
        if (root.isMember("playURL"))
        {
            playURL = root["playURL"].asString();
        }

        std::string title;
        if (root.isMember("title"))
        {
            title = root["title"].asString();
        }

        std::string logo;
        if (root.isMember("logo"))
        {
            logo = root["logo"].asString();
        }

        m_csLive = new DialogPushPlay();

        std::string ip;
        int proxyPort = 0;
        if (root.isMember("proxy"))
        {
            Json::Value proxy = root["proxy"];

            if (proxy.isMember("ip"))
            {
                ip = proxy["ip"].asString();
            }

            if (proxy.isMember("port"))
            {
                proxyPort = proxy["port"].asInt();
            }
        }

        m_csLive->setProxy(ip, static_cast<unsigned short>(proxyPort));

        m_csLive->setTitle(title.c_str());
        m_csLive->setLogo(logo.c_str());
        m_csLive->startPush(pushURL.c_str());
        m_csLive->startPlay(playURL.c_str());
        m_csLive->show();
    }
    else
    {
        return false;
    }

    return true;
}

bool Application::resolveLiveRoomProtol(const Json::Value& root)
{
    // todo ������������

    std::string action;
    if (root.isMember("action"))
    {
        action = root["action"].asString();
    }

    if (root.isMember("port"))
    {
        m_httpPort = root["port"].asInt();
        if (-1 != m_httpPort)
        {
            std::wstring queryURL = format(L"http://localhost:%d/query", m_httpPort);
            std::wstring leaveRoomURL = format(L"http://localhost:%d/quit", m_httpPort);
            std::vector<std::wstring> urls = { queryURL, leaveRoomURL };
            DWORD hsRet = m_httpServer.listen(urls);

            LINFO(L"Http server listen ret: %lu, port: %d", hsRet, m_httpPort);
        }
    }

    if (action == "createRoom" || action == "enterRoom")
    {
        if (NULL == m_liveDemo)
        {
            m_liveDemo = new LiveDemo();
        }

        std::string ip;
        int proxyPort = 0;
        if (root.isMember("proxy"))
        {
            Json::Value proxy = root["proxy"];

            if (proxy.isMember("ip"))
            {
                ip = proxy["ip"].asString();
            }

            if (proxy.isMember("port"))
            {
                proxyPort = proxy["port"].asInt();
            }
        }

        m_liveDemo->setProxy(ip, static_cast<unsigned short>(proxyPort));

        std::string serverDomain;
        int sdkAppID = 0;
        std::string accountType;
		std::string userID;
		std::string userSig;
		std::string userName;
		std::string userAvatar;
		std::string roomID;
		std::string roomInfo;
		std::string title;
		std::string logo;        
		std::string strTemplate = "1v4";
		std::string userTag = QStringLiteral("����Ա").toStdString();
		bool userList = true;
		bool IMList = true;
		bool whiteboard = false;
		bool screenShare = false;
		bool record = false;

		getConfigInfo(root, serverDomain, sdkAppID, accountType, userID, userSig, userName, userAvatar, roomID, roomInfo, strTemplate,userTag, userList, IMList, whiteboard, screenShare, record, title, logo);
		
		std::transform(strTemplate.begin(), strTemplate.end(), strTemplate.begin(), ::tolower);
		LRAuthData authData;
		authData.accountType = accountType;
		authData.sdkAppID = sdkAppID;
		authData.userAvatar = userAvatar;
		authData.userName = QString::fromUtf8(userName.c_str()).toStdString();
		authData.userID = userID;
		authData.userSig = userSig;

		m_liveDemo->setTitle(title.c_str());
        m_liveDemo->setLogo(logo.c_str());

		m_liveDemo->initUI(strTemplate.c_str(), QString::fromUtf8(userTag.c_str()), userList, IMList, whiteboard, screenShare);

		if (action == "createRoom")
			m_liveDemo->createRoom(authData, serverDomain.c_str(), roomID.c_str(), QString::fromUtf8(roomInfo.c_str()), record);
		else
			m_liveDemo->enterRoom(authData, serverDomain.c_str(), roomID.c_str(), QString::fromUtf8(roomInfo.c_str()), record);

		m_liveDemo->show();
    }
    else if (action == "leaveRoom")
    {
        if (NULL != m_liveDemo)
        {
            m_liveDemo->leaveRoom();
			m_liveDemo->hide();
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool Application::resolveRTCRoomProtol(const Json::Value& root)
{
    // todo ������������

    std::string action;
    if (root.isMember("action"))
    {
        action = root["action"].asString();
    }

    if (root.isMember("port"))
    {
        m_httpPort = root["port"].asInt();
        if (-1 != m_httpPort)
        {
            std::wstring queryURL = format(L"http://localhost:%d/query", m_httpPort);
            std::wstring leaveRoomURL = format(L"http://localhost:%d/quit", m_httpPort);
            std::vector<std::wstring> urls = { queryURL, leaveRoomURL };
            DWORD hsRet = m_httpServer.listen(urls);

            LINFO(L"Http server listen ret: %lu, port: %d", hsRet, m_httpPort);
        }
    }

    if (action == "createRoom" || action == "enterRoom")
    {
        if (NULL == m_RTCDemo)
        {
            m_RTCDemo = new RTCDemo();
        }

        std::string ip;
        int proxyPort = 0;
        if (root.isMember("proxy"))
        {
            Json::Value proxy = root["proxy"];

            if (proxy.isMember("ip"))
            {
                ip = proxy["ip"].asString();
            }

            if (proxy.isMember("port"))
            {
                proxyPort = proxy["port"].asInt();
            }
        }

        m_RTCDemo->setProxy(ip, static_cast<unsigned short>(proxyPort));

		std::string serverDomain;
		int sdkAppID = 0;
		std::string accountType;
		std::string userID;
		std::string userSig;
		std::string userName;
		std::string userAvatar;
		std::string roomID;
		std::string roomInfo;
		std::string title;
		std::string logo;
		std::string strTemplate = "1v1";
		std::string userTag = QStringLiteral("����Ա").toStdString();
		bool userList = true;
		bool IMList = true;
		bool whiteboard = false;
		bool screenShare = false;
		bool record = false;

		getConfigInfo(root, serverDomain, sdkAppID, accountType, userID, userSig, userName, userAvatar, roomID, roomInfo, strTemplate, userTag, userList, IMList, whiteboard, screenShare, record, title, logo);

		std::transform(strTemplate.begin(), strTemplate.end(), strTemplate.begin(), ::tolower);
		RTCAuthData authData;
		authData.accountType = accountType;
		authData.sdkAppID = sdkAppID;
		authData.userAvatar = userAvatar;
		authData.userName = QString::fromUtf8(userName.c_str()).toStdString();
		authData.userID = userID;
		authData.userSig = userSig;
		m_RTCDemo->initUI(strTemplate.c_str(), QString::fromUtf8(userTag.c_str()), userList, IMList, whiteboard, screenShare);

		if (action == "createRoom")
			m_RTCDemo->createRoom(authData, serverDomain.c_str(), roomID.c_str(), QString::fromUtf8(roomInfo.c_str()), record);
		else
			m_RTCDemo->enterRoom(authData, serverDomain.c_str(), roomID.c_str(), QString::fromUtf8(roomInfo.c_str()), record);

		m_RTCDemo->setTitle(title.c_str());
		m_RTCDemo->setLogo(logo.c_str(), strTemplate != "1v1");

		m_RTCDemo->show();
    }
    else if (action == "leaveRoom")
    {
        if (NULL != m_RTCDemo)
        {
            m_RTCDemo->leaveRoom();
			m_RTCDemo->hide();
        }
    }
    else
    {
        return false;
    }

    return true;
}

void Application::getConfigInfo(const Json::Value& root, std::string & serverDomain, int & sdkAppID, std::string & accountType, std::string & userID, std::string & userSig, std::string & userName, std::string & userAvatar, std::string & roomID, std::string & roomInfo, std::string& strTemplate, std::string& userTag, bool& userList, bool& IMList, bool& whiteboard, bool& screenShare, bool & record, std::string & title, std::string & logo)
{
	if (root.isMember("serverDomain"))
	{
		serverDomain = root["serverDomain"].asString();
		if (serverDomain.size() > 0 && '/' == serverDomain[serverDomain.size() - 1])
		{
			serverDomain[serverDomain.size() - 1] = '\0';
		}
	}

	if (root.isMember("sdkAppID"))
	{
		sdkAppID = root["sdkAppID"].asInt();
	}

	if (root.isMember("accountType"))
	{
		accountType = root["accountType"].asString();
	}

	if (root.isMember("userID"))
	{
		userID = root["userID"].asString();
	}

	if (root.isMember("userSig"))
	{
		userSig = root["userSig"].asString();
	}

	if (root.isMember("userName"))
	{
		userName = root["userName"].asString();
	}

	if (root.isMember("userAvatar"))
	{
		userAvatar = root["userAvatar"].asString();
	}

	if (root.isMember("roomID"))
	{
		roomID = root["roomID"].asString();
	}

	if (root.isMember("roomInfo"))
	{
		roomInfo = root["roomInfo"].asString();
	}

	if (root.isMember("template"))
	{
		strTemplate = root["template"].asString();
	}

	if (root.isMember("userTag"))
	{
		userTag = root["userTag"].asString();
	}

	if (root.isMember("userList"))
	{
		userList = root["userList"].asBool();
	}

	if (root.isMember("IMList"))
	{
		IMList = root["IMList"].asBool();
	}

	if (root.isMember("whiteboard"))
	{
		whiteboard = root["whiteboard"].asBool();
	}

	if (root.isMember("screenShare"))
	{
		screenShare = root["screenShare"].asBool();
	}	
	
	if (root.isMember("record"))
	{
		record = root["record"].asBool();
	}

	if (root.isMember("title"))
	{
		title = root["title"].asString();
	}

	if (root.isMember("logo"))
	{
		logo = root["logo"].asString();
	}
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

    // �Զ��ͷ�HKEY
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