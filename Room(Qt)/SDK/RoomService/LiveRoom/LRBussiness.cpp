#include "LRBussiness.h"
#include "TXLiveSDKEventDef.h"
#include "TXLiveSDKTypeDef.h"
#include "TIMManager.h"
#include "TXLiveCommon.h"
#include "log.h"
#include "Base.h"

#include <ctime>
#include <strstream>
#include <assert.h>

LRMainPublisher::LRMainPublisher()
    : m_userID(NULL)
    , m_pusher(new TXLivePusher())
    , m_player(new TXLivePlayer())
{
    assert(NULL != m_pusher);
    assert(NULL != m_player);
}

LRMainPublisher::~LRMainPublisher()
{
    if (NULL != m_userID)
    {
        delete[] m_userID;
        m_userID = NULL;
    }

    if (NULL != m_pusher)
    {
        delete m_pusher;
        m_pusher = NULL;
    }

    if (NULL != m_player)
    {
        delete m_player;
        m_player = NULL;
    }
}

char* LRMainPublisher::userID()
{
    return m_userID;
}

void LRMainPublisher::setUserID(const char* id)
{
    if (NULL != m_userID)
    {
        delete[] m_userID;
        m_userID = NULL;
    }

    if (NULL != id)
    {
        size_t size = ::strlen(id);
        m_userID = new char[size + 1];
        ::strncpy_s(m_userID, size + 1, id, size);
        m_userID[size] = '\0';
    }
}

TXLivePusher* LRMainPublisher::pusher()
{
    return m_pusher;
}

TXLivePlayer* LRMainPublisher::player()
{
    return m_player;
}

LRSubPublisher::LRSubPublisher()
    : m_userID(NULL)
    , m_player(new TXLivePlayer())
{
    assert(NULL != m_player);
}

LRSubPublisher::~LRSubPublisher()
{
    if (NULL != m_userID)
    {
        delete[] m_userID;
        m_userID = NULL;
    }

    if (NULL != m_player)
    {
        delete m_player;
        m_player = NULL;
    }
}

char* LRSubPublisher::userID()
{
    return m_userID;
}

void LRSubPublisher::setUserID(const char* id)
{
    if (NULL != m_userID)
    {
        delete[] m_userID;
        m_userID = NULL;
    }

    if (NULL != id)
    {
        size_t size = ::strlen(id);
        m_userID = new char[size + 1];
        ::strncpy_s(m_userID, size + 1, id, size);
        m_userID[size] = '\0';
    }
}

TXLivePlayer* LRSubPublisher::player()
{
    return m_player;
}

LRBussiness::LRBussiness()
	: m_authData()
	, m_httpRequest()
	, m_callback(NULL)
	, m_timerID(0)
	, m_pushUrl("")
	, m_roomList()
	, m_roomData()
    , m_subPublisher()
    , m_mainPublisher()
    , m_delayCleaner()
    , m_quit(false)
	, m_bPushBegin(false)
	, m_role(LRNullRole)
	, m_streamMixer(&m_httpRequest)
{
	TIMManager::instance()->setRecvMsgCallBack(this);
	TIMManager::instance()->setGroupChangeCallBack(this);
}

LRBussiness::~LRBussiness()
{
	leaveRoom();
	logout();

    m_delayCleaner.quit();
    m_delayCleaner.wait();
}

void LRBussiness::setCallback(ILiveRoomCallback * callback)
{
	m_callback = callback;
}

void LRBussiness::setProxy(const std::string& ip, unsigned short port)
{
    if (false == ip.empty())
    {
        TXLiveCommon::getInstance()->setProxy(ip.c_str(), port);

        m_httpRequest.setProxy(ip, port);
    }

    TIMManager::instance()->init(ip, port);
}

void LRBussiness::login(const std::string & serverDomain, const LRAuthData & authData, ILoginLiveCallback* callback)
{
	LOGGER;

	assert(false == serverDomain.empty() && NULL != callback);

	m_authData = authData;
	m_streamMixer.setSdkAppID(m_authData.sdkAppID);
    m_streamMixer.setUserID(m_authData.userID);

	TIMCommCB cb = { 0 };
	cb.data = this;
	cb.OnSuccess = onTIMLoginSuccess;
	cb.OnError = onTIMLoginError;

	IMAccountInfo accountInfo;
	accountInfo.accType = m_authData.accountType;
	accountInfo.sdkAppID = m_authData.sdkAppID;
	accountInfo.userID = m_authData.userID;
	accountInfo.userSig = m_authData.userSig;
	TIMManager::instance()->login(accountInfo, &cb);

	LINFO(L"%s", Ansi2Wide(serverDomain).c_str());

	m_httpRequest.login(serverDomain, m_authData, [=](const LRResult& res, const std::string& userID, const std::string& token) {
		assert(userID == m_authData.userID);

		m_authData.token = token;
		callback->onLogin(res, m_authData);
	});
}

void LRBussiness::recordVideo()
{
	m_bRecord = true;
}

void LRBussiness::logout()
{
	LOGGER;

    for (std::map<std::string, LRSubPublisher*>::iterator it = m_subPublisher.begin(); m_subPublisher.end() != it; ++it)
    {
        LRSubPublisher* subPublisher = it->second;
        subPublisher->player()->setCallback(NULL, NULL);
        subPublisher->player()->stopPlay();
        subPublisher->player()->closeRenderFrame();

        delete subPublisher;
    }

    m_subPublisher.clear();

    if (NULL != m_mainPublisher.userID())
    {
        m_mainPublisher.setUserID(NULL);

		m_mainPublisher.pusher()->setCallback(NULL, NULL);
        m_mainPublisher.pusher()->stopAudioCapture();
        m_mainPublisher.pusher()->stopPreview();
        m_mainPublisher.pusher()->stopPush();

		m_mainPublisher.player()->setCallback(NULL, NULL);
		m_mainPublisher.player()->stopPlay();
		m_mainPublisher.player()->closeRenderFrame();
    }

    if (false == m_quit)
    {
        m_quit = true;
        m_callback = NULL;

        TIMManager::instance()->logout();

        m_httpRequest.logout([this](const LRResult& res) {
            assert(LIVEROOM_SUCCESS == res.ec);

            m_httpRequest.close();
        });
    }
}

void LRBussiness::getRoomList(int index, int count, IGetLiveRoomListCallback* callback)
{
	LOGGER;

	m_httpRequest.getRoomList(index, count, [=](const LRResult& res, const std::vector<LRRoomData>& roomList) {
		m_roomList = roomList;
        callback->onGetRoomList(res, m_roomList);
	});
}

void LRBussiness::getAudienceList(const std::string& roomID)
{
    LOGGER;

    m_httpRequest.getAudienceList(roomID, [this](const LRResult& res, const std::vector<LRAudienceData>& audiences) {
        if (m_callback)
        {
            m_callback->onGetAudienceList(res, audiences);
        }
    });
}

void LRBussiness::createRoom(const std::string& roomID, const std::string& roomInfo)
{
	LOGGER;

	m_roomData.roomID = roomID;
	m_roomData.roomInfo = roomInfo;
	m_role = LRMainRole;

	m_httpRequest.getPushURL(m_authData.userID, [=](const LRResult& res, const std::string& pushURL) {
		if (LIVEROOM_SUCCESS != res.ec)
		{
			if (m_callback)
			{
				m_callback->onCreateRoom(res, std::string());
			}
		}
		else
		{
			m_pushUrl = pushURL;
			std::string streamID = getStreamID(m_pushUrl);
			m_streamMixer.setMainStream(streamID, 640, 360);

			std::string recordURL = m_pushUrl;
			if (m_bRecord)
			{
				recordURL.append("&record=mp4&record_interval=7200");
			}

			m_bPushBegin = false;
			m_mainPublisher.pusher()->startAudioCapture();
			m_mainPublisher.pusher()->setVideoResolution(TXE_VIDEO_RESOLUTION_640x360);
			m_mainPublisher.pusher()->startPush(recordURL.c_str());
		}
	});
}

void LRBussiness::enterRoom(const std::string& roomID, HWND rendHwnd, const RECT & rect)
{
	LOGGER;

	LINFO(L"%s", Ansi2Wide(roomID).c_str());

	// todo 播放混流地址

    m_role = LRAudience;
	m_roomData.roomID = roomID;

    // 查询得到播放地址
    for (std::vector<LRRoomData>::iterator it = m_roomList.begin(); m_roomList.end() != it; ++it)
    {
        if (it->roomID == roomID)
        {
            m_roomData = *it;

            m_mainPublisher.setUserID(m_authData.userID.c_str());
            m_mainPublisher.player()->setCallback(this, m_mainPublisher.userID());
            m_mainPublisher.player()->setRenderMode(TXE_RENDER_MODE_ADAPT);
            m_mainPublisher.player()->setRenderYMirror(false);
            m_mainPublisher.player()->setRenderFrame(rendHwnd, rect);
            m_mainPublisher.player()->startPlay(it->mixedPlayURL.c_str(), PLAY_TYPE_LIVE_RTMP);

            TIMManager::instance()->opGroup(kgJoinGroup, roomID.c_str(), this);

            Json::Value root;
            root["userName"] = m_authData.userName;
            root["userAvatar"] = m_authData.userAvatar;

            Json::FastWriter writer;
            std::string userInfo = writer.write(root);

            m_httpRequest.addAudience(roomID, m_authData.userID, userInfo, [](const LRResult& res) { });

            break;
        }
    }
}

void LRBussiness::leaveRoom()
{
	LOGGER;

	// exitRoom请求delete_Pusher CGI接口
	// 如果自己是创建房间的大主播，后台销毁这个房间，并且通知各个端房间销毁
	// 如果自己加入房间的小主播，后台不销毁房间，并且通知各个端成员变化
	// 目前destroy_room的接口可以认为已失效

    if (LRNullRole == m_role)
    {
        return;
    }

    for (std::map<std::string, LRSubPublisher*>::iterator it = m_subPublisher.begin(); m_subPublisher.end() != it; ++it)
    {
        LRSubPublisher* subPublisher = it->second;
        subPublisher->player()->setCallback(NULL, NULL);
        subPublisher->player()->stopPlay();
        subPublisher->player()->closeRenderFrame();

        delete subPublisher;
    }

    m_subPublisher.clear();

    if (NULL != m_mainPublisher.userID())
    {
        m_mainPublisher.setUserID(NULL);

        m_mainPublisher.pusher()->setCallback(NULL, NULL);
        m_mainPublisher.pusher()->stopAudioCapture();
        m_mainPublisher.pusher()->stopPreview();
        m_mainPublisher.pusher()->stopPush();

        m_mainPublisher.player()->setCallback(NULL, NULL);
        m_mainPublisher.player()->stopPlay();
        m_mainPublisher.player()->closeRenderFrame();
    }

	TIMManager::instance()->opGroup(kgQuitGroup, m_roomData.roomID.c_str(), this);

	m_httpRequest.deletePusher(m_roomData.roomID, m_authData.userID, [=](const LRResult& res) {});

	::timeKillEvent(m_timerID);

	m_streamMixer.reset();

    m_role = LRNullRole;
	m_roomData.roomID = "";
	m_roomData.members.clear(); // 清空房间m_mainPublisher信息，避免merge bug
}

void LRBussiness::sendRoomTextMsg(const char * msg)
{
    LINFO(L"msg: %s", Ansi2Wide(msg).c_str());

	Json::Value data;
	data["headpic"] = m_authData.userAvatar;
	data["nickName"] = m_authData.userName;

	Json::Value msgHead;
	msgHead["cmd"] = "CustomTextMsg";
	msgHead["data"] = data;

	Json::FastWriter writer;
	std::string jsonStr = writer.write(msgHead);

	TIMManager::instance()->setMsgHead(jsonStr.c_str());
	TIMManager::instance()->sendGroupTextMsg(m_roomData.roomID.c_str(), msg);
}

void LRBussiness::sendRoomCustomMsg(const char * cmd, const char * msg)
{
    LINFO(L"cmd: %s, msg: %s", Ansi2Wide(cmd).c_str(), Ansi2Wide(msg).c_str());

	Json::Value data;
	data["userName"] = m_authData.userName;
	data["userAvatar"] = m_authData.userAvatar;
	data["cmd"] = cmd;
	data["msg"] = msg;

	Json::Value customMessage;
	customMessage["cmd"] = "CustomCmdMsg";
	customMessage["data"] = data;

	Json::FastWriter writer;
	std::string jsonStr = writer.write(customMessage);

	TIMManager::instance()->setMsgHead(jsonStr.c_str());
	TIMManager::instance()->sendGroupCustomMsg(m_roomData.roomID.c_str(), msg);
}

void LRBussiness::sendC2CCustomMsg(const char* userID, const char * cmd, const char * msg)
{
    LINFO(L"userID: %s, cmd: %s, msg: %s", Ansi2Wide(userID).c_str(), Ansi2Wide(cmd).c_str(), Ansi2Wide(msg).c_str());

    Json::Value data;
    data["userName"] = m_authData.userName;
    data["userAvatar"] = m_authData.userAvatar;
    data["msg"] = msg;

    Json::Value customMessage;
    customMessage["cmd"] = cmd;
    customMessage["data"] = data;

    Json::FastWriter writer;
    std::string jsonStr = writer.write(customMessage);

    TIMManager::instance()->sendC2CCustomMsg(userID, jsonStr.c_str());
}

void LRBussiness::startLocalPreview(HWND rendHwnd, const RECT & rect)
{
    LINFO(L"rendHwnd: 0x%08X, rect: <%ld, %ld, %ld, %ld>", rendHwnd, rect.left, rect.top, rect.right, rect.bottom);

	m_mainPublisher.pusher()->setRenderMode(TXE_RENDER_MODE_ADAPT);

    m_mainPublisher.setUserID(m_authData.userID.c_str());
	m_mainPublisher.pusher()->setCallback(this, m_mainPublisher.userID());
	m_mainPublisher.pusher()->startPreview(rendHwnd, rect, 0);
	m_mainPublisher.pusher()->startAudioCapture();
}

void LRBussiness::updateLocalPreview(HWND rendHwnd, const RECT & rect)
{
    LINFO(L"rendHwnd: 0x%08X, rect: <%ld, %ld, %ld, %ld>", rendHwnd, rect.left, rect.top, rect.right, rect.bottom);

	m_mainPublisher.pusher()->setRenderMode(TXE_RENDER_MODE_ADAPT);
	m_mainPublisher.pusher()->updatePreview(rendHwnd, rect);
}

void LRBussiness::stopLocalPreview()
{
    LOGGER;

	// 不关闭录音
	m_mainPublisher.pusher()->stopPreview();
}

bool LRBussiness::startScreenPreview(HWND rendHwnd, HWND captureHwnd, const RECT & renderRect, const RECT & captureRect)
{
    LINFO(L"rendHwnd: 0x%08X, renderRect: <%ld, %ld, %ld, %ld>, captureHwnd: 0x%08X, captureRect: <%ld, %ld, %ld, %ld>"
        , rendHwnd, renderRect.left, renderRect.top, renderRect.right, renderRect.bottom
        , captureHwnd, captureRect.left, captureRect.top, captureRect.right, captureRect.bottom);

    m_mainPublisher.setUserID(m_authData.userID.c_str());
    m_mainPublisher.pusher()->setCallback(this, m_mainPublisher.userID());
	m_mainPublisher.pusher()->setVideoQualityParamPreset(TXE_VIDEO_QUALITY_STILLIMAGE_DEFINITION);
	m_mainPublisher.pusher()->setVideoFPS(10);
	m_mainPublisher.pusher()->setScreenCaptureParam(captureHwnd, captureRect);
	bool ret = m_mainPublisher.pusher()->startPreview(TXE_VIDEO_SRC_SDK_SCREEN, rendHwnd, renderRect);
	m_mainPublisher.pusher()->startAudioCapture();

	//sdk内部默认是镜像模式（针对摄像头），但是录屏出来的源数据本来就是镜像模式。
	m_mainPublisher.pusher()->setRenderYMirror(false);
	m_mainPublisher.pusher()->setOutputYMirror(false);

	return ret;
}

void LRBussiness::stopScreenPreview()
{
    LOGGER;

	// 不关闭录音
	m_mainPublisher.pusher()->stopPreview();
}

void LRBussiness::addRemoteView(HWND rendHwnd, const RECT & rect, const char * userID)
{
    LINFO(L"rendHwnd: 0x%08X, rect: <%ld, %ld, %ld, %ld>, userID: %s", rendHwnd, rect.left, rect.top, rect.right, rect.bottom, Ansi2Wide(userID).c_str());

    bool noLinkMic = (0 == m_subPublisher.size());

    std::map<std::string, LRSubPublisher*>::iterator it = m_subPublisher.find(userID);
    if (m_subPublisher.end() != it)
    {
        removeRemoteView(it->first.c_str());
    }

    for (int i = 0; i < m_roomData.members.size(); ++i)
    {
        if (m_roomData.members[i].userID.compare(userID) == 0)
        {
            LRSubPublisher* subPublisher = new LRSubPublisher();

            subPublisher->setUserID(userID);
            subPublisher->player()->setCallback(this, subPublisher->userID());
            subPublisher->player()->setRenderMode(TXE_RENDER_MODE_ADAPT);
            subPublisher->player()->setRenderYMirror(false);
            subPublisher->player()->setRenderFrame(rendHwnd, rect);
            subPublisher->player()->startPlay(m_roomData.members[i].accelerateURL.c_str(), PLAY_TYPE_LIVE_RTMP_ACC);

            m_subPublisher[userID] = subPublisher;

            LINFO(L"%s", Ansi2Wide(userID).c_str());

            break;
        }
    }

    if (noLinkMic && m_subPublisher.size() > 0) // 开始有连麦
    {
        switch (m_role)
        {
        case LRNullRole:
            break;
        case LRMainRole:
        {
            m_mainPublisher.pusher()->setVideoQualityParamPreset(TXE_VIDEO_QUALITY_LINKMIC_MAIN_PUBLISHER);
            m_mainPublisher.pusher()->setVideoResolution(TXE_VIDEO_RESOLUTION_640x360);
        }
        break;
        case LRSubRole:
        {
            m_mainPublisher.pusher()->setVideoQualityParamPreset(TXE_VIDEO_QUALITY_LINKMIC_SUB_PUBLISHER);
            m_mainPublisher.pusher()->setVideoResolution(TXE_VIDEO_RESOLUTION_640x360);
        }
        break;
        case LRAudience:
        break;
        default:
            break;
        }
    }
}

void LRBussiness::removeRemoteView(const char * userID)
{
    LINFO(L"%s", Ansi2Wide(userID).c_str());

    bool hasLinkMic = (m_subPublisher.size() > 0);

    std::map<std::string, LRSubPublisher*>::iterator it = m_subPublisher.find(userID);
    if (m_subPublisher.end() != it)
    {
        LRSubPublisher* subPublisher = it->second;
        subPublisher->player()->setCallback(NULL, NULL);
        subPublisher->player()->stopPlay();
        subPublisher->player()->closeRenderFrame();

        m_subPublisher.erase(it);

        // 因TXLivePlayer销毁比较耗时，放入任务队列异步销毁
        m_delayCleaner.post(true, [subPublisher] {
            delete subPublisher;
        });
    }

    if (hasLinkMic && 0 == m_subPublisher.size())  // 结束连麦, 恢复原有的画质设置
    {
        switch (m_role)
        {
        case LRNullRole:
            break;
        case LRMainRole:
        {
            m_mainPublisher.pusher()->setVideoQualityParamPreset(m_quality);
            m_mainPublisher.pusher()->setVideoResolution(TXE_VIDEO_RESOLUTION_640x360);
        }
        break;
        case LRSubRole:
        {
            m_mainPublisher.pusher()->setVideoQualityParamPreset(m_quality);
            m_mainPublisher.pusher()->setVideoResolution(TXE_VIDEO_RESOLUTION_640x360);
        }
        break;
        case LRAudience:
            break;
        default:
            break;
        }
    }
}

void LRBussiness::setMute(bool mute)
{
    LINFO(L"mute: %s", (true == mute ? L"true" : L"false"));

	m_mainPublisher.pusher()->setMute(mute);
}

void LRBussiness::setVideoQuality(LRVideoQuality quality, LRAspectRatio ratio)
{
    LINFO(L"quality: %d, ratio: %d", quality, ratio);

	switch (quality)
	{
	case LIVEROOM_VIDEO_QUALITY_STANDARD_DEFINITION:
        m_quality = TXE_VIDEO_QUALITY_STANDARD_DEFINITION;
		break;
	case LIVEROOM_VIDEO_QUALITY_HIGH_DEFINITION:
        m_quality = TXE_VIDEO_QUALITY_HIGH_DEFINITION;
		break;
	case LIVEROOM_VIDEO_QUALITY_SUPER_DEFINITION:
        m_quality = TXE_VIDEO_QUALITY_SUPER_DEFINITION;
		break;
	default:
        assert(false);
		break;
	}

    m_mainPublisher.pusher()->setVideoQualityParamPreset(m_quality);
    m_mainPublisher.pusher()->setVideoResolution(TXE_VIDEO_RESOLUTION_640x360);
}

void LRBussiness::onTIMLoginSuccess(void* data)
{
	LINFO(L"IM login success");
}

void LRBussiness::onTIMLoginError(int code, const char* desc, void* data)
{
	LINFO(L"IM login failed code: %d, desc: %s", code, desc);
}

void LRBussiness::setBeautyStyle(LRBeautyStyle beautyStyle, int beautyLevel, int whitenessLevel)
{
    LINFO(L"beautyStyle: %d, beautyLevel: %d, whitenessLevel: %d", beautyStyle, beautyLevel, whitenessLevel);

    switch (beautyStyle)
    {
    case LIVEROOM_BEAUTY_STYLE_SMOOTH:
        m_mainPublisher.pusher()->setBeautyStyle(TXE_BEAUTY_STYLE_SMOOTH, beautyLevel, whitenessLevel);
        break;
    case LIVEROOM_BEAUTY_STYLE_NATURE:
        m_mainPublisher.pusher()->setBeautyStyle(TXE_BEAUTY_STYLE_NATURE, beautyLevel, whitenessLevel);
        break;
    case LIVEROOM_BEAUTY_STYLE_BLUR:
        m_mainPublisher.pusher()->setBeautyStyle(TXE_BEAUTY_STYLE_BLUR, beautyLevel, whitenessLevel);
        break;
    default:
        break;
    }
}

void LRBussiness::requestJoinPusher()
{
	Json::Value data;
	data["type"] = "request";
	data["roomID"] = m_roomData.roomID;
	data["userID"] = m_authData.userID;
	data["userName"] = m_authData.userName;
	data["userAvatar"] = m_authData.userAvatar;

	Json::Value body;
	body["cmd"] = "linkmic";
	body["data"] = data;

	Json::FastWriter writer;
	std::string jsonStr = writer.write(body);

	TIMManager::instance()->sendC2CCustomMsg(m_roomData.roomCreator.c_str(), jsonStr.c_str());

	LINFO(L"userID: %s, json: %s", Ansi2Wide(m_roomData.roomCreator).c_str(), Ansi2Wide(jsonStr).c_str());
}

void LRBussiness::acceptJoinPusher(const std::string& userID)
{
	Json::Value data;
	data["type"] = "response";
	data["result"] = "accept";
	data["message"] = "";
	data["roomID"] = m_roomData.roomID;

	Json::Value body;
	body["cmd"] = "linkmic";
	body["data"] = data;

	Json::FastWriter writer;
	std::string jsonStr = writer.write(body);

	TIMManager::instance()->sendC2CCustomMsg(userID.c_str(), jsonStr.c_str());

	LINFO(L"userID: %s, json: %s", Ansi2Wide(userID).c_str(), Ansi2Wide(jsonStr).c_str());
}

void LRBussiness::rejectJoinPusher(const std::string& userID, const std::string& reason)
{
	Json::Value data;
	data["type"] = "response";
	data["result"] = "reject";
	data["message"] = reason;
	data["roomID"] = m_roomData.roomID;

	Json::Value body;
	body["cmd"] = "linkmic";
	body["data"] = data;

	Json::FastWriter writer;
	std::string jsonStr = writer.write(body);

	TIMManager::instance()->sendC2CCustomMsg(userID.c_str(), jsonStr.c_str());

	LINFO(L"userID: %s, json: %s", Ansi2Wide(userID).c_str(), Ansi2Wide(jsonStr).c_str());
}

void LRBussiness::kickoutSubPusher(const std::string& userID)
{
	Json::Value data;
	data["type"] = "kickout";
	data["roomID"] = m_roomData.roomID;

	Json::Value body;
	body["cmd"] = "linkmic";
	body["data"] = data;

	Json::FastWriter writer;
	std::string jsonStr = writer.write(body);

	TIMManager::instance()->sendC2CCustomMsg(userID.c_str(), jsonStr.c_str());

	LINFO(L"userID: %s, json: %s", Ansi2Wide(userID).c_str(), Ansi2Wide(jsonStr).c_str());
}

void LRBussiness::joinPusher()
{
	// todo 小主播请求连麦暂缓
}

void LRBussiness::quitPusher()
{
	// todo 小主播退出连麦暂缓
}

int LRBussiness::enumCameras(wchar_t ** camerasName, size_t capacity)
{
	return m_mainPublisher.pusher()->enumCameras(camerasName, capacity);
}

void LRBussiness::switchCamera(int cameraIndex)
{
	return m_mainPublisher.pusher()->switchCamera(cameraIndex);
}

int LRBussiness::micDeviceCount()
{
	return m_mainPublisher.pusher()->micDeviceCount();
}

bool LRBussiness::micDeviceName(unsigned int index, char name[MIC_DEVICE_NAME_MAX_SIZE])
{
	return m_mainPublisher.pusher()->micDeviceName(index, name);
}

void LRBussiness::selectMicDevice(unsigned int index)
{
	m_mainPublisher.pusher()->selectMicDevice(index);
}

unsigned int LRBussiness::micVolume()
{
	return m_mainPublisher.pusher()->micVolume();
}

void LRBussiness::setMicVolume(unsigned int volume)
{
	m_mainPublisher.pusher()->setMicVolume(volume);
}

// 群操作成功回调---创建、加入、删除、退出
void LRBussiness::onGroupOptSuccess(IMGroupOptType opType, const char* group_id)
{
	LINFO(L"opType: %d, group_id: %s", opType, Ansi2Wide(group_id).c_str());

	if (opType == kgCreateGroup)
	{
		// 创群成功
	}
	if (opType == kgJoinGroup)
	{

	}

	if (opType == kgQuitGroup)
	{
		// 退群成功
	}

	if (opType == kgDeleteGroup)
	{

	}
}

// 群操作失败回调---创建、加入、删除、退出
void LRBussiness::onGroupOptError(IMGroupOptType opType, const char* group_id, int code, const char* desc)
{
	LINFO(L"opType: %d, group_id: %s, code: %d, desc: %s", opType, group_id, code, desc);

	if (opType == kgCreateGroup)
	{
		// 创群失败了，要加入群
		TIMManager::instance()->opGroup(kgJoinGroup, group_id, this);
	}

	if (opType == kgJoinGroup)
	{
		stopLocalPreview();
	}

	if (opType == kgQuitGroup)
	{
		// 退群
	}

	if (opType == kgDeleteGroup)
	{

	}
}

void LRBussiness::onRecvC2CTextMsg(const char * userID, const char * msg)
{

}

void LRBussiness::onRecvGroupTextMsg(const char * groupID, const char * userID, const char * msg, const char * msgHead)
{
	LINFO(L"groupID: %s, userID: %s, msg: %s, msgHead: %s"
		, Ansi2Wide(groupID).c_str(), Ansi2Wide(userID).c_str(), Ansi2Wide(msg).c_str(), Ansi2Wide(msgHead).c_str());

	if (m_roomData.roomID.compare(groupID) != 0)
	{
		return;
	}

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(msgHead, root))
	{
		return;
	}

	std::string userAvatar;
	std::string userName;
	if (root.isMember("data"))
	{
		Json::Value data = root["data"];
		if (data.isMember("headpic"))
		{
			userAvatar = data["headpic"].asString();
		}
		if (data.isMember("nickName"))
		{
			userName = data["nickName"].asString();
		}
	}

	if (m_callback)
	{
		m_callback->onRecvRoomTextMsg(groupID, userID, userName.c_str(), userAvatar.c_str(), msg);
	}
}

void LRBussiness::onRecvC2CCustomMsg(const char * userID, const char * msg)
{
	LINFO(L"userID: %s, msg: %s", Ansi2Wide(userID).c_str(), Ansi2Wide(msg).c_str());

	if (!userID || !msg)
	{
		return;
	}

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(msg, root))
	{
		return;
	}

    std::string cmd;
    if (root.isMember("cmd"))
    {
        cmd = root["cmd"].asString();
    }

	if (cmd == "linkmic")
	{
		if (root.isMember("data"))
		{
			handleLinkMic(root["data"]);
		}
	}
    else if (cmd == "sketchpad")
    {
        handleSketchPad(userID, root);
    }
}

void LRBussiness::onRecvGroupCustomMsg(const char * groupID, const char * userID, const char * msgContent)
{
	LINFO(L"groupID: %s, userID: %s, msgContent: %s"
		, Ansi2Wide(groupID).c_str(), Ansi2Wide(userID).c_str(), Ansi2Wide(msgContent).c_str());

	if (m_roomData.roomID.compare(groupID) != 0)
	{
		return;
	}

	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(msgContent, root))
	{
		return;
	}

	std::string cmd;
	if (root.isMember("cmd"))
	{
		cmd = root["cmd"].asString();
	}

	if (LRNullRole != m_role && cmd == "notifyPusherChange")
	{
		getPushers();
	}
	else if (cmd == "CustomCmdMsg" && root["data"].isObject() && m_callback)
	{
		Json::Value data;
		if (root.isMember("data"))
		{
			data = root["data"];
		}

		std::string userName;
		if (data.isMember("userName"))
		{
			userName = data["userName"].asString();
		}

		std::string userAvatar;
		if (data.isMember("userAvatar"))
		{
			userAvatar = data["userAvatar"].asString();
		}

		std::string dataCmd;
		if (data.isMember("cmd"))
		{
			dataCmd = data["cmd"].asString();
		}

		std::string msg;
		if (data.isMember("msg"))
		{
			msg = data["msg"].asString();
		}

		m_callback->onRecvRoomCustomMsg(groupID, userID, userName.c_str(), userAvatar.c_str(), dataCmd.c_str(), msg.c_str());
	}
}

void LRBussiness::onRecvGroupSystemMsg(const char * groupID, const char * msg)
{

}

void LRBussiness::onGroupChangeMessage(IMGroupOptType opType, const char* group_id, const char * user_id)
{
	LINFO(L"opType: %d, group_id: %s, user_id: %s", opType, group_id, user_id);

	if (opType == kgDeleteGroup)
	{
		if (m_roomData.roomID.compare(group_id) == 0 && LRMainRole != m_role)
		{
			// 房间解散后后台自动退房间
			::timeKillEvent(m_timerID);
			if (m_callback)
			{
				m_callback->onRoomClosed(m_roomData.roomID.c_str());
			}
		}
	}
}

void LRBussiness::onEventCallback(int eventId, const int paramCount, const char** paramKeys, const char** paramValues,
	void* pUserData)
{
	if (eventId < 10000)
	{
		for (int i = 0; i < paramCount; ++i)
		{
			LINFO(L"eventId: %d, <%s, %s>", eventId, Ansi2Wide(paramKeys[i]).c_str(), Ansi2Wide(paramValues[i]).c_str());
		}
	}

	char* userID = reinterpret_cast<char*>(pUserData);

	switch (eventId)
	{
    case PushEvt::PUSH_WARNING_RECONNECT:
    {
        LINFO(L"Pusher reconnect, userID: %s", Ansi2Wide(userID).c_str());
    }
    break;
	case PushEvt::PUSH_ERR_NET_DISCONNECT:
	{
		handlePushDisconnect(userID);
	}
	break;
	case PushEvt::PUSH_EVT_PUSH_BEGIN:
	{
		handlePushBegin();
	}
	break;
	case PushEvt::PUSH_ERR_CAMERA_OCCUPY:
	{
		LINFO(L"PUSH_ERR_CAMERA_OCCUPY");

		if (m_callback)
		{
			m_callback->onError({ LIVEROOM_ERR_CAMERA_OCCUPY, "摄像头占用" }, m_authData.userID);
		}
	}
	break;
	case PushEvt::PUSH_EVT_CAMERA_REMOVED:
	{
		LINFO(L"PUSH_EVT_CAMERA_REMOVED");

		if (m_callback)
		{
			m_callback->onError({ LIVEROOM_ERR_CAMERA_REMOVED, "摄像头被拔出" }, m_authData.userID);
		}
	}
	break;
    case PlayEvt::PLAY_WARNING_RECONNECT:
    {
        LINFO(L"Player reconnect, userID: %s", Ansi2Wide(userID).c_str());
    }
    break;
	case  PlayEvt::PLAY_ERR_NET_DISCONNECT:
	{
        handlePlayDisconnect(userID);
	}
    break;
    case PlayEvt::PLAY_EVT_CHANGE_RESOLUTION:
    {
        int width = 0;
        int height = 0;
        for (int i = 0; i < paramCount; ++i)
        {
            if (std::string(EVT_PARAM1) == paramKeys[i])
            {
                width = ::atoi(paramValues[i]);
            }

            if (std::string(EVT_PARAM2) == paramKeys[i])
            {
                height = ::atoi(paramValues[i]);
            }
        }

        for (std::vector<LRMemberData>::iterator it = m_roomData.members.begin(); m_roomData.members.end() != it; ++it)
        {
            if (it->userID == userID)
            {
                std::string streamID = getStreamID(it->accelerateURL);
                if (false == streamID.empty())
                {
                    m_streamMixer.addSubStream(streamID, width, height);
                }

                break;
            }
        }
    }
	break;
    default:
        break;
	}
}

void CALLBACK LRBussiness::onTimerEvent(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	LRBussiness* impl = reinterpret_cast<LRBussiness*>(dwUser);
	if (impl)
	{
		impl->m_httpRequest.heartbeat(impl->m_roomData.roomID, impl->m_authData.userID, [=](const LRResult& res) {});
	}
}

void LRBussiness::getPushers()
{
	m_httpRequest.getPushers(m_roomData.roomID, [=](const LRResult& res, const LRRoomData& roomData) {
		if (LIVEROOM_SUCCESS == res.ec)
		{
			std::vector<LRMemberData> oldMembers = m_roomData.members;
			m_roomData = roomData;

			mergePushers(oldMembers);
		}

		if (m_callback)
		{
			m_callback->onUpdateRoomData(res, m_roomData);
		}
	});
}

void LRBussiness::mergePushers(const std::vector<LRMemberData>& tempPushers)
{
	for (int i = 0; i < tempPushers.size(); i++)
	{
		bool flag = false;
		for (int j = 0; j < m_roomData.members.size(); j++)
		{
			if (m_roomData.members[j].userID == tempPushers[i].userID)
			{
				flag = true;
				break;
			}
		}
		if (!flag && tempPushers[i].userID != m_authData.userID && m_callback)
		{
			m_callback->onPusherQuit(tempPushers[i]);
			LINFO(L"%s", Ansi2Wide(tempPushers[i].userID).c_str());

			std::string streamID = getStreamID(tempPushers[i].accelerateURL);
			if (false == streamID.empty())
			{
				m_streamMixer.removeSubStream(streamID);
			}
		}
	}

	for (int i = 0; i < m_roomData.members.size(); i++)
	{
		bool flag = false;
		for (int j = 0; j < tempPushers.size(); j++)
		{
			if (m_roomData.members[i].userID == tempPushers[j].userID)
			{
				flag = true;
				break;
			}
		}
		if (!flag&& m_roomData.members[i].userID != m_authData.userID && m_callback)
		{
			m_callback->onPusherJoin(m_roomData.members[i]);
			LINFO(L"%s", Ansi2Wide(m_roomData.members[i].userID).c_str());
		}
	}
}

void LRBussiness::handleLinkMic(const Json::Value& data)
{
	std::string type;
	if (data.isMember("type"))
	{
		type = data["type"].asString();
	}

	if (type == "request")
	{
		if (!data.isMember("roomID") || !data.isMember("userID") || !data.isMember("userName") || !data.isMember("userAvatar"))
		{
			return;
		}

		std::string roomID = data["roomID"].asString();
		std::string userID = data["userID"].asString();
		std::string userName = data["userName"].asString();
		std::string userAvatar = data["userAvatar"].asString();
		if (m_callback)
		{
			m_callback->onRecvJoinPusherRequest(roomID, userID, userName, userAvatar);
		}
	}
	else if (type == "response")
	{
		if (!data.isMember("result") || !data.isMember("message") || !data.isMember("roomID"))
		{
			return;
		}

		std::string result = data["result"].asString();
		std::string message = data["message"].asString();
		std::string roomID = data["roomID"].asString();
		if (result == "accept")
		{
			if (m_callback)
			{
				m_callback->onRecvAcceptJoinPusher(roomID, message);
			}
		}
		else if (result == "reject")
		{
			if (m_callback)
			{
				m_callback->onRecvRejectJoinPusher(roomID, message);
			}
		}
	}
	else if (type == "kickout")
	{
		if (!data.isMember("roomID"))
		{
			return;
		}

		std::string roomID = data["roomID"].asString();
		if (m_callback)
		{
			m_callback->onRecvKickoutSubPusher(roomID);
		}
	}
}

void LRBussiness::handleSketchPad(const std::string& userID, const Json::Value& root)
{
    std::string cmd;
    if (root.isMember("cmd"))
    {
        cmd = root["cmd"].asString();
    }

    Json::Value data;
    if (root.isMember("data"))
    {
        data = root["data"];
    }

    std::string msg;
    if (data.isMember("msg"))
    {
        msg = data["msg"].asString();
    }

    std::string userName;
    if (data.isMember("userName"))
    {
        userName = data["userName"].asString();
    }

    std::string userAvatar;
    if (data.isMember("userAvatar"))
    {
        userAvatar = data["userAvatar"].asString();
    }

    if (m_callback)
    {
        m_callback->onRecvC2CCustomMsg(userID.c_str(), userName.c_str(), userAvatar.c_str(), cmd.c_str(), msg.c_str());
    }
}

void LRBussiness::handlePushBegin()
{
    LINFO(L"m_bPushBegin", (true == m_bPushBegin ? L"true" : L"false"));;

    if (m_bPushBegin)
    {
        return;
    }

    m_bPushBegin = true;

	switch (m_role)
	{
	case LRMainRole:
	{
		m_streamMixer.mergeStream(10);
		m_httpRequest.createRoom(m_roomData.roomID, m_roomData.roomInfo, [=](const LRResult& res, const std::string& roomID) {
			if (LIVEROOM_SUCCESS != res.ec)
			{
				if (m_callback)
				{
					m_callback->onCreateRoom(res, roomID);
				}
			}
			else
			{
				m_roomData.roomID = roomID;

				m_streamMixer.setRoomID(roomID);

				TIMManager::instance()->opGroup(kgJoinGroup, roomID.c_str(), this);

				m_timerID = ::timeSetEvent(5000, 1, onTimerEvent, (DWORD_PTR)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION); // 开启心跳定时器

				m_httpRequest.addPusher(m_roomData.roomID, m_authData.userID, m_authData.userName, m_authData.userAvatar, m_pushUrl, [=](const LRResult& res) {
					if (m_callback)
					{
						m_callback->onCreateRoom(res, roomID);
					}

					if (LIVEROOM_SUCCESS == res.ec)
					{
						getPushers();
					}
				});
			}
		});
	}
	break;
	case LRSubRole:
		// todo
		break;
	case LRAudience:
		// todo
		break;
	default:
		break;
	}
}

void LRBussiness::handlePushDisconnect(const std::string& userID)
{
	switch (m_role)
	{
	case LRMainRole:
	{
		LINFO(L"PUSH_ERR_NET_DISCONNECT");

        m_mainPublisher.pusher()->setCallback(NULL, NULL);
		m_mainPublisher.pusher()->stopAudioCapture();
		m_mainPublisher.pusher()->stopPreview();
		m_mainPublisher.pusher()->stopPush();

		if (m_bPushBegin) //已经开始推流，中途失败
		{
			m_callback->onError({ LIVEROOM_ERR_PUSH_DISCONNECT, "推流断开，请检查网络设置" }, m_authData.userID);
		}
		else
		{
			m_callback->onCreateRoom({ LIVEROOM_ERR_PUSH_DISCONNECT, "推流断开，请检查网络设置" }, m_roomData.roomID);
		}

		leaveRoom();
	}
	break;
	case LRSubRole:
	{
		// todo 连麦失败
	}
	break;
	case LRAudience:
		break;
	default:
		break;
	}
}

void LRBussiness::handlePlayBegin()
{
	// todo
}

void LRBussiness::handlePlayDisconnect(const std::string& userID)
{
    LINFO(L"Player disconnect, userID: %s", Ansi2Wide(userID).c_str());

    if (m_callback)
    {
        m_callback->onError({ LIVEROOM_ERR_PLAY_DISCONNECT, "拉流连接断开" }, userID);
    }

    for (std::vector<LRMemberData>::iterator it = m_roomData.members.begin(); m_roomData.members.end() != it; ++it)
    {
        if (userID == it->userID)
        {
            m_callback->onPusherQuit(*it);

            m_roomData.members.erase(it);

            break;
        }
    }
}

std::string LRBussiness::getStreamID(const std::string& streamURL)
{
	if (streamURL.empty())
	{
		return std::string("");
	}

	//推流地址格式：rtmp://8888.livepush.myqcloud.com/path/8888_test_12345_test?txSecret=aaaa&txTime=bbbb
	//拉流地址格式：rtmp://8888.liveplay.myqcloud.com/path/8888_test_12345_test
	//            http://8888.liveplay.myqcloud.com/path/8888_test_12345_test.flv
	//            http://8888.liveplay.myqcloud.com/path/8888_test_12345_test.m3u8


	std::string subString = streamURL;

	{
		//1 截取第一个 ？之前的子串
		size_t index = subString.find("?");
		if (index != std::string::npos) {
			subString = subString.substr(0, index);
		}

		if (subString.empty())
		{
			return std::string("");
		}
	}

	{
		//2 截取最后一个 / 之后的子串
		size_t index = subString.rfind("/");
		if (index != std::string::npos) {
			subString = subString.substr(index + 1);
		}

		if (subString.empty())
		{
			return std::string("");
		}
	}

	{
		//3 截取第一个 . 之前的子串
		size_t index = subString.find(".");
		if (index != -1) {
			subString = subString.substr(0, index);
		}

		if (subString.empty())
		{
			return std::string("");
		}
	}

	return subString;
}
