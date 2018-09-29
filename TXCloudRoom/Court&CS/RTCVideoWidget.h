#pragma once

#include <QWidget>
#include "ui_VideoWidget.h"
#include "commonType.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "WinWidget.h"

class RTCVideoWidget : public QWidget
{
	Q_OBJECT

public:
	RTCVideoWidget(QWidget *parent = Q_NULLPTR, bool local = false);
	~RTCVideoWidget();
	void on_menu_actCamera(bool open);
	void on_menu_actMic(bool open);
	MenuInfo & getMenuInfo();

	void startVideo(std::string userID);
	void stopVideo();
	void setUserName(std::string userName);
	void setUserID(std::string userID);
	void setMenuInfo(MenuInfo & menuInfo);
	void updatePreview();
signals:
	void local_actCamera(bool open); 
	void local_actMic(bool open);
	void actCamera(bool open);
	void actMic(bool open);

protected:
	virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

private slots:
	void on_dis_actCamera(bool open);
	void on_dis_actMic(bool open);

private:
	Ui::VideoWidget ui;
	std::string	m_userID;
	bool m_local;
	HWND m_hwnd;
	QWidget * widgetDisplay;
	MenuInfo m_menuInfo;
	WinWidget * winWidget;
};
