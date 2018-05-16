#pragma once

#include "TXLiveSDKTypeDef.h"
#include "RTCRoom.h"
#include "HttpClient.h"
#include "TXLivePlayer.h"
#include "TXLivePusher.h"
#include "TXLiveSDKEventDef.h"
#include "TIMManager.h"
#include "RTCRHttpRequest.h"
#include "json.h"
#include "RTCStreamMixer.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmiscapi2.h>
#include <memory>
#include <functional>

typedef std::function<void(void)> txfunction;

class RTCMainPublisher
{
public:
    RTCMainPublisher();
    ~RTCMainPublisher();

    char* userID();
    void setUserID(const char* id);

    TXLivePusher* pusher();
private:
    char* m_userID;
    TXLivePusher* m_pusher;
};

class RTCSubPublisher
{
public:
    RTCSubPublisher();
    ~RTCSubPublisher();

    char* userID();
    void setUserID(const char* id);

    TXLivePlayer* player();
private:
    char* m_userID;
    TXLivePlayer* m_player;
};

class RTCRBussiness
    : public IMRecvMsgCallback
    , public IMGroupOptCallBack
    , public IMGroupChangeCallback
	, public TXLivePlayerCallback
	, public TXLivePusherCallback
{
public:
    RTCRBussiness();
    virtual ~RTCRBussiness();

    void setCallback(IRTCRoomCallback * callback);
    void setProxy(const std::string& ip, unsigned short port);

    void login(const std::string & serverDomain, const RTCAuthData & authData, ILoginRTCCallback* callback);
	void recordVideo(bool multi, int picture_id);
    void logout();

    void getRoomList(int index, int cnt, IGetRTCRoomListCallback* callback);
    void createRoom(const std::string& roomID, const std::string& roomInfo);
    void enterRoom(const std::string& roomID);
    void leaveRoom();

    void sendRoomTextMsg(const char * msg);
    void sendRoomCustomMsg(const char * cmd, const char * msg);
    void sendC2CCustomMsg(const char* userID, const char * cmd, const char * msg);

    void startLocalPreview(HWND rendHwnd, const RECT &rect);
    void updateLocalPreview(HWND rendHwnd, const RECT &rect);
    void stopLocalPreview();
    bool startScreenPreview(HWND rendHwnd, HWND captureHwnd, const RECT &renderRect, const RECT &captureRect);
    void stopScreenPreview();
    void addRemoteView(HWND rendHwnd, const RECT &rect, const char * userID);
    void removeRemoteView(const char * userID);
	void updateRemotePreview(HWND rendHwnd, const RECT &rect, const char * userID);

    void setMute(bool mute);
    void setVideoQuality(RTCVideoQuality quality, RTCAspectRatio ratio);
    void setBeautyStyle(RTCBeautyStyle beautyStyle, int beautyLevel, int whitenessLevel);

	int enumCameras(wchar_t **camerasName = NULL, size_t capacity = 0);
	void switchCamera(int cameraIndex);
	int micDeviceCount();
	bool micDeviceName(unsigned int index, char name[MIC_DEVICE_NAME_MAX_SIZE]);
	void selectMicDevice(unsigned int index);
	unsigned int micVolume();
	void setMicVolume(unsigned int volume);
private:
    static void onTIMLoginSuccess(void* data);
    static void onTIMLoginError(int code, const char* desc, void* data);

    void onGroupOptSuccess(IMGroupOptType opType, const char* group_id) override;
    void onGroupOptError(IMGroupOptType opType, const char* group_id, int code, const char* desc) override;

    // IM接收消息回调
    virtual void onRecvC2CTextMsg(const char * userID, const char * msg);
    virtual void onRecvGroupTextMsg(const char * groupID, const char * userID, const char * msg, const char * msgHead = nullptr);
    virtual void onRecvC2CCustomMsg(const char * userID, const char * msg);
    virtual void onRecvGroupCustomMsg(const char * groupID, const char * userID, const char * msg);
    virtual void onRecvGroupSystemMsg(const char * groupID, const char * msg);

    virtual void onGroupChangeMessage(IMGroupOptType opType, const char* group_id, const char * user_id);

    virtual void onEventCallback(int eventId, const int paramCount, const char **paramKeys, const char **paramValues, void *pUserData);

private:
    void getPushers();
    void mergePushers(const std::vector<RTCMemberData>& tempPushers);

    void handleSketchPad(const std::string& userID, const Json::Value& root);

	void handlePushBeginForCreate();
	void handlePushBeginForEnter();
	void handlePushDisconnect(const std::string& userID);
	void handlePlayBegin();
	void handlePlayDisconnect(const std::string& userID);

	std::string getStreamID(const std::string& streamURL);

protected:
    static void CALLBACK onTimerEvent(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

private:
	RTCAuthData m_authData;
    RTCRHttpRequest m_httpRequest;

    IRTCRoomCallback * m_callback;
    MMRESULT m_timerID;

    std::vector<RTCRoomData> m_roomList;

    RTCRoomData m_roomData;
    std::string m_pushUrl;

    RTCMainPublisher m_mainPublisher;
    std::map<std::string, RTCSubPublisher*> m_subPublisher;

    TaskQueue m_delayCleaner;       // 延迟销毁任务队列

    bool m_bPushBegin; // enter/create时，若推流失败则回调onEnterRoom或者onCreateRoom, 如果已经在推流，推流失败后使用onError
    bool m_bCreateRoom;
    bool m_bInRoom;
	TXEVideoQualityParamPreset m_quality = TXE_VIDEO_QUALITY_STANDARD_DEFINITION;
	RTCStreamMixer m_streamMixer;
	bool m_bRecord = false;
};
