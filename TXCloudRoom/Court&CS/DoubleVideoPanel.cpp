#include "DoubleVideoPanel.h"
#include "CaptureScreen.h"

extern QWidget * RTCMainWindow;

DoubleVideoPanel::DoubleVideoPanel(QWidget *parent)
	: QWidget(parent)
	, m_remoteID("")
	, m_roomCreator("")
{
	ui.setupUi(this);

	initUI();
}

DoubleVideoPanel::~DoubleVideoPanel()
{
	RTCRoom::instance()->stopLocalPreview();
}

void DoubleVideoPanel::initShowVideo()
{
	ui.label_bg->hide();
	ui.label_mute->hide();
	ui.widget_camera_tip->lower();
	ui.widget_camera_tip->setWindowFlags(Qt::WindowStaysOnBottomHint);
}

void DoubleVideoPanel::onPusherJoin(const MemberItem& member)
{
	if (!m_remoteID.empty())
	{
		return;
	}
	ui.label_mute_main->hide();
	ui.widget_camera_tip->hide();
	m_remoteID = member.userID;
	remoteWidget->show();
	remoteWidget->setUserID(m_remoteID);
	remoteWidget->setUserName(member.userName);
	if (m_tabIndex == 1)
	{
		remoteWidget->startVideo(member.userID);
	}
	else
		RTCRoom::instance()->addRemoteView((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() }, member.userID.c_str());
}

void DoubleVideoPanel::onPusherQuit(const MemberItem& member)
{
	ui.widget_camera_tip->show();
	ui.label_mute_main->show();
	remoteWidget->hide();
	remoteWidget->stopVideo();
	RTCRoom::instance()->removeRemoteView(m_remoteID.c_str());
	m_remoteID = "";
}

void DoubleVideoPanel::onRoomClosed()
{
	remoteWidget->hide();
	selfWidget->hide();
	RTCRoom::instance()->stopLocalPreview();
	RTCRoom::instance()->removeRemoteView(m_remoteID.c_str());
	ui.widget_camera_tip->show();
	ui.label_mute_main->show();
	ui.label_bg->show();
	ui.label_mute->show();
}

void DoubleVideoPanel::setRoomCreator(const std::string& id)
{
	m_roomCreator = id;
	if (!whiteBoard)
	{
		whiteBoard = new WhiteBoard(ui.widget_board);
		QVBoxLayout * vBoardLayout = new QVBoxLayout(ui.widget_board);
		vBoardLayout->setMargin(0);
		vBoardLayout->addWidget(whiteBoard);
		ui.widget_board->setLayout(vBoardLayout);
		whiteBoard->show();
	}

	if (m_roomCreator == m_userID && whiteBoard)
	{
		whiteBoard->setMainUser(true);
	}
}

void DoubleVideoPanel::setUserInfo(const std::string & id, const std::string & userName)
{
	m_userID = id;
	m_userName = userName;
	selfWidget->setUserName(userName);
}

void DoubleVideoPanel::updatePreview()
{
	on_tabWidget_currentChanged();
}

void DoubleVideoPanel::setDeviceEnabled(bool camera, bool mic)
{
	m_cameraEnabled = camera;
	if (!camera)
	{
		RTCRoom::instance()->stopLocalPreview();
		m_cameraPreview = false;
		ui.widget_camera_tip->show();
	}
	else
		ui.widget_camera_tip->hide();

	m_menuInfo.mic = mic;
	ui.dis_main->setMenuInfo(m_menuInfo);

	if (mic)
		ui.label_mute_main->hide();
	else
		ui.label_mute_main->show();

	RTCRoom::instance()->setMute(!mic);
}

void DoubleVideoPanel::initConfigSetting(bool whiteboard, bool screenShare)
{
	if (!screenShare)
	{
		ui.tabWidget->removeTab(2);
	}
	if (!whiteboard)
	{
		ui.tabWidget->removeTab(1);
	}
}

void DoubleVideoPanel::showEvent(QShowEvent * event)
{
	static bool init = true;
	if (init)
	{
		ui.widget_camera_tip->setFixedHeight(ui.dis_main->height());
		QRect dismain = ui.dis_main->geometry();
		ui.widget_local->setGeometry(QRect(dismain.right() - 187, dismain.bottom() - 140, 187, 140));
		ui.widget_local->raise();
		ui.widget_local->setWindowFlags(Qt::WindowStaysOnTopHint);
		RTCRoom::instance()->startLocalPreview((HWND)ui.widget_local->winId(), RECT{ 0, 0, ui.widget_local->width(), ui.widget_local->height() });
		init = false;
	}
}

void DoubleVideoPanel::keyPressEvent(QKeyEvent * event)
{
	switch (event->key())
	{
	case Qt::Key_Escape:
	{
		if (m_tabIndex == 2 && (m_screenFull || m_screenArea))
		{
			m_screenFull = false;
			m_screenArea = false;
			m_cameraPreview = true;

			RTCRoom::instance()->stopScreenPreview();
			ui.stacked_screen->setCurrentIndex(0);

			RTCRoom::instance()->stopScreenPreview();
			RTCRoom::instance()->startLocalPreview((HWND)ui.widget_local->winId(), RECT{ 0, 0, ui.widget_local->width(), ui.widget_local->height() });

			if (m_cameraPreview)
				RTCRoom::instance()->updateLocalPreview((HWND)ui.widget_local->winId(), RECT{ 0, 0, ui.widget_local->width(), ui.widget_local->height() });
			if (!m_remoteID.empty())
			{
				RTCRoom::instance()->updateRemotePreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() }, m_remoteID.c_str());
			}
		}
	}
	break;
	default:
		QWidget::keyPressEvent(event);
	}
}

void DoubleVideoPanel::initUI()
{
	ui.widget_camera1->hide();

	scrollArea_camera = new QScrollArea(ui.widget_camera2);
	scrollArea_camera->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea_camera->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea_camera->setFixedSize(QSize(780, 185));
	scrollArea_camera->setContentsMargins(QMargins(0, 0, 0, 0));
	ui.hLayout_board->addWidget(scrollArea_camera);
	hCameraLayout = new QHBoxLayout(scrollArea_camera);
	hCameraLayout->setMargin(0);
	hCameraLayout->setAlignment(Qt::AlignLeft);
	scrollArea_camera->setLayout(hCameraLayout);

	selfWidget = new RTCVideoWidget(scrollArea_camera, true);
	hCameraLayout->addWidget(selfWidget);
	selfWidget->show();

	remoteWidget = new RTCVideoWidget(scrollArea_camera);
	hCameraLayout->addWidget(remoteWidget);
	remoteWidget->hide();
	ui.label_mute_main->hide();
	ui.label_tip->setText(QStringLiteral("暂时没有客户登录"));
	//ui.tabWidget->removeTab(2);
	//ui.tabWidget->removeTab(1);

	widget_tab_corner = new QWidget(this);
	QPushButton * btn_device_manage = new QPushButton(QStringLiteral("设备管理"), widget_tab_corner);
	QPushButton * btn_beauty_manage = new QPushButton(QStringLiteral("美颜设置"), widget_tab_corner);

	QString btnStyle =
		R"(
QPushButton {
	border: 1px solid #dddddd;
	background:  #ffffff;
	color: #000000;
	font: 9pt "Microsoft YaHei";
}

QPushButton:hover {
	border: 1px solid #d2d2d2;
	background:  #f2f2f2;
}

QPushButton:pressed {
	border: 1px solid #d2d2d2;
	background:  #e5e5e5;
}

QPushButton:disable {
	border: 1px solid #d5d5d5;
	color: #bbbbbb;
	background:  #f2f2f2;
}
)";
	const QSize btnSize = QSize(72, 26);
	btn_device_manage->setStyleSheet(btnStyle);
	btn_device_manage->setFixedSize(btnSize);

	btn_beauty_manage->setStyleSheet(btnStyle);
	btn_beauty_manage->setFixedSize(btnSize);

	connect(btn_device_manage, SIGNAL(released()), RTCMainWindow, SLOT(on_btn_device_manage_clicked()));
	connect(btn_beauty_manage, SIGNAL(released()), RTCMainWindow, SLOT(on_btn_beauty_manage_clicked()));

	QHBoxLayout* hLayout = new QHBoxLayout(widget_tab_corner);
	hLayout->addWidget(btn_beauty_manage);
	hLayout->addWidget(btn_device_manage);
	hLayout->setMargin(0);
	widget_tab_corner->setStyleSheet("margin-top: 2");
	ui.tabWidget->setCornerWidget(widget_tab_corner);
	widget_tab_corner->show();
}

void DoubleVideoPanel::on_selectCaptureArea(QRect rect)
{
	m_screenArea = true;
	m_areaRect.left = rect.x();
	m_areaRect.top = rect.y();
	m_areaRect.right = rect.x() + rect.width();
	m_areaRect.bottom = rect.y() + rect.height();
	ui.stacked_screen->setCurrentIndex(1);

	RTCRoom::instance()->stopLocalPreview();
	RTCRoom::instance()->startScreenPreview((HWND)ui.page_screen_content->winId(), nullptr, RECT{ 0, 0, ui.page_screen_content->width(), ui.page_screen_content->height() }, m_areaRect);
}

void DoubleVideoPanel::on_tabWidget_currentChanged()
{
	if (!whiteBoard)
	{
		whiteBoard = new WhiteBoard(ui.widget_board);
		QVBoxLayout * vBoardLayout = new QVBoxLayout(ui.widget_board);
		vBoardLayout->setMargin(0);
		vBoardLayout->addWidget(whiteBoard);
		ui.widget_board->setLayout(vBoardLayout);
		whiteBoard->show();
	}

	m_tabIndex = ui.tabWidget->currentIndex();
	switch (m_tabIndex)
	{
	case 0:
	{
		widget_tab_corner->show();
		if (((m_screenFull || m_screenArea) && m_cameraPreview) || (!m_cameraPreview && m_cameraEnabled))
		{
			m_cameraPreview = true;
			RTCRoom::instance()->stopScreenPreview();
			RTCRoom::instance()->startLocalPreview((HWND)ui.widget_local->winId(), RECT{ 0, 0, ui.widget_local->width(), ui.widget_local->height() });
		}
		if (m_cameraPreview)
			RTCRoom::instance()->updateLocalPreview((HWND)ui.widget_local->winId(), RECT{ 0, 0, ui.widget_local->width(), ui.widget_local->height() });
		if (!m_remoteID.empty())
		{
			RTCRoom::instance()->updateRemotePreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() }, m_remoteID.c_str());
		}
	}
	break;
	case 1:
	{
		widget_tab_corner->hide();
		if (((m_screenFull || m_screenArea) && m_cameraPreview) || (!m_cameraPreview && m_cameraEnabled))
		{
			m_cameraPreview = true;
			RTCRoom::instance()->stopScreenPreview();
			selfWidget->startVideo(m_userID);
		}

		if (m_cameraPreview)
		{
			selfWidget->updatePreview();
			selfWidget->setMenuInfo(m_menuInfo);
		}

		if (!m_remoteID.empty())
		{
			remoteWidget->updatePreview();
		}
	}
	break;
	case 2:
	{
		widget_tab_corner->hide();
		if (!m_screenArea && !m_screenFull)
			break;

		RTCRoom::instance()->stopLocalPreview();
		if (m_screenFull)
			RTCRoom::instance()->startScreenPreview((HWND)ui.page_screen_content->winId(), nullptr, RECT{ 0, 0, ui.page_screen_content->width(), ui.page_screen_content->height() }, RECT{ 0 });
		else if (m_screenArea)
			RTCRoom::instance()->startScreenPreview((HWND)ui.page_screen_content->winId(), nullptr, RECT{ 0, 0, ui.page_screen_content->width(), ui.page_screen_content->height() }, m_areaRect);
	}
	break;
	default:
		break;
	}
}

void DoubleVideoPanel::on_btn_area_share_clicked()
{
	CaptureScreen* captureHelper = new CaptureScreen();
	connect(captureHelper, SIGNAL(signalSelectRect(QRect)), this, SLOT(on_selectCaptureArea(QRect)));
	captureHelper->show();
}

void DoubleVideoPanel::on_btn_full_share_clicked()
{
	m_screenFull = true;
	ui.stacked_screen->setCurrentIndex(1);
	RTCRoom::instance()->stopLocalPreview();
	RTCRoom::instance()->startScreenPreview((HWND)ui.page_screen_content->winId(), nullptr, RECT{ 0, 0, ui.page_screen_content->width(), ui.page_screen_content->height() }, RECT{ 0 });
}