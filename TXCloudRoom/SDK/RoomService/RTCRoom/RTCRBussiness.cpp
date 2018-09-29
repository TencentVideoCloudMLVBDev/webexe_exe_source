#include "RTCRBussiness.h"
#include "TXLiveSDKEventDef.h"
#include "TXLiveSDKTypeDef.h"
#include "TIMManager.h"
#include "log.h"
#include "Base.h"
#include "TXLiveCommon.h"
#include "DataReport.h"

#include <ctime>
#include <strstream>
#include <assert.h>
#include "HttpReportRequest.h"

RTCMainPublisher::RTCMainPublisher()
    : m_userID(NULL)
    , m_pusher(new TXLivePusher())
{
    assert(NULL != m_pusher);
}

RTCMainPublisher::~RTCMainPublisher()
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
}

char* RTCMainPublisher::userID()
{
    return m_userID;
}

void RTCMainPublisher::setUserID(const char* id)
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

TXLivePusher* RTCMainPublisher::pusher()
{
    return m_pusher;
}

RTCSubPublisher::RTCSubPublisher()
    : m_userID(NULL)
    , m_player(new TXLivePlayer())
{
    assert(NULL != m_player);
}

RTCSubPublisher::~RTCSubPublisher()
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

char* RTCSubPublisher::userID()
{
    return m_userID;
}

void RTCSubPublisher::setUserID(const char* id)
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

TXLivePlayer* RTCSubPublisher::player()
{
    return m_player;
}

RTCRBussiness::RTCRBussiness()
    : m_authData()
    , m_httpRequest()
    , m_callback(NULL)
    , m_timerID(0)
    , m_roomList()
    , m_roomData()
    , m_pushUrl("")
    , m_delayCleaner()
    , m_bPushBegin(false)
    , m_bCreateRoom(false)
    , m_bInRoom(false)
	, m_streamMixer(&m_httpRequest)
{

}

RTCRBussiness::~RTCRBussiness()
{
    leaveRoom();
    logout();

    m_delayCleaner.quit();
    m_delayCleaner.wait();
}

void RTCRBussiness::setCallback(IRTCRoomCallback * callback)
{
    m_callback = callback;
}

void RTCRBussiness::setProxy(const std::string& ip, unsigned short port)
{
    if (false == ip.empty())
    {
        TXLiveCommon::getInstance()->setProxy(ip.c_str(), port);

        m_httpRequest.setProxy(ip, port);
    }

    TIMManager::instance()->init(ip, port);
}

void RTCRBussiness::login(const std::string & serverDomain, const RTCAuthData & authData, ILoginRTCCallback* callback)
{
    LOGGER;

    assert(false == serverDomain.empty() && NULL != callback);

    TIMManager::instance()->setRecvMsgCallBack(this);
    TIMManager::instance()->setGroupChangeCallBack(this);
    TIMManager::instance()->setKickOfflineCallBack(this);

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

	m_serverDomain = serverDomain;
	m_loginCallback = callback;

    TIMManager::instance()->login(accountInfo, &cb);

    LINFO(L"%s", Ansi2Wide(serverDomain).c_str());
}

void RTCRBussiness::recordVideo(bool multi, int picture_id)
{
	m_bRecord = true;

	m_streamMixer.setMixType(multi);
	m_streamMixer.setPictureID(picture_id);
}

void RTCRBussiness::logout()
{
    LOGGER;

    for (std::map<std::string, RTCSubPublisher*>::iterator it = m_subPublisher.begin(); m_subPublisher.end() != it; ++it)
    {
        RTCSubPublisher* subPublisher = it->second;

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
    }

	if (m_callback)
	{
		TIMManager::instance()->logout();

		m_httpRequest.logout([this](const RTCResult& res) {
			assert(RTCROOM_SUCCESS == res.ec);

            m_httpRequest.close();
		});

		m_callback = NULL;
	}
}

void RTCRBussiness::getRoomList(int index, int cnt, IGetRTCRoomListCallback* callback)
{
    LOGGER;

    m_httpRequest.getRoomList(index, cnt, [=](const RTCResult& res, const std::vector<RTCRoomData>& roomList) {
        m_roomList = roomList;
        callback->onGetRoomList(res, m_roomList);
    });
}

void RTCRBussiness::createRoom(const std::string& roomID, const std::string& roomInfo)
{
    LOGGER;
	
    m_roomData.roomID = roomID; //用户指定roomid，若为空，则后台自动生成
    m_roomData.roomInfo = roomInfo;
    m_bCreateRoom = true;

    m_httpRequest.getPushURL(m_authData.userID, [=](const RTCResult& res, const std::string& pushURL) {
		DataReport::instance().setCGIPushURL(DataReport::instance().txf_gettickcount());

		if (RTCROOM_SUCCESS != res.ec)
		{
			if (m_callback)
			{
				m_callback->onCreateRoom(res, std::string());
			}
			m_bReport = true;
			DataReport::instance().setResult(DataReportEnter, "fail:1002", res.msg);
			HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
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
			m_mainPublisher.pusher()->startPush(recordURL.c_str());
		}
	});
}

void RTCRBussiness::enterRoom(const std::string& roomID)
{
	LOGGER;

	m_roomData.roomID = roomID;
	m_bInRoom = true;

	m_httpRequest.getPushURL(m_authData.userID, [=](const RTCResult& res, const std::string& pushURL) {
		if (RTCROOM_SUCCESS != res.ec)
		{
			if (m_callback)
			{
				m_callback->onEnterRoom(res);
			}
		}
		else
		{
			m_pushUrl = pushURL;

			m_bPushBegin = false;
			m_mainPublisher.pusher()->startAudioCapture();
			m_mainPublisher.pusher()->startPush(m_pushUrl.c_str());
		}
	});
}

void RTCRBussiness::leaveRoom()
{
	LOGGER;
	if (!m_bReport)
	{
		m_bReport = true;
		HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
	}
	
	if (!m_bCreateRoom && !m_bInRoom)
	{
		return;
	}

    for (std::map<std::string, RTCSubPublisher*>::iterator it = m_subPublisher.begin(); m_subPublisher.end() != it; ++it)
    {
        RTCSubPublisher* subPublisher = it->second;

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
    }

	m_httpRequest.deletePusher(m_roomData.roomID, m_authData.userID, [=](const RTCResult& res) {
        TIMManager::instance()->opGroup(kgQuitGroup, m_roomData.roomID.c_str(), this);
    });

	::timeKillEvent(m_timerID);
    m_timerID = 0;

	m_bInRoom = false;
	m_bCreateRoom = false;
	m_roomData.roomID = "";
	m_roomData.members.clear(); // 清空房间m_pusher信息，避免merge bug

	m_streamMixer.reset();
}

void RTCRBussiness::sendRoomTextMsg(const char * msg)
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

void RTCRBussiness::sendRoomCustomMsg(const char * cmd, const char * msg)
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

void RTCRBussiness::sendC2CCustomMsg(const char* userID, const char * cmd, const char * msg)
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

void RTCRBussiness::startLocalPreview(HWND rendHwnd, const RECT & rect)
{
    LINFO(L"rendHwnd: 0x%08X, rect: <%ld, %ld, %ld, %ld>", rendHwnd, rect.left, rect.top, rect.right, rect.bottom);

	if (m_cameraCount == 0)
	{
		m_cameraCount = m_mainPublisher.pusher()->enumCameras(); //重新检查摄像头
		if (m_cameraCount <= 0 && m_callback)
		{
			m_callback->onError({ RTCROOM_ERR_CAMERA_MISSED, "未检测到摄像头，无法正常开课，请先接入摄像头。" }, m_authData.userID);
		}
	}

    m_mainPublisher.pusher()->setRenderMode(TXE_RENDER_MODE_ADAPT);
	if (m_bRecord)
		m_mainPublisher.pusher()->setVideoQualityParamPreset(m_quality, TXE_VIDEO_RATIO_16_9);
	else
		m_mainPublisher.pusher()->setVideoQualityParamPreset(m_quality, TXE_VIDEO_RATIO_4_3);

    m_mainPublisher.setUserID(m_authData.userID.c_str());
    m_mainPublisher.pusher()->setCallback(this, m_mainPublisher.userID());
    m_mainPublisher.pusher()->startPreview(rendHwnd, rect, 0);
}

void RTCRBussiness::updateLocalPreview(HWND rendHwnd, const RECT & rect)
{
    LINFO(L"rendHwnd: 0x%08X, rect: <%ld, %ld, %ld, %ld>", rendHwnd, rect.left, rect.top, rect.right, rect.bottom);

    m_mainPublisher.pusher()->setRenderMode(TXE_RENDER_MODE_ADAPT);
    m_mainPublisher.pusher()->updatePreview(rendHwnd, rect);
}

void RTCRBussiness::stopLocalPreview()
{
    LOGGER;

    m_mainPublisher.pusher()->stopPreview();
}

bool RTCRBussiness::startScreenPreview(HWND rendHwnd, HWND captureHwnd, const RECT & renderRect, const RECT & captureRect, bool bFollowWndRect)
{
    LINFO(L"rendHwnd: 0x%08X, renderRect: <%ld, %ld, %ld, %ld>, captureHwnd: 0x%08X, captureRect: <%ld, %ld, %ld, %ld>"
        , rendHwnd, renderRect.left, renderRect.top, renderRect.right, renderRect.bottom
        , captureHwnd, captureRect.left, captureRect.top, captureRect.right, captureRect.bottom);

    m_mainPublisher.pusher()->setVideoQualityParamPreset(TXE_VIDEO_QUALITY_STILLIMAGE_DEFINITION);     // todo 目前屏幕分享的画质，不适合实时音视频通话
    m_mainPublisher.pusher()->setVideoFPS(10);

    m_mainPublisher.pusher()->setScreenCaptureParam(captureHwnd, captureRect, bFollowWndRect);
    bool ret = m_mainPublisher.pusher()->startPreview(TXE_VIDEO_SRC_SDK_SCREEN, rendHwnd, renderRect);

    //sdk内部默认是镜像模式（针对摄像头），但是录屏出来的源数据本来就是镜像模式。
    m_mainPublisher.pusher()->setRenderYMirror(false);
    m_mainPublisher.pusher()->setOutputYMirror(false);
	m_mainPublisher.pusher()->openSystemVoiceInput();
    return ret;
}

void RTCRBussiness::stopScreenPreview()
{
    LOGGER;

	m_mainPublisher.pusher()->closeSystemVoiceInput();
    m_mainPublisher.pusher()->stopPreview();
}

void RTCRBussiness::addRemoteView(HWND rendHwnd, const RECT & rect, const char * userID)
{
    LINFO(L"rendHwnd: 0x%08X, rect: <%ld, %ld, %ld, %ld>, userID: %s", rendHwnd, rect.left, rect.top, rect.right, rect.bottom, Ansi2Wide(userID).c_str());
	bool noLinkMic = (0 == m_subPublisher.size());

    std::map<std::string, RTCSubPublisher*>::iterator it = m_subPublisher.find(userID);
    if (m_subPublisher.end() != it)
    {
        removeRemoteView(userID);
    }

    for (int i = 0; i < m_roomData.members.size(); ++i)
    {
        if (m_roomData.members[i].userID.compare(userID) == 0)
        {
            RTCSubPublisher* subPublisher = new RTCSubPublisher();
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

	if (noLinkMic && m_subPublisher.size() > 0 && m_bRecord) // 开始混流
	{
		if (m_bCreateRoom)
			m_mainPublisher.pusher()->setVideoQualityParamPreset(TXE_VIDEO_QUALITY_LINKMIC_MAIN_PUBLISHER, TXE_VIDEO_RATIO_16_9);
		else
			m_mainPublisher.pusher()->setVideoQualityParamPreset(TXE_VIDEO_QUALITY_LINKMIC_SUB_PUBLISHER, TXE_VIDEO_RATIO_16_9);
	}
}

void RTCRBussiness::removeRemoteView(const char * userID)
{
    LINFO(L"%s", Ansi2Wide(userID).c_str());
	bool hasLinkMic = (m_subPublisher.size() > 0);

    std::map<std::string, RTCSubPublisher*>::iterator it = m_subPublisher.find(userID);
    if (m_subPublisher.end() != it)
    {
        RTCSubPublisher* subPublisher = it->second;
        subPublisher->player()->setCallback(NULL, NULL);
        subPublisher->player()->stopPlay();
        subPublisher->player()->closeRenderFrame();

        m_subPublisher.erase(it);

        // 因TXLivePlayer销毁比较耗时，放入任务队列异步销毁
        m_delayCleaner.post(true, [subPublisher] {
            delete subPublisher;
        });
    }

	if (hasLinkMic && 0 == m_subPublisher.size() && m_bRecord)  // 结束混流, 恢复原有的画质设置
	{
		m_mainPublisher.pusher()->setVideoQualityParamPreset(m_quality, TXE_VIDEO_RATIO_16_9);
	}
}

void RTCRBussiness::updateRemotePreview(HWND rendHwnd, const RECT & rect, const char * userID)
{
    LINFO(L"rendHwnd: 0x%08X, rect: <%ld, %ld, %ld, %ld>, userID: %s", rendHwnd, rect.left, rect.top, rect.right, rect.bottom, Ansi2Wide(userID).c_str());

	std::map<std::string, RTCSubPublisher*>::iterator it = m_subPublisher.find(userID);
	if (m_subPublisher.end() != it)
	{
        RTCSubPublisher* subPublisher = it->second;
        subPublisher->player()->setRenderMode(TXE_RENDER_MODE_ADAPT);
        subPublisher->player()->updateRenderFrame(rendHwnd, rect);

		LINFO(L"%s", Ansi2Wide(userID).c_str());
	}
}

void RTCRBussiness::setMute(bool mute)
{
    m_mainPublisher.pusher()->setMute(mute);
}

void RTCRBussiness::setVideoQuality(RTCVideoQuality quality, RTCAspectRatio ratio)
{
    LINFO(L"quality: %d, ratio: %d", quality, ratio);

	switch (quality)
	{
	case RTCROOM_VIDEO_QUALITY_STANDARD_DEFINITION:
		m_quality = TXE_VIDEO_QUALITY_STANDARD_DEFINITION;
		break;
	case RTCROOM_VIDEO_QUALITY_HIGH_DEFINITION:
		m_quality = TXE_VIDEO_QUALITY_HIGH_DEFINITION;
		break;
	case RTCROOM_VIDEO_QUALITY_SUPER_DEFINITION:
		m_quality = TXE_VIDEO_QUALITY_SUPER_DEFINITION;
		break;
	default:
		assert(false);
		break;
	}

	if (m_bRecord)
	{
		m_mainPublisher.pusher()->setVideoQualityParamPreset(m_quality, TXE_VIDEO_RATIO_16_9);
	}
	else
		m_mainPublisher.pusher()->setVideoQualityParamPreset(m_quality, TXE_VIDEO_RATIO_4_3);
}

void RTCRBussiness::onTIMLoginSuccess(void* data)
{
    LINFO(L"IM login success");
	DataReport::instance().setIMLogin(DataReport::instance().txf_gettickcount());

	RTCRBussiness* pImpl = reinterpret_cast<RTCRBussiness*>(data);
	if (NULL != pImpl)
	{
		pImpl->m_httpRequest.login(pImpl->m_serverDomain, pImpl->m_authData, [=](const RTCResult& res, const std::string& userID, const std::string& token) {
			assert(userID == pImpl->m_authData.userID);
			pImpl->m_authData.token = token;
			pImpl->m_loginCallback->onLogin(res, pImpl->m_authData);
			if (res.ec != RTCROOM_SUCCESS)
			{
				pImpl->m_bReport = true;
				DataReport::instance().setResult(DataReportEnter, "fail:1002", res.msg);
				HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
			}

			DataReport::instance().setCGILogin(DataReport::instance().txf_gettickcount());
		});
	}
}

void RTCRBussiness::onTIMLoginError(int code, const char* desc, void* data)
{
	DataReport::instance().setIMLogin(1);
	DataReport::instance().setResult(DataReportEnter, "fail:1001", desc);
    LINFO(L"IM login failed code: %d, desc: %s", code, desc);

	RTCRBussiness* pImpl = reinterpret_cast<RTCRBussiness*>(data);
	if (NULL != pImpl)
	{
		RTCResult res = { adaptRTCErrorCode(code), desc };
		pImpl->m_loginCallback->onLogin(res, pImpl->m_authData);
	}
}

void RTCRBussiness::setBeautyStyle(RTCBeautyStyle beautyStyle, int beautyLevel, int whitenessLevel)
{
    LINFO(L"beautyStyle: %d, beautyLevel: %d, whitenessLevel: %d", beautyStyle, beautyLevel, whitenessLevel);

    switch (beautyStyle)
    {
    case RTCROOM_BEAUTY_STYLE_SMOOTH:
        m_mainPublisher.pusher()->setBeautyStyle(TXE_BEAUTY_STYLE_SMOOTH, beautyLevel, whitenessLevel);
        break;
    case RTCROOM_BEAUTY_STYLE_NATURE:
        m_mainPublisher.pusher()->setBeautyStyle(TXE_BEAUTY_STYLE_NATURE, beautyLevel, whitenessLevel);
        break;
    case RTCROOM_BEAUTY_STYLE_BLUR:
        m_mainPublisher.pusher()->setBeautyStyle(TXE_BEAUTY_STYLE_BLUR, beautyLevel, whitenessLevel);
        break;
    default:
        break;
    }
}

int RTCRBussiness::enumCameras(wchar_t ** camerasName, size_t capacity)
{
	return m_mainPublisher.pusher()->enumCameras(camerasName, capacity);
}

void RTCRBussiness::switchCamera(int cameraIndex)
{
	return m_mainPublisher.pusher()->switchCamera(cameraIndex);
}

int RTCRBussiness::micDeviceCount()
{
	return m_mainPublisher.pusher()->micDeviceCount();
}

bool RTCRBussiness::micDeviceName(unsigned int index, char name[MIC_DEVICE_NAME_MAX_SIZE])
{
	return m_mainPublisher.pusher()->micDeviceName(index, name);
}

void RTCRBussiness::selectMicDevice(unsigned int index)
{
	m_mainPublisher.pusher()->selectMicDevice(index);
}

unsigned int RTCRBussiness::micVolume()
{
	return m_mainPublisher.pusher()->micVolume();
}

void RTCRBussiness::setMicVolume(unsigned int volume)
{
	m_mainPublisher.pusher()->setMicVolume(volume);
}

// 群操作成功回调---创建、加入、删除、退出
void RTCRBussiness::onGroupOptSuccess(IMGroupOptType opType, const char* group_id)
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
void RTCRBussiness::onGroupOptError(IMGroupOptType opType, const char* group_id, int code, const char* desc)
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

void RTCRBussiness::onRecvC2CTextMsg(const char * userID, const char * msg)
{

}

void RTCRBussiness::onRecvGroupTextMsg(const char * groupID, const char * userID, const char * msg, const char * msgHead)
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

void RTCRBussiness::onRecvC2CCustomMsg(const char * userID, const char * msg)
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

    if (cmd == "sketchpad")
    {
        handleSketchPad(userID, root);
    }
}

void RTCRBussiness::onRecvGroupCustomMsg(const char * groupID, const char * userID, const char * msgContent)
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

    if (cmd == "notifyPusherChange" && (m_bInRoom || m_bCreateRoom))
    {
        getPushers();
    }
    else if (cmd == "CustomCmdMsg" && root["data"].isObject() &&  m_callback)
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

void RTCRBussiness::onRecvGroupSystemMsg(const char * groupID, const char * msg)
{

}

void RTCRBussiness::onKickOffline()
{
    m_callback->onTIMKickOffline();
	DataReport::instance().setResult(DataReportError, "fail:1004", "IM kickoffline");
	HttpReportRequest::instance().reportELK(DataReport::instance().getErrorReport());
}

void RTCRBussiness::onGroupChangeMessage(IMGroupOptType opType, const char* group_id, const char * user_id)
{
    LINFO(L"opType: %d, group_id: %s, user_id: %s", opType, group_id, user_id);

    if (opType == kgDeleteGroup)
    {
        if (m_roomData.roomID.compare(group_id) == 0)
        {
            // 房间解散后后台自动退房间

            m_bInRoom = false;
			::timeKillEvent(m_timerID);
            if (m_callback)
            {
                m_callback->onRoomClosed(m_roomData.roomID.c_str());
            }

			DataReport::instance().setResult(DataReportLeave, "fail:1001", "im group delete");
			HttpReportRequest::instance().reportELK(DataReport::instance().getLeaveReport());
        }
    }
}

void RTCRBussiness::onEventCallback(int eventId, const int paramCount, const char** paramKeys, const char** paramValues,
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
	case PushEvt::PUSH_ERR_OPEN_CAMERA_FAIL:
	{
		LINFO(L"Pusher open camera fail, userID: %s", Ansi2Wide(userID).c_str());
		
		//if (!m_bReport)
		//{
		//	m_bReport = true;
		//	DataReport::instance().setResult(DataReportEnter, "fail:1003", std::to_string(eventId));
		//	HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
		//}
		//else
		//{
		//	DataReport::instance().setResult(DataReportError, "fail:1003", std::to_string(eventId));
		//	HttpReportRequest::instance().reportELK(DataReport::instance().getErrorReport());
		//}
	}
	break;
	case PushEvt::PUSH_WARNING_DNS_FAIL || PushEvt::PUSH_WARNING_SEVER_CONN_FAIL || PushEvt::PUSH_WARNING_SHAKE_FAIL || PushEvt::PUSH_WARNING_SERVER_DISCONNECT || PushEvt::PUSH_WARNING_SERVER_NO_DATA:
	{
		LINFO(L"Pusher rtmp connect fail, userID: %s, eventid: %d", Ansi2Wide(userID).c_str(), eventId);
		if (m_bReport)
		{
			DataReport::instance().setResult(DataReportError, "fail:1003", std::to_string(eventId));
			HttpReportRequest::instance().reportELK(DataReport::instance().getErrorReport());
		}
		else
		{
			m_bReport = true;
			DataReport::instance().setResult(DataReportEnter, "fail:1003", std::to_string(eventId));
			HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
		}
	}
	break;
	case PushEvt::PUSH_WARNING_RECONNECT:
	{
		LINFO(L"Pusher reconnect, userID: %s", Ansi2Wide(userID).c_str());
	}
	break;
	case  PushEvt::PUSH_ERR_NET_DISCONNECT:
	{
		handlePushDisconnect(userID);
		if (m_bReport)
		{
			DataReport::instance().setResult(DataReportError, "fail:1003", std::to_string(eventId));
			HttpReportRequest::instance().reportELK(DataReport::instance().getErrorReport());
		}
		else
		{
			m_bReport = true;
			DataReport::instance().setResult(DataReportEnter, "fail:1003", std::to_string(eventId));
			HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
		}
		DataReport::instance().setResult(DataReportStream, "fail", "push_disconnect");
		DataReport::instance().setStreamAction("PusherQuit");
		DataReport::instance().setStreamID(getStreamID(m_pushUrl));
		HttpReportRequest::instance().reportELK(DataReport::instance().getStreamReport());
	}
	break;
	case  PushEvt::PUSH_EVT_CONNECT_SUCC:
	{
		DataReport::instance().setConnectSucc(DataReport::instance().txf_gettickcount());
	}
	break;
	case PushEvt::PUSH_EVT_PUSH_BEGIN:
	{
		LINFO(L"m_bPushBegin: %s", (true == m_bPushBegin ? L"true" : L"false"));

		if (m_bPushBegin)
		{
			return;
		}
		DataReport::instance().setPushBegin(DataReport::instance().txf_gettickcount());

		DataReport::instance().setResult(DataReportStream, "success", "push_begin");
		DataReport::instance().setStreamAction("PusherJoin");
		DataReport::instance().setStreamID(getStreamID(m_pushUrl));
		HttpReportRequest::instance().reportELK(DataReport::instance().getStreamReport());

		if (m_bCreateRoom)
		{
			handlePushBeginForCreate();
		}
		else if (m_bInRoom)
		{
			handlePushBeginForEnter();
		}
	}
	break;
	case PushEvt::PUSH_ERR_CAMERA_OCCUPY:
	{
		LINFO(L"PUSH_ERR_CAMERA_OCCUPY");

		if (m_callback)
		{
			m_callback->onError({ RTCROOM_ERR_CAMERA_OCCUPY, "摄像头已被占用，无法正常开课，请关闭正在使用摄像头的程序后重试。" }, m_authData.userID);
		}

		if (m_bReport)
		{
			DataReport::instance().setResult(DataReportError, "fail:1003", std::to_string(eventId));
			HttpReportRequest::instance().reportELK(DataReport::instance().getErrorReport());
		}
		else
		{
			m_bReport = true;
			DataReport::instance().setResult(DataReportEnter, "fail:1003", std::to_string(eventId));
			HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
		}
	}
	break;
	case PushEvt::PUSH_EVT_CAMERA_REMOVED:
	{
		LINFO(L"PUSH_EVT_CAMERA_REMOVED");

		if (m_callback)
		{
			m_callback->onError({ RTCROOM_ERR_CAMERA_REMOVED, "摄像头被拔出，无法正常开课，请先接入摄像头。" }, m_authData.userID);
		}

		if (m_bReport)
		{
			DataReport::instance().setResult(DataReportError, "fail:1003", std::to_string(eventId));
			HttpReportRequest::instance().reportELK(DataReport::instance().getErrorReport());
		}
		else
		{
			m_bReport = true;
			DataReport::instance().setResult(DataReportEnter, "fail:1003", std::to_string(eventId));
			HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
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
		if (!m_bCreateRoom)
		{
			break;
		}
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

		for (std::vector<RTCMemberData>::iterator it = m_roomData.members.begin(); m_roomData.members.end() != it; ++it)
		{
			if (it->userID == userID)
			{
				std::string streamID = getStreamID(it->accelerateURL);
				if (false == streamID.empty() && m_bRecord && m_bCreateRoom)
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

void CALLBACK RTCRBussiness::onTimerEvent(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    RTCRBussiness* impl = reinterpret_cast<RTCRBussiness*>(dwUser);
    if (impl && 0 != impl->m_timerID)
    {
        impl->m_httpRequest.heartbeat(impl->m_roomData.roomID, impl->m_authData.userID, [=](const RTCResult& res) {});
    }
}

void RTCRBussiness::getPushers()
{
    m_httpRequest.getPushers(m_roomData.roomID, [=](const RTCResult& res, const RTCRoomData& roomData) {
        if (RTCROOM_SUCCESS == res.ec)
        {
            std::vector<RTCMemberData> oldMembers = m_roomData.members;
            m_roomData = roomData;

            mergePushers(oldMembers);
        }

        if (m_callback)
        {
            m_callback->onUpdateRoomData(res, m_roomData);
        }

		if (!m_bReport)
		{
			m_bReport = true;
			DataReport::instance().setResult(DataReportEnter, "success");
			HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
		}
    });
}

void RTCRBussiness::mergePushers(const std::vector<RTCMemberData>& tempPushers)
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
			DataReport::instance().setResult(DataReportStream, "success", "quit");
			DataReport::instance().setStreamAction("PusherQuit");
			DataReport::instance().setStreamID(getStreamID(tempPushers[i].accelerateURL));
			HttpReportRequest::instance().reportELK(DataReport::instance().getStreamReport());
            LINFO(L"%s", Ansi2Wide(tempPushers[i].userID).c_str());

			std::string streamID = getStreamID(tempPushers[i].accelerateURL);
			if (false == streamID.empty() && m_bRecord && m_bCreateRoom)
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
			DataReport::instance().setResult(DataReportStream, "success", "join");
			DataReport::instance().setStreamAction("PusherJoin");
			DataReport::instance().setStreamID(getStreamID(m_roomData.members[i].accelerateURL));
			HttpReportRequest::instance().reportELK(DataReport::instance().getStreamReport());
            LINFO(L"%s", Ansi2Wide(m_roomData.members[i].userID).c_str());
        }
    }
}

void RTCRBussiness::handleSketchPad(const std::string& userID, const Json::Value& root)
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

void RTCRBussiness::handlePushBeginForCreate()
{
	LOGGER;

	m_bPushBegin = true;

	if (m_bRecord)
	{
		m_streamMixer.mergeStream(10);
	}

	m_httpRequest.createRoom(m_roomData.roomID, m_roomData.roomInfo, [=](const RTCResult& res, const std::string& roomID) {
		DataReport::instance().setCGICreateRoom(DataReport::instance().txf_gettickcount());

		if (RTCROOM_SUCCESS != res.ec)
		{
			if (m_callback)
			{
				m_callback->onCreateRoom(res, roomID);
			}
			m_bReport = true;
			DataReport::instance().setResult(DataReportEnter, "fail:1002", res.msg);
			HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
		}
		else
		{
			m_roomData.roomID = roomID;
			m_streamMixer.setRoomID(roomID);

			TIMManager::instance()->opGroup(kgJoinGroup, roomID.c_str(), this);

			m_timerID = ::timeSetEvent(5000, 1, onTimerEvent, (DWORD_PTR)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION); // 开启心跳定时器

			m_httpRequest.addPusher(m_roomData.roomID, m_authData.userID, m_authData.userName, m_authData.userAvatar, m_pushUrl, [=](const RTCResult& res) {
				DataReport::instance().setCGIAddPusher(DataReport::instance().txf_gettickcount());
				DataReport::instance().setRoomInfo(m_roomData.roomID);

				if (m_callback)
				{
					m_callback->onCreateRoom(res, roomID);
				}

				if (RTCROOM_SUCCESS == res.ec)
				{
					getPushers();
				}
				else
				{
					m_bReport = true;
					DataReport::instance().setResult(DataReportEnter, "fail:1002", res.msg);
					HttpReportRequest::instance().reportELK(DataReport::instance().getEnterReport());
				}
			});
		}
	});
}

void RTCRBussiness::handlePushBeginForEnter()
{
	LOGGER;

	m_bPushBegin = true;

	m_timerID = ::timeSetEvent(5000, 1, onTimerEvent, (DWORD_PTR)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION); // 开启心跳定时器

	m_httpRequest.addPusher(m_roomData.roomID, m_authData.userID, m_authData.userName, m_authData.userAvatar, m_pushUrl, [=](const RTCResult& res) {
		DataReport::instance().setCGIAddPusher(DataReport::instance().txf_gettickcount());
		if (m_callback)
		{
			m_callback->onEnterRoom(res);
		}

        if (RTCROOM_SUCCESS == res.ec)
        {
            TIMManager::instance()->opGroup(kgJoinGroup, m_roomData.roomID.c_str(), this);
            getPushers();
        }
	});
}

void RTCRBussiness::handlePushDisconnect(const std::string& userID)
{
    LINFO(L"Pusher disconnect, userID: %s", Ansi2Wide(userID).c_str());

	m_mainPublisher.pusher()->setCallback(NULL, NULL);
	m_mainPublisher.pusher()->stopAudioCapture();
    m_mainPublisher.pusher()->stopPreview();
    m_mainPublisher.pusher()->stopPush();

	if (m_bPushBegin) //已经开始推流，中途失败
	{
        if (m_callback)
        {
            m_callback->onError({ RTCROOM_ERR_PUSH_DISCONNECT, "推流断开，请检查网络设置" }, m_authData.userID);
        }
	}
	else
	{
        if (m_callback)
        {
            m_callback->onCreateRoom({ RTCROOM_ERR_PUSH_DISCONNECT, "推流断开，请检查网络设置" }, m_roomData.roomID);
        }
	}

	leaveRoom();
}

void RTCRBussiness::handlePlayBegin()
{
}

void RTCRBussiness::handlePlayDisconnect(const std::string& userID)
{
    LINFO(L"Player disconnect, userID: %s", Ansi2Wide(userID).c_str());

    if (m_callback)
    {
        m_callback->onError({ RTCROOM_ERR_PLAY_DISCONNECT, "拉流连接断开" }, userID);
    }

    for (std::vector<RTCMemberData>::iterator it = m_roomData.members.begin(); m_roomData.members.end() != it; ++it)
    {
        if (userID == it->userID)
        {
            m_callback->onPusherQuit(*it);
			DataReport::instance().setResult(DataReportStream, "fail", "play_disconnect");
			DataReport::instance().setStreamAction("PusherQuit");
			std::string streamID = getStreamID(it->accelerateURL);
			DataReport::instance().setStreamID(streamID);
			HttpReportRequest::instance().reportELK(DataReport::instance().getStreamReport());
            m_roomData.members.erase(it);

			if (false == streamID.empty() && m_bRecord && m_bCreateRoom)
			{
				m_streamMixer.removeSubStream(streamID);
			}
            break;
        }
    }
}

std::string RTCRBussiness::getStreamID(const std::string& streamURL)
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
