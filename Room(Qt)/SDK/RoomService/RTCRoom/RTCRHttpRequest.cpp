#include "RTCRHttpRequest.h"
#include "log.h"
#include "Base.h"

#include <assert.h>
#include <sstream>

RTCRHttpRequest::RTCRHttpRequest()
    : m_domain("")
    , m_httpClient(L"User-Agent")
    , m_taskQueue()
    , m_authData()
{

}

RTCRHttpRequest::~RTCRHttpRequest()
{
    close();
    m_taskQueue.wait();
}

void RTCRHttpRequest::close()
{
    m_taskQueue.quit();
    m_httpClient.http_close();
}

void RTCRHttpRequest::setProxy(const std::string& ip, unsigned short port)
{
    m_httpClient.setProxy(ip, port);
}

void RTCRHttpRequest::getLoginInfo(const std::string& domain, const std::string& userID, getLoginInfoFunc func)
{
    //m_taskQueue.post([=]() { todo µ÷ÕûRoomList
    std::stringstream buffer;
    if (userID.empty())
    {
        buffer << domain << "/get_login_info";
    }
    else
    {
        buffer << domain << "/get_login_info?userID=" << userID;
    }

    std::wstring url = Ansi2Wide(buffer.str());

    std::vector<std::wstring> headers;
    headers.push_back(L"Content-Type: application/json; charset=utf-8");

    std::string respData;
    DWORD ret = m_httpClient.http_get(url, headers, respData);
    LINFO(L"ret: %lu, respData: %s", ret, UTF82Wide(respData).c_str());
    if (0 != ret || true == respData.empty())
    {
        func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" }, RTCAuthData());
        return;
    }

    Json::Reader reader;
    Json::Value data;
    if (!reader.parse(respData, data))
    {
        func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" }, RTCAuthData());
        return;
    }

    int code = -1;
    if (data.isMember("code"))
    {
        code = data["code"].asInt();
    }

    std::string message;
    if (data.isMember("message"))
    {
        message = data["message"].asString();
    }

    if (code != 0)
    {
        func({ adaptRTCErrorCode(code), message }, RTCAuthData());
        return;
    }

	RTCAuthData authData;
    if (data.isMember("sdkAppID"))
    {
        authData.sdkAppID = data["sdkAppID"].asInt();
    }

    if (data.isMember("accType"))
    {
        authData.accountType = data["accType"].asString();
    }

    if (data.isMember("userID"))
    {
        authData.userID = data["userID"].asString();
    }

    if (data.isMember("userSig"))
    {
        authData.userSig = data["userSig"].asString();
    }

    func({ RTCROOM_SUCCESS, "" }, authData);
    //});
}

void RTCRHttpRequest::login(const std::string& domain, const RTCAuthData & authData, loginFunc func)
{
    m_domain = domain;
    m_authData = authData;

    m_taskQueue.post([=]() {
        std::stringstream buffer;
        buffer << m_domain << "/login?sdkAppID=" << m_authData.sdkAppID
            << "&accountType=" << m_authData.accountType << "&userID=" << m_authData.userID
            << "&userSig=" << m_authData.userSig;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, std::string(), respData);
        LINFO(L"url: %s, ret: %lu, respData: %s", url.c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" }, std::string(), std::string());
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" }, std::string(), std::string());
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message }, std::string(), std::string());
            return;
        }

        std::string userID;
        if (data.isMember("userID"))
        {
            userID = data["userID"].asString();
        }

        std::string token;
        if (data.isMember("token"))
        {
            token = data["token"].asString();
        }

        m_authData.token = token;

        func({ RTCROOM_SUCCESS, message }, userID, token);
    });
}

void RTCRHttpRequest::logout(logoutFunc func)
{
    m_taskQueue.post(true, [=]() {
        std::stringstream buffer;
        buffer << m_domain << "/logout?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, std::string(), respData);
        LINFO(L"url: %s, ret: %lu, respData: %s", url.c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" });
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "Json parse failed" });
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message });
            return;
        }

        func({ RTCROOM_SUCCESS, message });
    });
}

void RTCRHttpRequest::heartbeat(const std::string& roomID, const std::string& userID, heartbeatFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["userID"] = userID;
        jsonObj["roomID"] = roomID;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/pusher_heartbeat?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" });
            return;
        }

        func({ RTCROOM_SUCCESS, "" });
    });
}

void RTCRHttpRequest::getRoomList(int index, int cnt, getRoomListFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["cnt"] = cnt;
        jsonObj["index"] = index;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/get_room_list?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" }, std::vector<RTCRoomData>());
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "Json parse failed" }, std::vector<RTCRoomData>());
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message }, std::vector<RTCRoomData>());
            return;
        }

        Json::Value rooms = data["rooms"];
        if (!rooms.isArray())
        {
            func({ RTCROOM_ERR_WRONG_JSON, "Json parse failed" }, std::vector<RTCRoomData>());
            return;
        }

        std::vector<RTCRoomData> roomList;
        for (int i = 0; i < rooms.size(); ++i)
        {
            Json::Value roomObj = rooms[i];

			RTCRoomData newRoom;
            newRoom.roomID = roomObj["roomID"].asString();
            newRoom.roomInfo = roomObj["roomInfo"].asString();
            newRoom.roomCreator = roomObj["roomCreator"].asString();

            if (roomObj.isMember("pushers") && roomObj["pushers"].isArray())
            {
                Json::Value pusherArray = roomObj["pushers"];

                for (int i = 0; i < pusherArray.size(); ++i)
                {
                    Json::Value pusherObj = pusherArray[i];

					RTCMemberData member;
                    member.userAvatar = pusherObj["userAvatar"].asString();
                    member.userName = pusherObj["userName"].asString();
                    member.userID = pusherObj["userID"].asString();
                    member.accelerateURL = pusherObj["accelerateURL"].asString();
                    newRoom.members.push_back(member);
                }
            }

            roomList.push_back(newRoom);
        }

        func({ RTCROOM_SUCCESS, message }, roomList);
    });
}

void RTCRHttpRequest::createRoom(const std::string& roomID, const std::string& roomInfo, createRoomFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["userID"] = m_authData.userID;
        jsonObj["roomID"] = roomID;
        jsonObj["roomInfo"] = roomInfo;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/create_room?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" }, std::string());
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" }, std::string());
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message }, std::string());
            return;
        }

        std::string roomID;
        if (data.isMember("roomID"))
        {
            roomID = data["roomID"].asString();
        }

        func({ RTCROOM_SUCCESS, message }, roomID);
    });
}

void RTCRHttpRequest::destroyRoom(const std::string& roomID, const std::string& userID, destroyRoomFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["roomID"] = roomID;
        jsonObj["userID"] = userID;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/destroy_room?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" });
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" });
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message });
            return;
        }

        func({ RTCROOM_SUCCESS, message });
    });
}

void RTCRHttpRequest::getPushURL(const std::string& userID, getPushURLFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["userID"] = userID;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/get_push_url?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" }, std::string());
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" }, std::string());
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message }, std::string());
            return;
        }

        std::string pushURL;
        if (data.isMember("pushURL"))
        {
            pushURL = data["pushURL"].asString();
        }

        func({ RTCROOM_SUCCESS, message }, pushURL);
    });
}

void RTCRHttpRequest::getPushers(const std::string& roomID, getPushersFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["roomID"] = roomID;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/get_pushers?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" }, RTCRoomData());
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" }, RTCRoomData());
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message }, RTCRoomData());
            return;
        }

		RTCRoomData roomData;
        if (data.isMember("roomID"))
        {
            roomData.roomID = data["roomID"].asString();
        }

        if (data.isMember("roomInfo"))
        {
            roomData.roomInfo = data["roomInfo"].asString();
        }

        if (data.isMember("roomCreator"))
        {
            roomData.roomCreator = data["roomCreator"].asString();
        }

        if (data.isMember("pushers") && data["pushers"].isArray())
        {
            Json::Value pushers = data["pushers"];

            for (int i = 0; i < pushers.size(); ++i)
            {
                Json::Value pusherObj = pushers[i];

				RTCMemberData member;
                member.userAvatar = pusherObj["userAvatar"].asString();
                member.userName = pusherObj["userName"].asString();
                member.userID = pusherObj["userID"].asString();
                member.accelerateURL = pusherObj["accelerateURL"].asString();
                roomData.members.push_back(member);
            }
        }

        func({ RTCROOM_SUCCESS, message }, roomData);
    });
}

void RTCRHttpRequest::addPusher(const std::string& roomID, const std::string& userID, const std::string& userName
    , const std::string& userAvatar, const std::string& pushURL, addPusherFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["roomID"] = roomID;
        jsonObj["userID"] = userID;
        jsonObj["userName"] = userName;
        jsonObj["userAvatar"] = userAvatar;
        jsonObj["pushURL"] = pushURL;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/add_pusher?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" });
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" });
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message });
            return;
        }

        func({ RTCROOM_SUCCESS, message });
    });
}

void RTCRHttpRequest::deletePusher(const std::string& roomID, const std::string& userID, deletePusherFunc func)
{
    m_taskQueue.post(true, [=]() {
        Json::Value jsonObj;
        jsonObj["userID"] = userID;
        jsonObj["roomID"] = roomID;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/delete_pusher?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" });
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" });
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message });
            return;
        }

        func({ RTCROOM_SUCCESS, message });
    });
}

void RTCRHttpRequest::mergeStream(const std::string& roomID, const std::string& userID, const Json::Value& mergeParams, mergeStreamFunc func)
{
    m_taskQueue.post([=] {
        Json::Value jsonObj;
        jsonObj["roomID"] = roomID;
        jsonObj["userID"] = userID;
        jsonObj["mergeParams"] = mergeParams;

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "/merge_stream?userID=" << m_authData.userID << "&token=" << m_authData.token;

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ RTCROOM_ERR_SYSTEM_ERROR, "Http request failed" });
            return;
        }

        Json::Reader reader;
        Json::Value data;
        if (!reader.parse(respData, data))
        {
            func({ RTCROOM_ERR_WRONG_JSON, "json parse failed" });
            return;
        }

        int code = -1;
        if (data.isMember("code"))
        {
            code = data["code"].asInt();
        }

        std::string message;
        if (data.isMember("message"))
        {
            message = data["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message });
            return;
        }

        Json::Value result;
        if (data.isMember("result"))
        {
            result = data["result"];
        }

        if (result.isMember("code"))
        {
            code = result["code"].asInt();
        }

        if (result.isMember("message"))
        {
            message = result["message"].asString();
        }

        if (code != 0)
        {
            func({ adaptRTCErrorCode(code), message });
            return;
        }

        func({ RTCROOM_SUCCESS, message });
    });
}
