#ifndef __RTCROOMUTIL_H__
#define __RTCROOMUTIL_H__

#include <vector>
#include <string>
#include <assert.h>

/**
* \brief：Auth鉴权信息
*/
struct RTCAuthData
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
* \brief：成员信息
*/
struct RTCMemberData
{
    std::string userID;
    std::string userName;
    std::string userAvatar;
    std::string accelerateURL;
};

/**
* \brief：房间信息
*/
struct RTCRoomData
{
    std::string roomID;
    std::string roomInfo;
    std::string roomCreator;
    std::vector<RTCMemberData> members;
};

/**
* \brief：错误码
*/
enum RTCErrorCode
{
    RTCROOM_SUCCESS = 0, // 成功
    RTCROOM_FAILED = -1, // 失败

    // 业务逻辑
    RTCROOM_ERR_SYSTEM_ERROR = -1001, // 系统错误
    RTCROOM_ERR_REQUEST_TIMEOUT = -1002, // 请求超时
    RTCROOM_ERR_WRONG_JSON = -1003, // 错误json串
    RTCROOM_ERR_IM_LOGIN = -1004, // IM登录失败
    RTCROOM_ERR_CREATE_ROOM = -1005, // 创建房间失败
    RTCROOM_ERR_ENTER_ROOM = -1006, // 加入房间失败
    RTCROOM_ERR_PUSH_DISCONNECT = -1007, // 推流连接断开
    RTCROOM_ERR_CAMERA_OCCUPY = -1008, // 摄像头占用
	RTCROOM_ERR_CAMERA_REMOVED = -1009, // 摄像头被拔出
    RTCROOM_ERR_CAMERA_MISSED = -1010, // 无可用摄像头
    RTCROOM_ERR_PLAY_DISCONNECT = -1011, // 拉流连接断开

    // CGI
    RTCROOM_ERR_WRONG_PARAMETER = -2001, // 参数错误
    RTCROOM_ERR_AUTH_INVALID = -2002, // 鉴权失败 - 可能sign过期
    RTCROOM_ERR_DECOMPRESS_FAILED = -2003, // 解压失败
    RTCROOM_ERR_DECODE_FAILED = -2004, // 解码失败
    RTCROOM_ERR_FIELD_MISS = -2005, // 字段缺失
    RTCROOM_ERR_FIELD_INVALID = -2006, // 字段无效
    RTCROOM_ERR_UNKWON_CODE = -2007, // 未知错误码
};

// CGI请求错误码转换为LRErrorCode错误码
static RTCErrorCode adaptRTCErrorCode(int cgiErrorCode)
{
    RTCErrorCode ec = RTCROOM_FAILED;
    switch (cgiErrorCode)
    {
    case -1:
        ec = RTCROOM_FAILED;
        break;
    case 0:
        ec = RTCROOM_SUCCESS;
        break;
    case 1:
        ec = RTCROOM_ERR_WRONG_PARAMETER;
        break;
    case 2:
        ec = RTCROOM_ERR_AUTH_INVALID;
        break;
    case 3:
        ec = RTCROOM_ERR_DECOMPRESS_FAILED;
        break;
    case 4:
        ec = RTCROOM_ERR_DECODE_FAILED;
        break;
    case 5:
        ec = RTCROOM_ERR_FIELD_MISS;
        break;
    case 6:
        ec = RTCROOM_ERR_FIELD_INVALID;
        break;
    case 7:
        ec = RTCROOM_ERR_AUTH_INVALID;
        break;
    default:
        ec = RTCROOM_ERR_UNKWON_CODE;
        break;
    }

    return ec;
}

struct RTCResult
{
    RTCErrorCode ec;     // 错误码
    std::string msg;    // 错误描述
};

/**
* \brief：视频宽高比
*/
enum RTCAspectRatio
{
    RTCROOM_ASPECT_RATIO_4_3 = 0,
    RTCROOM_ASPECT_RATIO_16_9 = 1,
    RTCROOM_ASPECT_RATIO_9_16 = 2,
};

/**
* \brief：画质预设选项
*/
enum RTCVideoQuality
{
    RTCROOM_VIDEO_QUALITY_STANDARD_DEFINITION = 1,         // 标清：建议追求流畅性的客户使用该选项
    RTCROOM_VIDEO_QUALITY_HIGH_DEFINITION = 2,             // 高清：建议对清晰度有要求的客户使用该选项
    RTCROOM_VIDEO_QUALITY_SUPER_DEFINITION = 3,            // 超清：如果不是大屏观看，不推荐使用
};

/**
* \brief：设置美颜风格
*/
enum RTCBeautyStyle
{
    RTCROOM_BEAUTY_STYLE_SMOOTH = 0,        // 光滑
    RTCROOM_BEAUTY_STYLE_NATURE = 1,        // 自然
    RTCROOM_BEAUTY_STYLE_BLUR = 2,          // 朦胧
};

class ILoginRTCCallback
{
public:
	virtual ~ILoginRTCCallback() {}

    /**
    * \brief：login登录
    * \param：res - 执行结果，参考 RTCRoomUtil.h 中定义的 RTCResult 结构体
    * \param：authData - RoomService提供的登录信息，包括IM相关的配置字段，在login成功后，获取到token字段
    */
	virtual void onLogin(const RTCResult& res, const RTCAuthData& authData) = 0;
};

class IGetRTCRoomListCallback
{
public:
    virtual ~IGetRTCRoomListCallback() {}

    /**
    * \brief：获取房间列表
    * \param：res - 执行结果，参考 RTCRoomUtil.h 中定义的 RTCResult 结构体
    * \param：rooms - 房间信息的列表
    */
    virtual void onGetRoomList(const RTCResult& res, const std::vector<RTCRoomData>& rooms) = 0;
};

class IRTCRoomCallback
{
public:
    virtual ~IRTCRoomCallback() {}

    /**
    * \brief：创建房间的回调
    * \param：res - 执行结果，参考 RTCRoomUtil.h 中定义的 RTCResult 结构体
    * \param：roomID - 房间ID
    */
    virtual void onCreateRoom(const RTCResult& res, const std::string& roomID) = 0;

    /**
    * \brief：进入房间的回调
    * \param：res - 执行结果，参考 RTCRoomUtil.h 中定义的 RTCResult 结构体
    */
    virtual void onEnterRoom(const RTCResult& res) = 0;

    /**
    * \brief：房间信息变更的回调
    * \param：res - 执行结果，参考 RTCRoomUtil.h 中定义的 RTCResult 结构体
    * \param：roomData - 房间信息，参考 RTCRoomUtil.h 中定义的 LRRoomData 结构体
    */
    virtual void onUpdateRoomData(const RTCResult& res, const RTCRoomData& roomData) = 0;

    /**
    * \brief：成员进入房间的回调
    * \param：member - 成员信息，参考 RTCRoomUtil.h 中定义的 LRMemberData 结构体
    */
    virtual void onPusherJoin(const RTCMemberData& member) = 0;

    /**
    * \brief：成员退出房间的回调
    * \param：member - 成员信息，参考 RTCRoomUtil.h 中定义的 LRMemberData 结构体
    */
    virtual void onPusherQuit(const RTCMemberData& member) = 0;

    /**
    * \brief：房间解散的回调
    * \param：roomID - 房间ID
    */
	virtual void onRoomClosed(const std::string& roomID) = 0;

	/**
	* \brief：收到普通文本消息
	* \param：roomID - 房间ID
	* \param userID - 发送者ID
	* \param userName - 发送者昵称
	* \param userAvatar - 发送者头像
	* \param message - 文本消息内容
	*/
	virtual void onRecvRoomTextMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * msg) = 0;

	/**
	* \brief：收到房间自定义消息
	* \param：roomID - 房间ID
	* \param userID - 发送者ID
	* \param userName - 发送者昵称
	* \param userAvatar - 发送者头像
	* \param cmd - 自定义cmd
	* \param message - 自定义消息内容
	*/
	virtual void onRecvRoomCustomMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * msg) = 0;

    /**
    * \brief：收到C2C自定义消息
    * \param：roomID - 房间ID
    * \param userID - 发送者ID
    * \param userName - 发送者昵称
    * \param userAvatar - 发送者头像
    * \param cmd - 自定义cmd
    * \param message - 自定义消息内容
    */
    virtual void onRecvC2CCustomMsg(const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * msg) = 0;

    /**
    * \brief：同一个账号重复登录或者后台强制下线，都会触发该事件
    */
    virtual void onTIMKickOffline() = 0;

    /**
    * \brief：LiveRoom内部发生错误的通知
    * \param：res - 执行结果，参考 RTCRoomUtil.h 中定义的 RTCResult 结构体
    * \param：userID - 用户ID
    */
    virtual void onError(const RTCResult& res, const std::string& userID) = 0;
};

#endif
