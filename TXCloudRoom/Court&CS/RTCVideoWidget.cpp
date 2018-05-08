#include "RTCVideoWidget.h"
#include "RTCRoom.h"

RTCVideoWidget::RTCVideoWidget(QWidget *parent, bool local)
	: QWidget(parent)
	, m_local(local)
{
	ui.setupUi(this);

	MenuInfo menuInfo;
	menuInfo.mainDis = m_local; 
	menuInfo.linkMic = false;
	ui.widget_display->setMenuInfo(menuInfo);
	ui.label_mute->hide();

	connect(ui.widget_display, SIGNAL(actCamera(bool)), this, SLOT(on_dis_actCamera(bool)));
	connect(ui.widget_display, SIGNAL(actMic(bool)), this, SLOT(on_dis_actMic(bool)));
}

RTCVideoWidget::~RTCVideoWidget()
{
}

void RTCVideoWidget::startVideo(std::string userID)
{
	ui.label_bg->hide();
	ui.label_mute->hide();
	if (m_local)
	{
		RTCRoom::instance()->startLocalPreview((HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() });
	}
	else
	{
		m_userID = userID;
		RTCRoom::instance()->addRemoteView(/*nullptr*/(HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() }, userID.c_str());
	}
}

void RTCVideoWidget::stopVideo()
{
	m_userID = "";
	ui.label_bg->show();
	//ui.label_mute->show();
}

void RTCVideoWidget::setUserName(std::string userName)
{
	ui.label_user->setText(userName.c_str());
}

void RTCVideoWidget::setUserID(std::string userID)
{
	m_userID = userID;
}

void RTCVideoWidget::setMenuInfo(MenuInfo & menuInfo)
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

void RTCVideoWidget::updatePreview()
{
	ui.label_bg->hide();
	ui.label_mute->hide();
	if (m_local)
	{
		RTCRoom::instance()->updateLocalPreview((HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() });
	}
	else if (!m_userID.empty())
	{
		RTCRoom::instance()->updateRemotePreview(/*nullptr*/(HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() }, m_userID.c_str());
	}
}

void RTCVideoWidget::on_dis_actCamera(bool open)
{
	if (m_local)
	{
		if (open)
		{
			ui.label_bg->hide();
			RTCRoom::instance()->startLocalPreview((HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() });
		}
		else
		{
			ui.label_bg->show();
			RTCRoom::instance()->stopLocalPreview();
		}
		emit local_actCamera(open);
	}
	else if (open)
	{
		ui.label_bg->hide();
		RTCRoom::instance()->addRemoteView(/*nullptr*/(HWND)ui.widget_display->winId(), RECT{ 0, 0, ui.widget_display->width(), ui.widget_display->height() }, m_userID.c_str());
	}
	else
	{
		ui.label_bg->show();
		RTCRoom::instance()->removeRemoteView(m_userID.c_str());
	}
}

void RTCVideoWidget::on_dis_actMic(bool open)
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
