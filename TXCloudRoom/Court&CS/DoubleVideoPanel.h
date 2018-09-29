#pragma once

#include <QWidget>
#include "ui_MainPanel.h"
#include "WhiteBoard.h"
#include "RTCVideoWidget.h"
#include "RTCRoom.h"
#include "RTCRoomUtil.h"
#include <QScrollArea>
#include "WinWidget.h"
#include "TXCloudRecordCmd.h"
#include "common/TXShareFrameMgr.h"
#include "RTCShareVideo.h"

class DoubleVideoPanel : public QWidget, public TXShareFrameCallback
{
	Q_OBJECT

public:
	DoubleVideoPanel(QWidget *parent = Q_NULLPTR);
	~DoubleVideoPanel();

	void initShowVideo();
	void onPusherJoin(const MemberItem& member);
    void onPusherQuit(const MemberItem& member);
    void onRoomClosed();

	void setRoomCreator(const std::string& id);
	void setUserInfo(const std::string& id, const std::string& userName);

	void updatePreview();
	void setDeviceEnabled(bool camera, bool mic);

	void initConfigSetting(bool whiteboard, bool screenShare, ScreenRecordType screenRecord);

	void on_startRecord(ScreenRecordType recordType);
protected:
	virtual void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
	virtual void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
	virtual void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
	virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

	virtual void onSwitch(HWND, QRect, bool bFollowWnd);
	virtual void onClose();
private:
	HWND m_hwndLocal;
	HWND m_hwndRemote;
	QWidget * widgetLocalDisplay;
	QWidget * widgetRemoteDisplay;
	WinWidget * winWidgetLocal;
	WinWidget * winWidgetRemote;

	Ui::MainPanel ui;
	WhiteBoard * whiteBoard = nullptr;
	QScrollArea * scrollArea_camera;
	QHBoxLayout * hCameraLayout;
	RTCVideoWidget * selfWidget = nullptr;
	RTCVideoWidget * remoteWidget = nullptr;
	QWidget* widget_tab_corner;
	int m_tabIndex = 0;
	bool m_screenFull = false;
	bool m_screenArea = false;
	bool m_cameraPreview = true;
	RECT m_areaRect;
	HWND m_shareHwnd = nullptr;
	bool m_bFollowWnd = false;
	TXShareFrameMgr *m_pTXShareFrameMgr = nullptr;
	MenuInfo m_menuInfo;
	std::string m_remoteID;
	bool m_cameraEnabled = true;

	std::string m_roomCreator;
	std::string m_userName;
	std::string m_userID;

	QPushButton * m_btn_device_manage;
	QPushButton * m_btn_beauty_manage;
	QPushButton * m_btn_esc;

	bool m_whiteboard = true;
	bool m_screenShare = true;

	QPushButton * m_btn_record_manage;
	ScreenRecordType m_screenRecord = RecordScreenNone;
	int m_timerID = 0;
	int m_recordTime = 0;
	QString m_btnStyle;
	QString m_recordBtnStyle;
	bool m_bResize = false;
	RTCShareVideo * m_RTCShareVideo = nullptr;
	bool m_bInitShowEvent = false;

private:
	void showRemoteWnd(bool show);
	void showLocalWnd(bool show);
	void resizeUI();
	void initUI();
	void shareVideo();

signals:
	void record_manage_clicked();

private slots:
	void on_btn_record_manage_clicked();
	void on_tabWidget_currentChanged();
	void on_btn_area_share_clicked();
	void on_btn_full_share_clicked();
	void on_btn_esc_clicked();
};
