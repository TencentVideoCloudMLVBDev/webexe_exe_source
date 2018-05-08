#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <QtCore>

#include "HttpServer.h"
#include "json.h"
#include "PushPlayDemo.h"
#include "LiveDemo.h"
#include "RTCDemo.h"
#include "DialogPushPlay.h"
#include "HttpServer.h"

typedef std::function<void(void)> ApplicationFunction;

class CLocalHttpServer;

class Application
    : public QObject
    , public IHttpServerCallback
{
    Q_OBJECT

protected:
    Application(QObject *parent = Q_NULLPTR);
    Application(const Application&);
    Application& operator =(const Application&);
public:
    virtual ~Application();
    static Application& instance();

    int run(int &argc, char **argv);
    void quit(int retcode = 0);

    void pushSDKEvent(int eventID, const std::map<std::string, std::string>& params);
    void pushRoomStatus(int code, const std::string& msgUTF8);
    void pushRoomTextMsg(const std::string& roomID, const std::string& userID, const std::string& userName
        , const std::string& userAvatar, const std::string& msg);
    void pushMemberChange(const std::list<MemberItem>& members);

    void executeInMainThread(ApplicationFunction func);     // 主线程执行
signals:
    void dispatch(ApplicationFunction func);                // 投递线程队列
protected slots:
    void handle(ApplicationFunction func);                  // 执行函数
protected:
    virtual void onGetRequest(const std::wstring& absPath, DWORD& statusCode, std::string& respDataUTF8);
    virtual void onLog(HSLogLevel level, const std::string& content);
    virtual void onClose(ULONGLONG requestId);
private:
    void handleReqQuery(const std::wstring& absPath, DWORD& statusCode, std::string& respDataUTF8);
    void handleReqQuit(const std::wstring& absPath, DWORD& statusCode, std::string& respDataUTF8);

    bool resolveProtol(const std::string& json);
    bool resolveNormalLiveProtol(const Json::Value& root);
    bool resolveCSLiveProtol(const Json::Value& root);
    bool resolveLiveRoomProtol(const Json::Value& root);
    bool resolveRTCRoomProtol(const Json::Value& root);

	void getConfigInfo(const Json::Value& root, std::string& serverDomain, int& sdkAppID, std::string& accountType, std::string& userID,
		std::string& userSig, std::string& userName, std::string& userAvatar, std::string& roomID, std::string& roomInfo,
		std::string& strTemplate, std::string& userTag, bool & userList, bool & IMList, bool & whiteboard, bool & screenShare, bool & record, std::string& title, std::string& logo);
    bool regProtol();   // 注册表注册伪协议
private:
    ULONG_PTR m_gdiplusToken;
    PushPlayDemo* m_normalLive;
    DialogPushPlay* m_csLive;
    LiveDemo* m_liveDemo;
    RTCDemo* m_RTCDemo;

    int m_httpPort;
    HttpServer m_httpServer;
    std::list<Json::Value> m_dataList;  // 缓存推送包
    const size_t m_maxDataListCount;    // 最大缓存数量
    std::mutex m_mutex;
};
