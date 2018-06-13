#pragma once

#include <QMainWindow>
#include "ui_RoomDemo.h"
#include "LiveVideoPanel.h"
#include "MemberPanel.h"
#include "IMPanel.h"
#include "LiveRoom.h"
#include "LiveRoomUtil.h"
#include "commonType.h"
#include "QWidgetToast.h"
#include "DeviceManage.h"
#include "BeautyManage.h"
#include "ImageDownloader.h"

class LiveDemo
    : public QMainWindow
    , public ILiveRoomCallback
	, public IIMPanelCallback
	, public ILoginLiveCallback
{
	Q_OBJECT

public:
	LiveDemo(QWidget *parent = Q_NULLPTR);
	virtual ~LiveDemo();

    void createRoom(const LRAuthData& authData, const QString& serverDomain, const std::string& ip, unsigned short port, const QString& roomID, const QString& roomInfo, bool record, int picture_id, ScreenRecordType screenRecord);
	void enterRoom(const LRAuthData& authData, const QString& serverDomain, const std::string& ip, unsigned short port, const QString& roomID, const QString& roomInfo, bool record, int picture_id, ScreenRecordType screenRecord);
	void setLogo(QString logoURL);
	void setTitle(QString title);
	void leaveRoom();
	void initUI(const QString& strTemplate, const QString& userTag, bool bUserList, bool bIMList, bool whiteboard, bool screenShare);
protected:
	virtual void onCreateRoom(const LRResult& res, const std::string& roomID);
	virtual void onEnterRoom(const LRResult& res);
	virtual void onUpdateRoomData(const LRResult& res, const LRRoomData& roomData);
    virtual void onGetAudienceList(const LRResult& res, const std::vector<LRAudienceData>& audiences);
	virtual void onPusherJoin(const LRMemberData & member);
	virtual void onPusherQuit(const LRMemberData & member);
	virtual void onRoomClosed(const std::string& roomID);
	virtual void onRecvRoomTextMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * msg);
	virtual void onRecvRoomCustomMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * message);
    virtual void onRecvC2CCustomMsg(const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * msg);
    virtual void onTIMKickOffline();
	virtual void onError(const LRResult& res, const std::string& userID);
	virtual void onRecvJoinPusherRequest(const std::string& roomID, const std::string& userID, const std::string& userName, const std::string& userAvatar);
	virtual void onRecvAcceptJoinPusher(const std::string& roomID, const std::string& msg);
	virtual void onRecvRejectJoinPusher(const std::string& roomID, const std::string& msg);
	virtual void onRecvKickoutSubPusher(const std::string& roomID);

	virtual void onLogin(const LRResult& res, const LRAuthData& authData);

	virtual void onSendIMGroupMsg(const std::string& msg);

	void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;	
	void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;
private:
	Ui::RoomDemo ui;

	QPoint mousePressedPosition; // 鼠标按下时的坐标
	QPoint windowPositionAsDrag; // 鼠标按下时窗口左上角的坐标

	LiveVideoPanel * m_mainPanel = nullptr;
	MemberPanel * m_memberPanel = nullptr;
	IMPanel* m_imPanel = nullptr;
	QWidgetToast * m_toast = nullptr;
	DeviceManage * m_deviceManage = nullptr;
    LRAuthData m_authData;
	BeautyManage * m_beautyManage = nullptr;
    std::string m_roomID;
    int m_timerID;
	bool m_bCreate;
	QString m_roomInfo;
	QString m_userTag;
    std::list<MemberItem> m_members;
	int m_demoWidth;
	int m_cameraSize = 4;
	int m_beautyStyle = 1;
	int m_beautyLevel = 5;
	int m_whitenessLevel = 5;

    ImageDownloader m_imgDownloader;
	ScreenRecordType m_screenRecord;

private:
	void init(const LRAuthData& authData, const QString& roomName, const std::string& ip, unsigned short port);
	void setInitBeautyValue();
signals:
	void dispatch(txfunction func);

private slots:
    void onDownloadFinished(bool success, QByteArray image);

	void on_btn_close_clicked();
    void on_btn_min_clicked();
	void on_btn_device_manage_clicked();
	void on_btn_beauty_manage_clicked();
	void on_cmb_mic_currentIndexChanged(int index);
	void on_cmb_camera_currentIndexChanged(int index);
	void on_slider_volume_valueChanged(int value);
	void on_device_manage_ok(bool enableCamera, bool enableMic);
	void on_device_manage_cancel(int cameraIndex, int micIndex, int micVolume);
	void on_beauty_manage_ok(int beautyStyle, int beautyLevel, int whitenessLevel);
	void on_chb_camera_stateChanged(int state);
	void handle(txfunction func);
};
