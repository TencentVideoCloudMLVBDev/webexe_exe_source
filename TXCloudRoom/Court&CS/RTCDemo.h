#pragma once

#include <QMainWindow>
#include "ui_RoomDemo.h"
#include "MultiVideoPanel.h"
#include "DoubleVideoPanel.h"
#include "MemberPanel.h"
#include "IMPanel.h"
#include "commonType.h"
#include "QWidgetToast.h"
#include "RTCRoom.h"
#include "commonType.h"
#include "DeviceManage.h"
#include "BeautyManage.h"
#include "ImageDownloader.h"

class RTCDemo
    : public QMainWindow
    , public IRTCRoomCallback
	, public IIMPanelCallback
	, public ILoginRTCCallback
{
	Q_OBJECT

public:
	RTCDemo(QWidget *parent = Q_NULLPTR);
	virtual ~RTCDemo();

	void createRoom(RTCAuthData authData, const QString& serverDomain, const QString& roomID, const QString& roomInfo, bool record, int picture_id, ScreenRecordType screenRecord);
	void enterRoom(RTCAuthData authData, const QString& serverDomain, const QString& roomID, const QString& roomInfo, bool record, int picture_id, ScreenRecordType screenRecord);
	void setLogo(QString logoURL, bool multi = true);
	void setTitle(QString title);
	void leaveRoom();
	void initUI(const QString& strTemplate, const QString& userTag, bool bUserList, bool bIMList, bool whiteboard, bool screenShare);

    void setProxy(const std::string& ip, unsigned short port);
protected:
	virtual void onCreateRoom(const RTCResult& res, const std::string& roomID);
	virtual void onEnterRoom(const RTCResult& res);
	virtual void onUpdateRoomData(const RTCResult& res, const RTCRoomData& roomData);
	virtual void onPusherJoin(const RTCMemberData & member);
	virtual void onPusherQuit(const RTCMemberData & member);
	virtual void onRoomClosed(const std::string& roomID);
	virtual void onRecvRoomTextMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * msg);
	virtual void onRecvRoomCustomMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * message);
    virtual void onRecvC2CCustomMsg(const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * msg);
    virtual void onTIMKickOffline();
	virtual void onError(const RTCResult& res, const std::string& userID);

	virtual void onLogin(const RTCResult& res, const RTCAuthData& authData);

	virtual void onSendIMGroupMsg(const std::string& msg);

	void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;
private:
	Ui::RoomDemo ui;

	QPoint mousePressedPosition; // 鼠标按下时的坐标
	QPoint windowPositionAsDrag; // 鼠标按下时窗口左上角的坐标

	MultiVideoPanel * m_multiPanel = nullptr;
	DoubleVideoPanel * m_doublePanel = nullptr;
	MemberPanel * m_memberPanel = nullptr;
	IMPanel* m_imPanel = nullptr;
	QWidgetToast * m_toast = nullptr;
	DeviceManage * m_deviceManage = nullptr;
    RTCAuthData m_authData;
	BeautyManage * m_beautyManage = nullptr;
	bool m_bCreate;
    QString m_roomID;
	QString m_roomInfo;
	QString m_userTag;
    std::list<MemberItem> m_members;
	int m_demoWidth;
    ImageDownloader m_imgDownloader;
	int m_cameraSize = 4;
	bool m_multi = true;
	int m_beautyStyle = 1;
	int m_beautyLevel = 5;
	int m_whitenessLevel = 5;
	ScreenRecordType m_screenRecord;

private:
	void init(const RTCAuthData& authData, const QString& roomName);
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
