#include "LiveVideoWidget.h"
#include "LiveRoom.h"

LiveVideoWidget::LiveVideoWidget(QWidget *parent, bool local)
	: QWidget(parent)
	, m_local(local)
{
	ui.setupUi(this);

	m_menuInfo.mainDis = m_local;
	ui.widget_display->setMenuInfo(m_menuInfo);
	ui.label_mute->hide();

	connect(ui.widget_display, SIGNAL(actLinkMic()), this, SLOT(on_dis_actLinkMic()));
	connect(ui.widget_display, SIGNAL(actCamera(bool)), this, SLOT(on_dis_actCamera(bool)));
	connect(ui.widget_display, SIGNAL(actMic(bool)), this, SLOT(on_dis_actMic(bool)));
}

LiveVideoWidget::~LiveVideoWidget()
{
}

void LiveVideoWidget::startVideo(std::string userID)
{
	ui.label_bg->hide();
	ui.label_mute->hide();
	m_menuInfo.linkMic = true;
	ui.widget_display->setMenuInfo(m_menuInfo);
	if (m_local)
	{
		LiveRoom::instance()->updateLocalPreview((HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() });
	}
	else
	{
		m_userID = userID;
		LiveRoom::instance()->addRemoteView(/*nullptr*/(HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() }, userID.c_str());
	}
}

void LiveVideoWidget::stopVideo()
{
	m_menuInfo.linkMic = false;
	ui.widget_display->setMenuInfo(m_menuInfo);
	m_userID = "";
	ui.label_bg->show();
	//ui.label_mute->show();
}

void LiveVideoWidget::setUserName(std::string userName)
{
	ui.label_user->setText(userName.c_str());
}

void LiveVideoWidget::setMenuInfo(MenuInfo & menuInfo)
{
	ui.widget_display->setMenuInfo(menuInfo);

	if (menuInfo.mic)
		ui.label_mute->hide();
	//else
	//	ui.label_mute->show();

	if (menuInfo.camera)
		ui.label_bg->hide();
	else
		ui.label_bg->show();
}

void LiveVideoWidget::on_dis_actLinkMic()
{
	LiveRoom::instance()->removeRemoteView(m_userID.c_str());
	LiveRoom::instance()->kickoutSubPusher(m_userID.c_str());
}

void LiveVideoWidget::on_dis_actCamera(bool open)
{
	if (m_local)
	{
		if (open)
		{
			ui.label_bg->hide();
			LiveRoom::instance()->startLocalPreview((HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() });
		}
		else
		{
			ui.label_bg->show();
			LiveRoom::instance()->stopLocalPreview();
		}
		emit local_actCamera(open);
	}
	else if (open)
	{
		ui.label_bg->hide();
		LiveRoom::instance()->addRemoteView(/*nullptr*/(HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() }, m_userID.c_str());
	}
	else
	{
		ui.label_bg->show();
		LiveRoom::instance()->removeRemoteView(m_userID.c_str());
	}
}

void LiveVideoWidget::on_dis_actMic(bool open)
{
	if (open)
		ui.label_mute->hide();
	//else
	//	ui.label_mute->show();

	if (m_local)
	{
		emit local_actMic(open);
	}
}
