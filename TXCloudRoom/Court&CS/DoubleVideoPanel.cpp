#include "DoubleVideoPanel.h"
#include "CaptureScreen.h"
#include <QSettings>
#include "DialogMessage.h"
#include <QDateTime>
#include <QKeyEvent>

extern QWidget * RTCMainWindow;
#define NORMAL_LOCAL_WIDTH 150
#define MAX_LOCAL_WIDTH 200

DoubleVideoPanel::DoubleVideoPanel(QWidget *parent)
	: QWidget(parent)
	, m_remoteID("")
	, m_roomCreator("")
{
	ui.setupUi(this);

	initUI();
	m_pTXShareFrameMgr = new TXShareFrameMgr(this, nullptr);
}

DoubleVideoPanel::~DoubleVideoPanel()
{
	RTCRoom::instance()->stopLocalPreview();
	if (m_pTXShareFrameMgr)
	{
		delete m_pTXShareFrameMgr;
		m_pTXShareFrameMgr = nullptr;
}
}

void DoubleVideoPanel::initShowVideo()
{
	showLocalWnd(true);
}

void DoubleVideoPanel::onPusherJoin(const MemberItem& member)
{
	if (!m_remoteID.empty())
	{
		return;
	}
	showRemoteWnd(true);

	m_remoteID = member.userID;
	remoteWidget->setUserID(m_remoteID);
	remoteWidget->setUserName(member.userName);

	RECT clientRect;
	::GetClientRect(m_hwndRemote, &clientRect);

	if (m_tabIndex == 1)
	{
		remoteWidget->startVideo(member.userID);
	}
	else
		RTCRoom::instance()->addRemoteView(m_hwndRemote, clientRect, member.userID.c_str());

	switch (m_tabIndex)
	{
	case 0:
	{
		RTCRoom::instance()->addRemoteView(m_hwndRemote, clientRect, member.userID.c_str());
		m_RTCShareVideo->setUserInfo(member.userName, member.userID, true, false);
	}
	break;
	case 1:
	{
		remoteWidget->startVideo(member.userID);
		m_RTCShareVideo->setUserInfo(member.userName, member.userID, true, false);
	}
	break;
	case 2:
	{
		m_RTCShareVideo->setUserInfo(member.userName, member.userID, true, true);
	}
	break;
	default:
		break;
	}

	SetForegroundWindow(m_hwndRemote);
	SetWindowPos(m_hwndRemote, m_hwndLocal, 0, 0, 0, 0, 3);
}

void DoubleVideoPanel::onPusherQuit(const MemberItem& member)
{
	showRemoteWnd(false);
	
	remoteWidget->stopVideo();
	RTCRoom::instance()->removeRemoteView(m_remoteID.c_str());
	m_RTCShareVideo->setUserInfo(member.userName, member.userID, false, false);
	m_remoteID = "";	
}

void DoubleVideoPanel::onRoomClosed()
{
	showRemoteWnd(false);
	showLocalWnd(false);
	
	RTCRoom::instance()->stopLocalPreview();
	RTCRoom::instance()->removeRemoteView(m_remoteID.c_str());
}

void DoubleVideoPanel::setRoomCreator(const std::string& id)
{
	m_roomCreator = id;
	if (!whiteBoard && m_whiteboard)
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
	m_menuInfo.mic = mic;
	if (!camera)
	{
		RTCRoom::instance()->stopLocalPreview();
		m_cameraPreview = false;
		showLocalWnd(false);
	}
	else
		showLocalWnd(true);

	ui.dis_main->setMenuInfo(m_menuInfo);
	
	RTCRoom::instance()->setMute(!mic);
}

void DoubleVideoPanel::initConfigSetting(bool whiteboard, bool screenShare, ScreenRecordType screenRecord)
{
	m_RTCShareVideo->setCameraSize(1);

	if (!screenShare)
	{
		m_screenShare = false;
		ui.tabWidget->removeTab(2);
	}
	if (!whiteboard)
	{
		m_whiteboard = false;
		ui.tabWidget->removeTab(1);
	}

	m_screenRecord = screenRecord;
	if (screenRecord != RecordScreenNone)
	{
		m_btn_record_manage->setStyleSheet(m_recordBtnStyle);
		m_btn_record_manage->setText(QStringLiteral("● 00:00:00"));
		m_timerID = startTimer(1000);
	}
}

void DoubleVideoPanel::onSwitch(HWND hwnd, QRect rect, bool bFollowWnd)
{
	shareVideo();
	m_screenArea = true;
	m_areaRect.left = rect.x();
	m_areaRect.top = rect.y();
	m_areaRect.right = rect.x() + rect.width();
	m_areaRect.bottom = rect.y() + rect.height();
	m_shareHwnd = hwnd;
	m_bFollowWnd = bFollowWnd;
	ui.stacked_screen->setCurrentIndex(1);
	m_btn_esc->show();
	showLocalWnd(false);
	RTCRoom::instance()->stopLocalPreview();
	RTCRoom::instance()->startScreenPreview((HWND)ui.widget_screen_share->winId(), m_shareHwnd, RECT{ 0, 0, ui.widget_screen_share->width(), ui.widget_screen_share->height() }, m_areaRect, m_bFollowWnd);
}

void DoubleVideoPanel::onClose()
{
	on_btn_esc_clicked();
}

void DoubleVideoPanel::on_startRecord(ScreenRecordType recordType)
{
	m_recordTime = 0;
	m_timerID = startTimer(1000);
	m_btn_record_manage->setStyleSheet(m_recordBtnStyle);
	m_btn_record_manage->setText(QStringLiteral("● 00:00:00"));

	m_screenRecord = recordType;
}

void DoubleVideoPanel::showEvent(QShowEvent * event)
{
	RECT clientRect;
	static bool init = true;
	m_bInitShowEvent = true;
	if (init)
	{
		resizeUI();

		showLocalWnd(true);
		::GetClientRect(m_hwndLocal, &clientRect);
		RTCRoom::instance()->startLocalPreview(m_hwndLocal, clientRect);
		init = false;
	}
}

void DoubleVideoPanel::keyPressEvent(QKeyEvent * event)
{
	switch (event->key())
	{
	case Qt::Key_Escape:
	{
		if (m_tabIndex == 2 && (/*m_screenFull ||*/ m_screenArea))
		{
			m_RTCShareVideo->hide();
			//m_screenFull = false;
			m_screenArea = false;
			m_cameraPreview = true;
			m_pTXShareFrameMgr->stopShareFrame();
			RTCRoom::instance()->stopScreenPreview();
			ui.stacked_screen->setCurrentIndex(0);

			RECT localRect, remoteRect;
			::GetClientRect(m_hwndLocal, &localRect);
			::GetClientRect(m_hwndRemote, &remoteRect);

			if (m_cameraEnabled)
			{
				showLocalWnd(true);
				RTCRoom::instance()->startLocalPreview(m_hwndLocal, localRect);
			}

			//if (!m_remoteID.empty())
			//{
			//	RTCRoom::instance()->updateRemotePreview(m_hwndRemote, remoteRect, m_remoteID.c_str());
			//}
			m_btn_esc->hide();
		}
	}
	break;
	default:
		QWidget::keyPressEvent(event);
	}
}

void DoubleVideoPanel::timerEvent(QTimerEvent *event)
{
	if (m_timerID == event->timerId())
	{
		m_recordTime++;
		QString str_recordTime = QStringLiteral("● ");
		str_recordTime.append(QDateTime::fromTime_t(m_recordTime).toUTC().toString("hh:mm:ss"));
		m_btn_record_manage->setText(str_recordTime);
	}
}

void DoubleVideoPanel::resizeEvent(QResizeEvent *event)
{
	if (!m_bInitShowEvent)
	{
		return;
	}

	m_bResize = true;
	resizeUI();
	on_tabWidget_currentChanged();
}

void DoubleVideoPanel::initUI()
{
	ui.widget_camera1->hide();
	ui.gridLayout->setRowStretch(0, 0);
	ui.gridLayout->setRowStretch(1, 100);

	winWidgetLocal = new WinWidget;

	widgetLocalDisplay = winWidgetLocal->getWidget();
	m_hwndLocal = winWidgetLocal->getHwnd();
	QHBoxLayout * hLocalLayout = new QHBoxLayout(ui.widget_wnd_local);
	hLocalLayout->setMargin(0);
	hLocalLayout->setAlignment(Qt::AlignLeft);
	hLocalLayout->addWidget(widgetLocalDisplay);
	ui.widget_wnd_local->setLayout(hLocalLayout);

	winWidgetRemote = new WinWidget;
	widgetRemoteDisplay = winWidgetRemote->getWidget();
	m_hwndRemote = winWidgetRemote->getHwnd();
	QHBoxLayout * hRemoteLayout = new QHBoxLayout(ui.widget_wnd);
	hRemoteLayout->setMargin(0);
	hRemoteLayout->setAlignment(Qt::AlignLeft);
	hRemoteLayout->addWidget(widgetRemoteDisplay);
	ui.widget_wnd->setLayout(hRemoteLayout);

	scrollArea_camera = new QScrollArea(ui.widget_camera2);
	scrollArea_camera->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea_camera->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea_camera->setContentsMargins(QMargins(0, 0, 0, 0));
	ui.hLayout_board->addWidget(scrollArea_camera);
	hCameraLayout = new QHBoxLayout(scrollArea_camera);
	hCameraLayout->setMargin(0);
	hCameraLayout->setAlignment(Qt::AlignLeft);
	hCameraLayout->setSpacing(15);
	scrollArea_camera->setLayout(hCameraLayout);

	selfWidget = new RTCVideoWidget(scrollArea_camera, true);
	hCameraLayout->addWidget(selfWidget);
	selfWidget->hide();

	remoteWidget = new RTCVideoWidget(scrollArea_camera);
	hCameraLayout->addWidget(remoteWidget);

	ui.label_tip->setText(QStringLiteral("暂时没有客户登录"));
	//ui.tabWidget->removeTab(2);
	//ui.tabWidget->removeTab(1);
	//ui.btn_area_share->hide();
	widget_tab_corner = new QWidget(this);
	m_btn_record_manage = new QPushButton(QStringLiteral("录制"), widget_tab_corner);
	m_btn_device_manage = new QPushButton(QStringLiteral("设置"), widget_tab_corner);
	m_btn_beauty_manage = new QPushButton(QStringLiteral("美颜"), widget_tab_corner);
	m_btn_esc = new QPushButton(QStringLiteral("结束分享"), widget_tab_corner);
	m_btnStyle =
		R"(
QPushButton {
border-radius: 15px;
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

	m_recordBtnStyle =
		R"(
QPushButton {
	border-radius: 15px;
	border: 1px solid #dddddd;
	background:  #ffffff;
	color: #ff0000;
	font: 9pt "Microsoft YaHei";
}
)";
	const QSize btnSize = QSize(80, 30);
	
	m_btn_record_manage->setStyleSheet(m_btnStyle);
	m_btn_record_manage->setFixedSize(btnSize);

	m_btn_device_manage->setStyleSheet(m_btnStyle);
	m_btn_device_manage->setFixedSize(btnSize);

	m_btn_beauty_manage->setStyleSheet(m_btnStyle);
	m_btn_beauty_manage->setFixedSize(btnSize);

	m_btn_esc->setStyleSheet(m_btnStyle);
	m_btn_esc->setFixedSize(btnSize);

	connect(this, SIGNAL(record_manage_clicked()), RTCMainWindow, SLOT(on_record_manage_clicked()));
	connect(m_btn_record_manage, SIGNAL(clicked()), this, SLOT(on_btn_record_manage_clicked()));
	connect(m_btn_device_manage, SIGNAL(released()), RTCMainWindow, SLOT(on_btn_device_manage_clicked()));
	connect(m_btn_beauty_manage, SIGNAL(released()), RTCMainWindow, SLOT(on_btn_beauty_manage_clicked()));
	connect(m_btn_esc, SIGNAL(released()), this, SLOT(on_btn_esc_clicked()));

	m_btn_esc->hide();
	QHBoxLayout* hLayout = new QHBoxLayout(widget_tab_corner);
	hLayout->addWidget(m_btn_record_manage);
	hLayout->addWidget(m_btn_beauty_manage);
	hLayout->addWidget(m_btn_device_manage);
	hLayout->addWidget(m_btn_esc);
	hLayout->setMargin(0);
	widget_tab_corner->setStyleSheet("margin-left: 5px; margin-right:5px");
	ui.tabWidget->setCornerWidget(widget_tab_corner);
	widget_tab_corner->show();

	showRemoteWnd(false);

	m_RTCShareVideo = new RTCShareVideo;
}

void DoubleVideoPanel::shareVideo()
{
	m_RTCShareVideo->show();
	m_RTCShareVideo->updateUI();
}

void DoubleVideoPanel::showRemoteWnd(bool show)
{
	if (show)
	{
		widgetRemoteDisplay->show();
		ui.widget_wnd->show();
		ui.widget_camera_tip->hide();
		ui.label_mute_main->hide();

		remoteWidget->show();

		SetForegroundWindow(m_hwndRemote);
		SetWindowPos(m_hwndRemote, m_hwndLocal, 0, 0, 0, 0, 3);
	}
	else
	{
		widgetRemoteDisplay->hide();
		ui.widget_wnd->hide();
		ui.widget_camera_tip->show();
		ui.label_mute_main->show();

		remoteWidget->hide();
	}
}

void DoubleVideoPanel::showLocalWnd(bool show)
{
	if (show)
	{
		ui.label_bg->hide();
		ui.label_mute->hide();

		widgetLocalDisplay->show();
		ui.widget_wnd_local->show();

		if (m_tabIndex == 1)
			selfWidget->show();

		SetForegroundWindow(m_hwndLocal);
		SetWindowPos(m_hwndLocal, HWND_TOPMOST, 0, 0, 0, 0, 3);
	}
	else
	{
		ui.label_bg->show();
		if (m_menuInfo.mic)
			ui.label_mute->hide();
		else
			ui.label_mute->show();

		widgetLocalDisplay->hide();
		ui.widget_wnd_local->hide();

		selfWidget->hide();
		selfWidget->stopVideo();
	}

	ui.widget_local->show();
	ui.widget_local->raise();
	ui.widget_local->setWindowFlags(Qt::WindowStaysOnTopHint);
}

void DoubleVideoPanel::resizeUI()
{
	if (m_tabIndex == 2)
	{
		return;
	}
	QSize cameraWidgetSize;
	if (m_tabIndex == 0)
	{
		cameraWidgetSize = QSize(ui.widget_camera1->height() - 22, ui.widget_camera1->height());
		QSize wndSize = QSize(ui.dis_main->width(), ui.dis_main->height());
		ui.widget_wnd->setFixedSize(wndSize);
		widgetRemoteDisplay->setFixedSize(wndSize);
		ui.widget_camera_tip->setFixedSize(wndSize);

		ui.label_mute_main->setGeometry(QRect(ui.dis_main->width() - 30, 0, 30, 30));

		int width = ui.dis_main->width() > 620 ? MAX_LOCAL_WIDTH : NORMAL_LOCAL_WIDTH;
		ui.widget_local->setGeometry(QRect(ui.dis_main->width() - width, ui.dis_main->height() - width, width, width));
		ui.widget_wnd_local->setFixedSize(QSize(ui.widget_local->width(), ui.widget_local->height()));
		widgetLocalDisplay->setFixedSize(QSize(ui.widget_local->width(), ui.widget_local->height()));
		ui.label_bg->setFixedSize(QSize(ui.widget_local->width(), ui.widget_local->height()));
		ui.label_mute->setGeometry(width - 30, 0, 30, 30);
	}
	else if (m_tabIndex == 1)
	{
		cameraWidgetSize = QSize(ui.widget_camera2->height() - 22, ui.widget_camera2->height());
	}

	remoteWidget->setFixedSize(cameraWidgetSize);
	selfWidget->setFixedSize(cameraWidgetSize);

	//m_RTCShareVideo->setWidth(cameraWidgetSize.height() - 22);
}

void DoubleVideoPanel::on_btn_record_manage_clicked()
{
	if (m_screenRecord != RecordScreenNone)
	{
		int ret = DialogMessage::exec(QStringLiteral("是否停止录制?"), DialogMessage::OK | DialogMessage::CANCEL);
		if (ret == DialogMessage::Rejected)
		{
			return;
		}
		killTimer(m_timerID);
		m_timerID = 0;
		m_btn_record_manage->setStyleSheet(m_btnStyle);
		m_recordTime = 0;
		m_btn_record_manage->setText(QStringLiteral("录制"));
		TXCloudRecordCmd::instance().stop();
		m_screenRecord = RecordScreenNone;
	}
	else
	{
		emit record_manage_clicked();
	}
}

void DoubleVideoPanel::on_tabWidget_currentChanged()
{
	if (!whiteBoard && m_whiteboard)
	{
		whiteBoard = new WhiteBoard(ui.widget_board);
		QVBoxLayout * vBoardLayout = new QVBoxLayout(ui.widget_board);
		vBoardLayout->setMargin(0);
		vBoardLayout->addWidget(whiteBoard);
		ui.widget_board->setLayout(vBoardLayout);
		whiteBoard->show();
	}

	m_tabIndex = ui.tabWidget->currentIndex();

	if (m_bResize)
	{
		resizeUI();
	}

	if (!m_whiteboard && m_tabIndex == 1)
	{
		m_tabIndex = 2;
	}
	switch (m_tabIndex)
	{
	case 0:
	{
		m_btn_beauty_manage->show();
		m_btn_device_manage->show();
		m_btn_esc->hide();

		RECT localRect, remoteRect;
		::GetClientRect(m_hwndLocal, &localRect);
		::GetClientRect(m_hwndRemote, &remoteRect);

		if (((/*m_screenFull ||*/ m_screenArea) && m_cameraPreview) || (!m_cameraPreview && m_cameraEnabled))
		{
			m_cameraPreview = true;
			m_pTXShareFrameMgr->stopShareFrame();
			RTCRoom::instance()->stopScreenPreview();			
			RTCRoom::instance()->startLocalPreview(m_hwndLocal, localRect);
		}

		if (!m_remoteID.empty())
		{
			showRemoteWnd(true);
			RTCRoom::instance()->updateRemotePreview(m_hwndRemote, remoteRect, m_remoteID.c_str());
		}

		if (m_cameraPreview && m_cameraEnabled)
		{
			showLocalWnd(true);
			RTCRoom::instance()->updateLocalPreview(m_hwndLocal, localRect);
		}

		selfWidget->hide();

		m_RTCShareVideo->hide();
	}
	break;
	case 1:
	{
		m_btn_beauty_manage->show();
		m_btn_device_manage->show();
		m_btn_esc->hide();
		selfWidget->show();

		if (((/*m_screenFull ||*/ m_screenArea) && m_cameraPreview) || (!m_cameraPreview && m_cameraEnabled))
		{
			m_cameraPreview = true;
			m_pTXShareFrameMgr->stopShareFrame();
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

		m_RTCShareVideo->hide();
	}
	break;
	case 2:
	{
		m_btn_beauty_manage->hide();
		m_btn_device_manage->hide();

		if (!m_screenArea /*&& !m_screenFull*/)
			break;
		m_btn_esc->show();

		RTCRoom::instance()->stopLocalPreview();
		/*if (m_screenFull)
			RTCRoom::instance()->startScreenPreview((HWND)ui.widget_screen_share->winId(), nullptr, RECT{ 0, 0, ui.widget_screen_share->width(), ui.widget_screen_share->height() }, RECT{ 0 }, m_bFollowWnd);
		else*/ 
		if (m_screenArea) {
			shareVideo();
			m_pTXShareFrameMgr->reTrackFrame(m_shareHwnd, QRect(m_areaRect.left, m_areaRect.top, m_areaRect.right - m_areaRect.left, m_areaRect.bottom - m_areaRect.top));
			RTCRoom::instance()->startScreenPreview((HWND)ui.widget_screen_share->winId(), m_shareHwnd, RECT{ 0, 0, ui.widget_screen_share->width(), ui.widget_screen_share->height() }, m_areaRect, m_bFollowWnd);
	}
	}
	break;
	default:
		break;
	}
}

void DoubleVideoPanel::on_btn_area_share_clicked()
{
	m_pTXShareFrameMgr->areaShareFrame();
}

void DoubleVideoPanel::on_btn_full_share_clicked()
{
	m_pTXShareFrameMgr->fullShareFrame();
}

void DoubleVideoPanel::on_btn_esc_clicked()
{
	m_RTCShareVideo->hide();
	m_btn_esc->hide();
	//m_screenFull = false;
	m_screenArea = false;
	m_cameraPreview = true;
	m_pTXShareFrameMgr->stopShareFrame();
	RTCRoom::instance()->stopScreenPreview();
	ui.stacked_screen->setCurrentIndex(0);

	RECT localRect, remoteRect;
	::GetClientRect(m_hwndLocal, &localRect);
	::GetClientRect(m_hwndRemote, &remoteRect);

	if (m_cameraEnabled)
	{
		showLocalWnd(true);
		RTCRoom::instance()->startLocalPreview(m_hwndLocal, localRect);
	}
}