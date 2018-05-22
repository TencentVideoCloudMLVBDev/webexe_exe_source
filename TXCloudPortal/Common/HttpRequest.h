#ifndef __RTCRHttpRequest_H__
#define __RTCRHttpRequest_H__

#include "HttpClient.h"
#include "TaskQueue.h"
#include "jsoncpp/json.h"

#include <string>
#include <vector>
#include <functional>

/**
* \brief：Auth鉴权信息
*/
struct AuthData
{
    int sdkAppID;
    std::string accountType;
    std::string userID;
    std::string userSig;
    std::string userName;
    std::string userAvatar;
    std::string token;
};

/**
* \brief：房间信息
*/
struct RoomData
{
    std::string roomID;
    std::string roomInfo;
	std::string roomType;
};

/**
* \brief：错误码
*/
enum ErrorCode
{
    ROOM_SUCCESS = 0, // 成功
    ROOM_FAILED = -1, // 失败

    // 业务逻辑
    ROOM_ERR_SYSTEM_ERROR = -1001, // 系统错误
    ROOM_ERR_REQUEST_TIMEOUT = -1002, // 请求超时
    ROOM_ERR_WRONG_JSON = -1003, // 错误json串
    ROOM_ERR_IM_LOGIN = -1004, // IM登录失败
    ROOM_ERR_CREATE_ROOM = -1005, // 创建房间失败
    ROOM_ERR_ENTER_ROOM = -1006, // 加入房间失败
    ROOM_ERR_PUSH_DISCONNECT = -1007, // 推流连接断开
    ROOM_ERR_CAMERA_OCCUPY = -1008, // 摄像头占用
    ROOM_ERR_CAMERA_REMOVED = -1009, // 摄像头被拔出
    ROOM_ERR_PLAY_DISCONNECT = -1010, // 拉流连接断开

    // CGI
    ROOM_ERR_WRONG_PARAMETER = -2001, // 参数错误
    ROOM_ERR_AUTH_INVALID = -2002, // 鉴权失败 - 可能sign过期
    ROOM_ERR_DECOMPRESS_FAILED = -2003, // 解压失败
    ROOM_ERR_DECODE_FAILED = -2004, // 解码失败
    ROOM_ERR_FIELD_MISS = -2005, // 字段缺失
    ROOM_ERR_FIELD_INVALID = -2006, // 字段无效
    ROOM_ERR_UNKWON_CODE = -2007, // 未知错误码
};

// CGI请求错误码转换为LRErrorCode错误码
static ErrorCode adaptRTCErrorCode(int cgiErrorCode)
{
    ErrorCode ec = ROOM_FAILED;
    switch (cgiErrorCode)
    {
    case -1:
        ec = ROOM_FAILED;
        break;
    case 0:
        ec = ROOM_SUCCESS;
        break;
    case 1:
        ec = ROOM_ERR_WRONG_PARAMETER;
        break;
    case 2:
        ec = ROOM_ERR_AUTH_INVALID;
        break;
    case 3:
        ec = ROOM_ERR_DECOMPRESS_FAILED;
        break;
    case 4:
        ec = ROOM_ERR_DECODE_FAILED;
        break;
    case 5:
        ec = ROOM_ERR_FIELD_MISS;
        break;
    case 6:
        ec = ROOM_ERR_FIELD_INVALID;
        break;
    case 7:
        ec = ROOM_ERR_AUTH_INVALID;
        break;
    default:
        ec = ROOM_ERR_UNKWON_CODE;
        break;
    }

    return ec;
}

struct Result
{
    ErrorCode ec;     // 错误码
    std::string msg;    // 错误描述
};

typedef std::function<void(void)> reqfunction;

class HttpRequest
{
public:
    HttpRequest(const std::string& domain);
    ~HttpRequest();

    void close();

    typedef std::function<void(const Result& res, const AuthData& authData)> getLoginInfoFunc;
    void getLoginInfo(const std::string& userID, getLoginInfoFunc func);

    typedef std::function<void(const Result& res, const std::string& roomID)> createRoomFunc;
    void createRoom(const std::string& roomID, const std::string& userID, const std::string& roomInfo, const std::string& roomType, createRoomFunc func);

    typedef std::function<void(const Result& res)> destroyRoomFunc;
    void destroyRoom(const std::string& roomID, const std::string& roomType, destroyRoomFunc func);   // 同步接口

    typedef std::function<void(const Result& res, const std::vector<RoomData>& roomList)> getRoomListFunc;
    void getRoomList(int index, int cnt, const std::string& roomType, getRoomListFunc func);

    typedef std::function<void(const Result& res)> heardbeatFunc;
    void heartbeat(const std::string& roomID, const std::string& roomType, heardbeatFunc func);
private:
    std::string m_domain;
    HttpClient m_httpClient;
    TaskQueue m_taskQueue;
};


#endif /* __RTCRHttpRequest_H__ */
