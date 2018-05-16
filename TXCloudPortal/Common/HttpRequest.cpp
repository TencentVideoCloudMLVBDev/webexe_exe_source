#include "HttpRequest.h"
#include "log.h"
#include "Base.h"

#include <assert.h>
#include <sstream>

HttpRequest::HttpRequest(const std::string& domain)
    : m_domain(domain)
    , m_httpClient(L"User-Agent")
    , m_taskQueue()
{

}

HttpRequest::~HttpRequest()
{
    close();
    m_taskQueue.wait();
}

void HttpRequest::close()
{
    m_taskQueue.quit();
    m_httpClient.http_close();
}

void HttpRequest::getLoginInfo(const std::string& userID, getLoginInfoFunc func)
{
    //m_taskQueue.post([=]() { todo µ÷ÕûRTCRoomList
    Json::Value jsonObj;
    jsonObj["userID"] = userID;

    Json::FastWriter writer;
    std::string jsonStr = writer.write(jsonObj);

    std::stringstream buffer;
    buffer << m_domain << "get_login_info";

    std::wstring url = Ansi2Wide(buffer.str());

    std::vector<std::wstring> headers;
    headers.push_back(L"Content-Type: application/json; charset=utf-8");

    std::string respData;
    DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
    LINFO(L"ret: %lu, respData: %s", ret, UTF82Wide(respData).c_str());
    if (0 != ret || true == respData.empty())
    {
        func({ ROOM_ERR_SYSTEM_ERROR, "Http request failed" }, AuthData());
        return;
    }

    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(respData, root))
    {
        func({ ROOM_ERR_WRONG_JSON, "json parse failed" }, AuthData());
        return;
    }

    int code = -1;
    if (root.isMember("code"))
    {
        code = root["code"].asInt();
    }

    std::string message;
    if (root.isMember("message"))
    {
        message = root["message"].asString();
    }

    if (code != 200)
    {
        func({ adaptRTCErrorCode(code), message }, AuthData());
        return;
    }

    Json::Value data;
    if (root.isMember("data"))
    {
        data = root["data"];
    }

    AuthData authData;
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

    func({ ROOM_SUCCESS, "" }, authData);
    //});
}

void HttpRequest::createRoom(const std::string& roomID, const std::string& roomInfo, RoomType roomType, createRoomFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["roomID"] = roomID;
        jsonObj["roomInfo"] = roomInfo;
        jsonObj["roomType"] = static_cast<int>(roomType);

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "create_room";

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ ROOM_ERR_SYSTEM_ERROR, "Http request failed" }, std::string());
            return;
        }

        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(respData, root))
        {
            func({ ROOM_ERR_WRONG_JSON, "json parse failed" }, std::string());
            return;
        }

        int code = -1;
        if (root.isMember("code"))
        {
            code = root["code"].asInt();
        }

        std::string message;
        if (root.isMember("message"))
        {
            message = root["message"].asString();
        }

        if (code != 200)
        {
            func({ adaptRTCErrorCode(code), message }, std::string());
            return;
        }

        Json::Value data;
        if (root.isMember("data"))
        {
            data = root["data"];
        }

        std::string roomID;
        if (data.isMember("roomID"))
        {
            roomID = data["roomID"].asString();
        }

        func({ ROOM_SUCCESS, message }, roomID);
    });
}

void HttpRequest::destroyRoom(const std::string& roomID, RoomType roomType, destroyRoomFunc func)
{
    m_taskQueue.post(true, [=]() {
        Json::Value jsonObj;
        jsonObj["roomID"] = roomID;
        jsonObj["roomType"] = static_cast<int>(roomType);

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "delete_room";

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ ROOM_ERR_SYSTEM_ERROR, "Http request failed" });
            return;
        }

        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(respData, root))
        {
            func({ ROOM_ERR_WRONG_JSON, "json parse failed" });
            return;
        }

        int code = -1;
        if (root.isMember("code"))
        {
            code = root["code"].asInt();
        }

        std::string message;
        if (root.isMember("message"))
        {
            message = root["message"].asString();
        }

        if (code != 200)
        {
            func({ adaptRTCErrorCode(code), message });
            return;
        }

        func({ ROOM_SUCCESS, message });
    });
}

void HttpRequest::getRoomList(int index, int cnt, RoomType roomType, getRoomListFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["count"] = cnt;
        jsonObj["index"] = index;
        jsonObj["roomType"] = static_cast<int>(roomType);

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "get_room_list";

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ ROOM_ERR_SYSTEM_ERROR, "Http request failed" }, std::vector<RoomData>());
            return;
        }

        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(respData, root))
        {
            func({ ROOM_ERR_WRONG_JSON, "Json parse failed" }, std::vector<RoomData>());
            return;
        }

        int code = -1;
        if (root.isMember("code"))
        {
            code = root["code"].asInt();
        }

        std::string message;
        if (root.isMember("message"))
        {
            message = root["message"].asString();
        }

        if (code != 200)
        {
            func({ adaptRTCErrorCode(code), message }, std::vector<RoomData>());
            return;
        }

        Json::Value data;
        if (root.isMember("data"))
        {
            data = root["data"];
        }

        Json::Value rooms = data["list"];
        if (false == rooms.isArray())
        {
            func({ ROOM_ERR_WRONG_JSON, "Json parse failed" }, std::vector<RoomData>());
            return;
        }

        std::vector<RoomData> roomList;
        for (int i = 0; i < rooms.size(); ++i)
        {
            Json::Value roomObj = rooms[i];

            RoomData newRoom;
            newRoom.roomID = roomObj["roomID"].asString();
            newRoom.roomInfo = roomObj["roomInfo"].asString();
            newRoom.roomType = static_cast<RoomType>(roomObj["roomType"].asInt());

            roomList.push_back(newRoom);
        }

        func({ ROOM_SUCCESS, message }, roomList);
    });
}

void HttpRequest::heartbeat(const std::string& roomID, RoomType roomType, heardbeatFunc func)
{
    m_taskQueue.post([=]() {
        Json::Value jsonObj;
        jsonObj["roomID"] = roomID;
        jsonObj["roomType"] = static_cast<int>(roomType);

        Json::FastWriter writer;
        std::string jsonStr = writer.write(jsonObj);

        std::wstring contentLength = format(L"Content-Length: %lu", jsonStr.size());

        std::stringstream buffer;
        buffer << m_domain << "room_heartbeat";

        std::wstring url = Ansi2Wide(buffer.str());

        std::vector<std::wstring> headers;
        headers.push_back(L"Content-Type: application/json; charset=utf-8");
        headers.push_back(contentLength);

        std::string respData;
        DWORD ret = m_httpClient.http_post(url, headers, jsonStr, respData);
        LINFO(L"jsonStr: %s, ret: %lu, respData: %s", Ansi2Wide(jsonStr).c_str(), ret, UTF82Wide(respData).c_str());
        if (0 != ret || true == respData.empty())
        {
            func({ ROOM_ERR_SYSTEM_ERROR, "Http request failed" });
            return;
        }

        Json::Reader reader;
        Json::Value root;
        if (!reader.parse(respData, root))
        {
            func({ ROOM_ERR_WRONG_JSON, "json parse failed" });
            return;
        }

        int code = -1;
        if (root.isMember("code"))
        {
            code = root["code"].asInt();
        }

        std::string message;
        if (root.isMember("message"))
        {
            message = root["message"].asString();
        }

        if (code != 200)
        {
            func({ adaptRTCErrorCode(code), message });
            return;
        }

        func({ ROOM_SUCCESS, message });
    });
}
