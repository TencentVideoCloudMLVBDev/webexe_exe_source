#pragma once

#include <QWidget>
#include "ui_MainPanel.h"
#include "WhiteBoard.h"
#include "RTCVideoWidget.h"
#include "RTCRoom.h"
#include "RTCRoomUtil.h"
#include <QScrollArea>

class DoubleVideoPanel : public QWidget
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

	void initConfigSetting(bool whiteboard, bool screenShare);

protected:
	virtual void showEvent(QShowEvent *event) override;
	virtual void keyPressEvent(QKeyEvent *event) override;

private:
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
	MenuInfo m_menuInfo;
	std::string m_remoteID;
	void initUI();
	bool m_cameraEnabled = true;

	std::string m_roomCreator;
	std::string m_userName;
	std::string m_userID;

private slots:
	void on_selectCaptureArea(QRect rect);

	void on_tabWidget_currentChanged();
	void on_btn_area_share_clicked();
	void on_btn_full_share_clicked();
};
