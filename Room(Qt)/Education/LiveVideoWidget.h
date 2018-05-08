#pragma once

#include <QWidget>
#include "ui_VideoWidget.h"
#include "commonType.h"

class LiveVideoWidget : public QWidget
{
	Q_OBJECT

public:
	LiveVideoWidget(QWidget *parent = Q_NULLPTR, bool local = false);
	~LiveVideoWidget();

	void startVideo(std::string userID);
	void stopVideo();
	void setUserName(std::string userName);
	void setMenuInfo(MenuInfo & menuInfo);

signals:
	void local_actCamera(bool open); 
	void local_actMic(bool open);

private slots:
	void on_dis_actLinkMic();
	void on_dis_actCamera(bool open);
	void on_dis_actMic(bool open);

private:
	Ui::VideoWidget ui;
	std::string	m_userID;
	bool m_local;
	MenuInfo m_menuInfo;
};
