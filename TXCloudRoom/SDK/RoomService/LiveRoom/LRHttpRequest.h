#ifndef __LRHTTPREQUEST_H__
#define __LRHTTPREQUEST_H__

#include "HttpClient.h"
#include "LiveRoomUtil.h"
#include "TaskQueue.h"
#include "json.h"

#include <string>
#include <vector>
#include <functional>

typedef std::function<void(void)> reqfunction;

class LRHttpRequest
{
public:
    LRHttpRequest();
    ~LRHttpRequest();

    void close();

    void setProxy(const std::string& ip, unsigned short port);

    typedef std::function<void(const LRResult& res, const LRAuthData& authData)> getLoginInfoFunc;
    void getLoginInfo(const std::string& domain, const std::string& userID, getLoginInfoFunc func);

    typedef std::function<void(const LRResult& res, const std::string& userID, const std::string& token)> loginFunc;
    void login(const std::string& domain, const LRAuthData & authData, loginFunc func);

    typedef std::function<void(const LRResult& res)> logoutFunc;
    void logout(logoutFunc func);       // logout网络请求，内部确保执行

    typedef std::function<void(const LRResult& res)> heartbeatFunc;
    void heartbeat(const std::string& roomID, const std::string& userID, heartbeatFunc func);

    typedef std::function<void(const LRResult& res, const std::vector<LRRoomData>& roomList)> getRoomListFunc;
    void getRoomList(int index, int cnt, getRoomListFunc func);

    typedef std::function<void(const LRResult& res, const std::string& roomID)> createRoomFunc;
    void createRoom(const std::string& roomID, const std::string& roomInfo, createRoomFunc func);

    typedef std::function<void(const LRResult& res)> destroyRoomFunc;
    void destroyRoom(const std::string& roomID, const std::string& userID, destroyRoomFunc func);

    typedef std::function<void(const LRResult& res, const std::string& pushURL)> getPushURLFunc;
    void getPushURL(const std::string& userID, getPushURLFunc func);

    typedef std::function<void(const LRResult& res, const LRRoomData& roomData)> getPushersFunc;
    void getPushers(const std::string& roomID, getPushersFunc func);

    typedef std::function<void(const LRResult& res)> addPusherFunc;
    void addPusher(const std::string& roomID, const std::string& userID, const std::string& userName
        , const std::string& userAvatar, const std::string& pushURL, addPusherFunc func);

    typedef std::function<void(const LRResult& res)> deletePusherFunc;
    void deletePusher(const std::string& roomID, const std::string& userID, deletePusherFunc func);

    typedef std::function<void(const LRResult& res)> addAudienceFunc;
    void addAudience(const std::string& roomID, const std::string& userID, const std::string& userInfo, addAudienceFunc func);

    typedef std::function<void(const LRResult& res)> deleteAudienceFunc;
    void deleteAudience(const std::string& roomID, const std::string& userID, deleteAudienceFunc func);

    typedef std::function<void(const LRResult& res, const std::vector<LRAudienceData>& audiences)> getAudienceListFunc;
    void getAudienceList(const std::string& roomID, getAudienceListFunc func);

    typedef std::function<void(const LRResult& res)> mergeStreamFunc;
    void mergeStream(const std::string& roomID, const std::string& userID, const Json::Value& mergeParams, mergeStreamFunc func);
private:
    std::string m_domain;
    HttpClient m_httpClient;
    TaskQueue m_taskQueue;
    LRAuthData m_authData;
};


#endif /* __LRHTTPREQUEST_H__ */
