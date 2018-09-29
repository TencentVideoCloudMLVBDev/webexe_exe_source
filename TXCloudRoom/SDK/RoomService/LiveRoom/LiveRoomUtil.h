#ifndef __LIVEROOMUTIL_H__
#define __LIVEROOMUTIL_H__

#include <vector>
#include <string>
#include <assert.h>

/**
* \brief：Auth鉴权信息，通过RoomService后台API接口请求得到
*/
struct LRAuthData
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
* \brief：角色类型
*/
enum LRRole
{
    LRNullRole,     // 角色未知，未进入房间或者退出房间
    LRMainRole,     // 大主播
    LRSubRole,      // 小主播
    LRAudience      // 观众
};

/**
* \brief：成员信息
*/
struct LRMemberData
{
	std::string userID;
	std::string userName;
	std::string userAvatar;
	std::string accelerateURL;
};

/**
* \brief：观众信息
*/
struct LRAudienceData
{
    std::string userID;
    std::string userInfo;
};

/**
* \brief：房间信息
*/
struct LRRoomData
{
	std::string roomID;
	std::string roomInfo;
	std::string roomCreator;
	std::string mixedPlayURL;
	int audienceCount;
	std::vector<LRMemberData> members;
};

/**
* \brief：错误码
*/
enum LRErrorCode
{
	LIVEROOM_SUCCESS = 0, // 成功
	LIVEROOM_FAILED = -1, // 失败

	// 业务逻辑
	LIVEROOM_ERR_SYSTEM_ERROR = -1001, // 系统错误
	LIVEROOM_ERR_REQUEST_TIMEOUT = -1002, // 请求超时
	LIVEROOM_ERR_WRONG_JSON = -1003, // 错误json串
	LIVEROOM_ERR_IM_LOGIN = -1004, // IM登录失败
	LIVEROOM_ERR_CREATE_ROOM = -1005, // 创建房间失败
	LIVEROOM_ERR_ENTER_ROOM = -1006, // 加入房间失败
	LIVEROOM_ERR_PUSH_DISCONNECT = -1007, // 推流连接断开
	LIVEROOM_ERR_CAMERA_OCCUPY = -1008, // 摄像头占用
	LIVEROOM_ERR_CAMERA_REMOVED = -1009, // 摄像头被拔出
    LIVEROOM_ERR_CAMERA_MISSED = -1010, // 无可用摄像头
    LIVEROOM_ERR_PLAY_DISCONNECT = -1011, // 拉流连接断开

	// CGI
	LIVEROOM_ERR_WRONG_PARAMETER = -2001, // 参数错误
	LIVEROOM_ERR_AUTH_INVALID = -2002, // 鉴权失败 - 可能sign过期
	LIVEROOM_ERR_DECOMPRESS_FAILED = -2003, // 解压失败
	LIVEROOM_ERR_DECODE_FAILED = -2004, // 解码失败
	LIVEROOM_ERR_FIELD_MISS = -2005, // 字段缺失
	LIVEROOM_ERR_FIELD_INVALID = -2006, // 字段无效
	LIVEROOM_ERR_UNKWON_CODE = -2007, // 未知错误码
};

// CGI请求错误码转换为LRErrorCode错误码
static LRErrorCode adaptLiveErrorCode(int cgiErrorCode)
{
	LRErrorCode ec = LIVEROOM_FAILED;
	switch (cgiErrorCode)
	{
	case -1:
		ec = LIVEROOM_FAILED;
		break;
	case 0:
		ec = LIVEROOM_SUCCESS;
		break;
	case 1:
		ec = LIVEROOM_ERR_WRONG_PARAMETER;
		break;
	case 2:
		ec = LIVEROOM_ERR_AUTH_INVALID;
		break;
	case 3:
		ec = LIVEROOM_ERR_DECOMPRESS_FAILED;
		break;
	case 4:
		ec = LIVEROOM_ERR_DECODE_FAILED;
		break;
	case 5:
		ec = LIVEROOM_ERR_FIELD_MISS;
		break;
	case 6:
		ec = LIVEROOM_ERR_FIELD_INVALID;
		break;
	case 7:
		ec = LIVEROOM_ERR_AUTH_INVALID;
		break;
	default:
		ec = LIVEROOM_ERR_UNKWON_CODE;
		break;
	}

	return ec;
}

/**
* \brief：执行结果
*/
struct LRResult
{
	LRErrorCode ec;     // 错误码
	std::string msg;    // 错误描述
};

/**
* \brief：视频宽高比
*/
enum LRAspectRatio
{
	LIVEROOM_ASPECT_RATIO_4_3 = 0,
	LIVEROOM_ASPECT_RATIO_16_9 = 1,
	LIVEROOM_ASPECT_RATIO_9_16 = 2,
};

/**
* \brief：画质预设选项
*/
enum LRVideoQuality
{
	LIVEROOM_VIDEO_QUALITY_STANDARD_DEFINITION = 1,         // 标清：建议追求流畅性的客户使用该选项
	LIVEROOM_VIDEO_QUALITY_HIGH_DEFINITION = 2,             // 高清：建议对清晰度有要求的客户使用该选项
	LIVEROOM_VIDEO_QUALITY_SUPER_DEFINITION = 3,            // 超清：如果不是大屏观看，不推荐使用
};

/**
* \brief：设置美颜风格
*/
enum LRBeautyStyle
{
    LIVEROOM_BEAUTY_STYLE_SMOOTH = 0,       // 光滑
    LIVEROOM_BEAUTY_STYLE_NATURE = 1,       // 自然
    LIVEROOM_BEAUTY_STYLE_BLUR = 2,         // 朦胧
};

class ILoginLiveCallback
{
public:
	virtual ~ILoginLiveCallback() {}

	/**
	* \brief：login登录
    * \param：res - 执行结果，参考 LiveRoomUtil.h 中定义的 LRResult 结构体
    * \param：authData - RoomService提供的登录信息，包括IM相关的配置字段，在login成功后，获取到token字段
	*/
	virtual void onLogin(const LRResult& res, const LRAuthData& authData) = 0;
};

class IGetLiveRoomListCallback
{
public:
	virtual ~IGetLiveRoomListCallback() {}

	/**
	* \brief：获取房间列表
    * \param：res - 执行结果，参考 LiveRoomUtil.h 中定义的 LRResult 结构体
    * \param：rooms - 房间信息的列表
	*/
	virtual void onGetRoomList(const LRResult& res, const std::vector<LRRoomData>& rooms) = 0;
};

class ILiveRoomCallback
{
public:
	virtual ~ILiveRoomCallback() {}

	/**
	* \brief：创建房间的回调
	* \param：res - 执行结果，参考 LiveRoomUtil.h 中定义的 LRResult 结构体
	* \param：roomID - 房间ID
	*/
	virtual void onCreateRoom(const LRResult& res, const std::string& roomID) = 0;

	/**
	* \brief：进入房间的回调
	* \param：res - 执行结果，参考 LiveRoomUtil.h 中定义的 LRResult 结构体
	*/
	virtual void onEnterRoom(const LRResult& res) = 0;

	/**
	* \brief：房间信息变更的回调
	* \param：res - 执行结果，参考 LiveRoomUtil.h 中定义的 LRResult 结构体
	* \param：roomData - 房间信息，参考 LiveRoomUtil.h 中定义的 LRRoomData 结构体
	*/
	virtual void onUpdateRoomData(const LRResult& res, const LRRoomData& roomData) = 0;

    /**
    * \brief：查询观众列表的回调
    * \param：res - 执行结果，参考 LiveRoomUtil.h 中定义的 LRResult 结构体
    * \param：rooms - 观众信息的列表
    */
    virtual void onGetAudienceList(const LRResult& res, const std::vector<LRAudienceData>& audiences) = 0;

	/**
	* \brief：成员进入房间的回调
	* \param：member - 成员信息，参考 LiveRoomUtil.h 中定义的 LRMemberData 结构体
	*/
	virtual void onPusherJoin(const LRMemberData& member) = 0;

	/**
	* \brief：成员退出房间的回调
	* \param：member - 成员信息，参考 LiveRoomUtil.h 中定义的 LRMemberData 结构体
	*/
	virtual void onPusherQuit(const LRMemberData& member) = 0;

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
	* \brief：收到自定义消息
	* \param：roomID - 房间ID
	* \param userID - 发送者ID
	* \param userName - 发送者昵称
	* \param userAvatar - 发送者头像
	* \param cmd - 自定义cmd
	* \param message - 自定义消息内容
	*/
	virtual void onRecvRoomCustomMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * message) = 0;

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
	* \param：res - 执行结果，参考 LiveRoomUtil.h 中定义的 LRResult 结构体
	* \param：userID - 用户ID
	*/
	virtual void onError(const LRResult& res, const std::string& userID) = 0;

	/**
	* \brief：接收到连麦请求
	* \param：roomID - 房间ID
	* \param userID - 发送者ID
	* \param userName - 发送者昵称
	* \param userAvatar - 发送者头像
	*/
	virtual void onRecvJoinPusherRequest(const std::string& roomID, const std::string& userID, const std::string& userName, const std::string& userAvatar) = 0;

	/**
	* \brief：接收到接受连麦请求的回复
	* \param：roomID - 房间ID
	* \param msg - 消息
	*/
	virtual void onRecvAcceptJoinPusher(const std::string& roomID, const std::string& msg) = 0;

	/**
	* \brief：接收到拒绝连麦请求的回复
	* \param：roomID - 房间ID
	* \param msg - 消息
	*/
	virtual void onRecvRejectJoinPusher(const std::string& roomID, const std::string& msg) = 0;

	/**
	* \brief：接收大主播踢小主播的通知
	* \param：roomID - 房间ID
	*/
	virtual void onRecvKickoutSubPusher(const std::string& roomID) = 0;
};

#endif /* __LIVEROOMUTIL_H__ */
