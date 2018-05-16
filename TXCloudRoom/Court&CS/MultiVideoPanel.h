#pragma once

#include <QWidget>
#include "ui_MainPanel.h"
#include "WhiteBoard.h"
#include "RTCVideoWidget.h"
#include "RTCRoom.h"
#include "RTCRoomUtil.h"
#include <QScrollArea>

struct  RTCCameraWidget
{
	RTCVideoWidget* cameraWidget;
	bool idle;
	std::string userID;
};

class MultiVideoPanel : public QWidget
{
	Q_OBJECT

public:
	MultiVideoPanel(QWidget *parent = Q_NULLPTR);
	~MultiVideoPanel();

	void onPusherJoin(const MemberItem& member);
    void onPusherQuit(const MemberItem& member);
    void onRoomClosed();

	void setRoomCreator(const std::string& id);
	void setUserInfo(const std::string& id, const std::string& userName);
	int getVideoCount();

	void updatePreview();
	void setDeviceEnabled(bool camera, bool mic);

	void initConfigSetting(int size, bool whiteboard, bool screenShare);
	void initShowVideo();
protected:
	virtual void showEvent(QShowEvent *event) override;
	virtual void keyPressEvent(QKeyEvent *event) override;

private:
	void initUI();
	void initCameraWidget();

private:
	Ui::MainPanel ui;
	WhiteBoard * whiteBoard = nullptr;
	std::vector<RTCCameraWidget*> m_vCameraWidgets;
	QScrollArea *scrollArea_camera;
	QHBoxLayout * hCameraLayout;
	RTCVideoWidget * selfWidget = nullptr;
	QWidget* widget_tab_corner;
	int m_tabIndex = 0;
	bool m_screenFull = false;
	bool m_screenArea = false;
	bool m_cameraPreview = true;
	RECT m_areaRect;
	MenuInfo m_menuInfo;
	bool m_cameraEnabled = true;

	std::string m_roomCreator;
	std::string m_userName;
	std::string m_userID;
	int m_cameraSize;

private slots:
	void on_dis_actCamera(bool open);
	void on_dis_actMic(bool open);
	void on_local_actCamera(bool open);
	void on_selectCaptureArea(QRect rect);

	void on_tabWidget_currentChanged();
	void on_btn_area_share_clicked();
	void on_btn_full_share_clicked();
};
