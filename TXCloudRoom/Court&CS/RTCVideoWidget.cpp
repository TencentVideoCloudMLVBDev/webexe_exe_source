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

	connect(this, SIGNAL(actCamera(bool)), this, SLOT(on_dis_actCamera(bool)));
	connect(this, SIGNAL(actMic(bool)), this, SLOT(on_dis_actMic(bool)));

	winWidget = new WinWidget;

	widgetDisplay = winWidget->getWidget();
	m_hwnd = winWidget->getHwnd();
	QHBoxLayout * hCameraLayout = new QHBoxLayout(ui.widget_wnd);

	SetProp(m_hwnd, L"RTCVideoWidget", this);

	hCameraLayout->setMargin(0);
	hCameraLayout->setAlignment(Qt::AlignLeft);
	hCameraLayout->addWidget(widgetDisplay);
	widgetDisplay->setFixedSize(QSize(ui.widget_display->width(), ui.widget_display->height()));
	widgetDisplay->show();
	ui.widget_wnd->setLayout(hCameraLayout);
	ui.widget_wnd->hide();
}

RTCVideoWidget::~RTCVideoWidget()
{
}

void RTCVideoWidget::on_menu_actCamera(bool open)
{
	m_menuInfo.camera = open;
	emit actCamera(open);
}

void RTCVideoWidget::on_menu_actMic(bool open)
{
	m_menuInfo.mic = open;
	emit actMic(open);
}

void RTCVideoWidget::startVideo(std::string userID)
{
	ui.label_bg->hide();
	ui.label_mute->hide();

	ui.widget_wnd->show();
	widgetDisplay->show();

	RECT clientRect{ 0, 0, ui.widget_display->width(), ui.widget_display->height() };

	if (m_local)
	{
		RTCRoom::instance()->startLocalPreview(m_hwnd, clientRect);
	}
	else
	{
		m_userID = userID;
		RTCRoom::instance()->addRemoteView(m_hwnd, clientRect, userID.c_str());
	}
}

void RTCVideoWidget::stopVideo()
{
	ui.widget_wnd->hide();
	widgetDisplay->hide();
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
	m_menuInfo = menuInfo;
	ui.widget_display->setMenuInfo(menuInfo);	
	if (menuInfo.mic)
		ui.label_mute->hide();
	//else
	//	ui.label_mute->show();

	if (menuInfo.camera)
	{
		ui.label_bg->hide();
		ui.widget_wnd->show();
	}
	else
	{
		ui.label_bg->show();
		ui.widget_wnd->hide();
	}
}

MenuInfo & RTCVideoWidget::getMenuInfo()
{
	return m_menuInfo;
}

void RTCVideoWidget::resizeEvent(QResizeEvent *event)
{
	QSize displaySize = QSize(ui.widget_display->width(), ui.widget_display->height());
	ui.widget_wnd->setFixedSize(displaySize);
	ui.label_bg->setFixedSize(displaySize);
	widgetDisplay->setFixedSize(displaySize);
	::MoveWindow(m_hwnd, 0, 0, displaySize.width(), displaySize.height(), true);
}

void RTCVideoWidget::updatePreview()
{
	ui.label_bg->hide();
	ui.label_mute->hide();

	ui.widget_wnd->show();
	widgetDisplay->show();

	//::MoveWindow(m_hwnd, 0, 0, displaySize.width(), displaySize.height(), true);
	RECT clientRect{ 0, 0, ui.widget_display->width(), ui.widget_display->height() };

	if (m_local)
	{
		RTCRoom::instance()->updateLocalPreview(m_hwnd, clientRect);
	}
	else if (!m_userID.empty())
	{
		RTCRoom::instance()->updateRemotePreview(m_hwnd, clientRect, m_userID.c_str());
	}
}

void RTCVideoWidget::on_dis_actCamera(bool open)
{
	RECT clientRect;
	::GetClientRect(m_hwnd, &clientRect);

	m_menuInfo.camera = open;
	ui.widget_display->setMenuInfo(m_menuInfo);

	if (m_local)
	{
		if (open)
		{
			ui.label_bg->hide();
			ui.widget_wnd->show();
			widgetDisplay->show();
			RTCRoom::instance()->startLocalPreview(m_hwnd, clientRect);
		}
		else
		{
			ui.label_bg->show();
			ui.widget_wnd->hide();
			widgetDisplay->hide();
			RTCRoom::instance()->stopLocalPreview();
		}
		emit local_actCamera(open);
	}
	else if (open)
	{
		ui.label_bg->hide();
		ui.widget_wnd->show();
		RTCRoom::instance()->addRemoteView(m_hwnd, clientRect, m_userID.c_str());
	}
	else
	{
		ui.label_bg->show();
		ui.widget_wnd->hide();
		RTCRoom::instance()->removeRemoteView(m_userID.c_str());
	}
}

void RTCVideoWidget::on_dis_actMic(bool open)
{
	m_menuInfo.mic = open;
	ui.widget_display->setMenuInfo(m_menuInfo);

	if (open)
		ui.label_mute->hide();
	//else
	//	ui.label_mute->show();

	if (m_local)
	{
		emit local_actMic(open);
	}
}
