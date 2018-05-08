#ifndef __RTCRHttpRequest_H__
#define __RTCRHttpRequest_H__

#include "HttpClient.h"
#include "RTCRoomUtil.h"
#include "TaskQueue.h"
#include "json.h"

#include <string>
#include <vector>
#include <functional>

typedef std::function<void(void)> reqfunction;

class RTCRHttpRequest
{
public:
    RTCRHttpRequest();
    ~RTCRHttpRequest();

    void close();

    void setProxy(const std::string& ip, unsigned short port);

    typedef std::function<void(const RTCResult& res, const RTCAuthData& authData)> getLoginInfoFunc;
    void getLoginInfo(const std::string& domain, const std::string& userID, getLoginInfoFunc func);

    typedef std::function<void(const RTCResult& res, const std::string& userID, const std::string& token)> loginFunc;
    void login(const std::string& domain, const RTCAuthData & authData, loginFunc func);

    typedef std::function<void(const RTCResult& res)> logoutFunc;
    void logout(logoutFunc func);

    typedef std::function<void(const RTCResult& res)> heartbeatFunc;
    void heartbeat(const std::string& roomID, const std::string& userID, heartbeatFunc func);

    typedef std::function<void(const RTCResult& res, const std::vector<RTCRoomData>& roomList)> getRoomListFunc;
    void getRoomList(int index, int cnt, getRoomListFunc func);

    typedef std::function<void(const RTCResult& res, const std::string& roomID)> createRoomFunc;
    void createRoom(const std::string& roomID, const std::string& roomInfo, createRoomFunc func);

    typedef std::function<void(const RTCResult& res)> destroyRoomFunc;
    void destroyRoom(const std::string& roomID, const std::string& userID, destroyRoomFunc func);

    typedef std::function<void(const RTCResult& res, const std::string& pushURL)> getPushURLFunc;
    void getPushURL(const std::string& userID, getPushURLFunc func);

    typedef std::function<void(const RTCResult& res, const RTCRoomData& roomData)> getPushersFunc;
    void getPushers(const std::string& roomID, getPushersFunc func);

    typedef std::function<void(const RTCResult& res)> addPusherFunc;
    void addPusher(const std::string& roomID, const std::string& userID, const std::string& userName
        , const std::string& userAvatar, const std::string& pushURL, addPusherFunc func);

    typedef std::function<void(const RTCResult& res)> deletePusherFunc;
    void deletePusher(const std::string& roomID, const std::string& userID, deletePusherFunc func);

    typedef std::function<void(const RTCResult& res)> mergeStreamFunc;
    void mergeStream(const std::string& roomID, const std::string& userID, const Json::Value& mergeParams, mergeStreamFunc func);
private:
    std::string m_domain;
    HttpClient m_httpClient;
    TaskQueue m_taskQueue;
	RTCAuthData m_authData;
};


#endif /* __RTCRHttpRequest_H__ */
