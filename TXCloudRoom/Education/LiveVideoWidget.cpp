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

	connect(this, SIGNAL(actCamera(bool)), this, SLOT(on_dis_actCamera(bool)));
	connect(this, SIGNAL(actMic(bool)), this, SLOT(on_dis_actMic(bool)));
	connect(this, SIGNAL(actLinkMic()), this, SLOT(on_dis_actLinkMic()));

	winWidget = new WinWidget;

	widgetDisplay = winWidget->getWidget();
	m_hwnd = winWidget->getHwnd();
	//::SetParent(m_hwnd, (HWND)widgetDisplay->winId());
	QHBoxLayout * hCameraLayout = new QHBoxLayout(ui.widget_wnd);

	SetProp(m_hwnd, L"LiveVideoWidget", this);

	hCameraLayout->setMargin(0);
	hCameraLayout->setAlignment(Qt::AlignLeft);
	hCameraLayout->addWidget(widgetDisplay);
	widgetDisplay->setFixedSize(QSize(ui.widget_display->width(), ui.widget_display->height()));
	widgetDisplay->show();
	ui.widget_wnd->setLayout(hCameraLayout);
	ui.widget_wnd->hide();
}

LiveVideoWidget::~LiveVideoWidget()
{
}

void LiveVideoWidget::setCancelLinkMic(bool supportCancelLinkMic)
{
	m_menuInfo.linkMic = supportCancelLinkMic;
}

void LiveVideoWidget::on_menu_actCamera(bool open)
{
	m_menuInfo.camera = open;
	emit actCamera(open);
}

void LiveVideoWidget::on_menu_actMic(bool open)
{
	m_menuInfo.mic = open;
	emit actMic(open);
}

void LiveVideoWidget::on_menu_actLink()
{
	m_menuInfo.linkMic = false;
	emit actLinkMic();
}

void LiveVideoWidget::startVideo(std::string userID)
{
	ui.label_bg->hide();
	ui.label_mute->hide();
	ui.widget_display->setMenuInfo(m_menuInfo);

	ui.widget_wnd->show();
	widgetDisplay->show();
	QSize displaySize = QSize(ui.widget_display->width(), ui.widget_display->height());
	//::MoveWindow(m_hwnd, 0, 0, displaySize.width(), displaySize.height(), true);

	RECT clientRect{ 0, 0, displaySize.width(), displaySize.height() };

	if (m_local)
	{
		LiveRoom::instance()->updateLocalPreview(m_hwnd, clientRect);
	}
	else
	{
		m_userID = userID;
		LiveRoom::instance()->addRemoteView(m_hwnd, clientRect, userID.c_str());
	}
}

void LiveVideoWidget::stopVideo()
{
	ui.widget_wnd->hide();
	widgetDisplay->hide();
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

void LiveVideoWidget::setUserID(std::string userID)
{
	m_userID = userID;
}

void LiveVideoWidget::setMenuInfo(MenuInfo & menuInfo)
{
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

MenuInfo & LiveVideoWidget::getMenuInfo()
{
	return m_menuInfo;
}

void LiveVideoWidget::updatePreview()
{
	ui.label_bg->hide();
	ui.label_mute->hide();

	ui.widget_wnd->show();
	widgetDisplay->show();

	//::MoveWindow(m_hwnd, 0, 0, displaySize.width(), displaySize.height(), true);
	RECT clientRect{ 0, 0, ui.widget_display->width(), ui.widget_display->height() };

	if (m_local)
	{
		LiveRoom::instance()->updateLocalPreview(m_hwnd, clientRect);
	}
	else if (!m_userID.empty())
	{
		LiveRoom::instance()->updateRemotePreview(m_hwnd, clientRect, m_userID.c_str());
	}
}

void LiveVideoWidget::resizeEvent(QResizeEvent *event)
{
	QSize displaySize = QSize(ui.widget_display->width(), ui.widget_display->height());
	ui.widget_wnd->setFixedSize(displaySize);
	ui.label_bg->setFixedSize(displaySize);
	widgetDisplay->setFixedSize(displaySize);
	::MoveWindow(m_hwnd, 0, 0, displaySize.width(), displaySize.height(), true);
}

void LiveVideoWidget::on_dis_actLinkMic()
{
	LiveRoom::instance()->removeRemoteView(m_userID.c_str());
	LiveRoom::instance()->kickoutSubPusher(m_userID.c_str());
}

void LiveVideoWidget::on_dis_actCamera(bool open)
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
			LiveRoom::instance()->startLocalPreview(m_hwnd, clientRect);
		}
		else
		{
			ui.label_bg->show();
			ui.widget_wnd->hide();
			widgetDisplay->hide();
			LiveRoom::instance()->stopLocalPreview();
		}
		emit local_actCamera(open);
	}
	else if (open)
	{
		ui.label_bg->hide();
		ui.widget_wnd->show();
		LiveRoom::instance()->addRemoteView(m_hwnd, clientRect, m_userID.c_str());
	}
	else
	{
		ui.label_bg->show();
		ui.widget_wnd->hide();
		LiveRoom::instance()->removeRemoteView(m_userID.c_str());
	}
}

void LiveVideoWidget::on_dis_actMic(bool open)
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