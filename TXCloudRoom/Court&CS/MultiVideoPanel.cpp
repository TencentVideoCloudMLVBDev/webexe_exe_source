#include "MultiVideoPanel.h"
#include "CaptureScreen.h"
#include <QScrollBar>
#include <QKeyEvent>
#include <QSettings>
#include "DialogMessage.h"
#include <QDateTime>

extern QWidget * RTCMainWindow;

MultiVideoPanel::MultiVideoPanel(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	initUI();

	connect(ui.dis_main, SIGNAL(actCamera(bool)), this, SLOT(on_dis_actCamera(bool)));
	connect(ui.dis_main, SIGNAL(actMic(bool)), this, SLOT(on_dis_actMic(bool)));
	connect(this, SIGNAL(actCamera(bool)), this, SLOT(on_dis_actCamera(bool)));
	connect(this, SIGNAL(actMic(bool)), this, SLOT(on_dis_actMic(bool)));
	connect(selfWidget, SIGNAL(local_actCamera(bool)), this, SLOT(on_local_actCamera(bool)));
	connect(selfWidget, SIGNAL(local_actMic(bool)), this, SLOT(on_dis_actMic(bool)));

	m_pTXShareFrameMgr = new TXShareFrameMgr(this, nullptr);
}

MultiVideoPanel::~MultiVideoPanel()
{
	RTCRoom::instance()->stopLocalPreview();
	if (m_pTXShareFrameMgr)
	{
		delete m_pTXShareFrameMgr;
		m_pTXShareFrameMgr = nullptr;
	}
}

void MultiVideoPanel::initShowVideo()
{
	showLocalWnd(true);
}

void MultiVideoPanel::on_menu_actCamera(bool open)
{
	m_menuInfo.camera = open;
	emit actCamera(open);
}

void MultiVideoPanel::on_menu_actMic(bool open)
{
	m_menuInfo.mic = open;
	emit actMic(open);
}

MenuInfo & MultiVideoPanel::getMenuInfo()
{
	return m_menuInfo;
}

void MultiVideoPanel::on_startRecord(ScreenRecordType recordType)
{
	m_recordTime = 0;
	m_timerID = startTimer(1000);
	m_btn_record_manage->setStyleSheet(m_recordBtnStyle);
	m_btn_record_manage->setText(QStringLiteral("● 00:00:00"));

	m_screenRecord = recordType;
}

void MultiVideoPanel::onPusherJoin(const MemberItem& member)
{
	RTCVideoWidget * cameraWidget;

	bool idle = false;
	for (int i = 0; i < m_vCameraWidgets.size(); i++)
	{
		if (m_vCameraWidgets[i]->idle == true)
		{
			idle = true;
			m_vCameraWidgets[i]->idle = false;
			m_vCameraWidgets[i]->userID = member.userID;
			cameraWidget = m_vCameraWidgets[i]->cameraWidget;
			break;
		}
	}
	if (!idle)
	{
		cameraWidget = new RTCVideoWidget(scrollArea_camera);
		RTCCameraWidget * pusherCameraWidget = new RTCCameraWidget;
		pusherCameraWidget->cameraWidget = cameraWidget;
		pusherCameraWidget->idle = false;
		pusherCameraWidget->userID = member.userID;
		hCameraLayout->addWidget(cameraWidget);
		m_vCameraWidgets.push_back(pusherCameraWidget);
	}

	cameraWidget->show();
	cameraWidget->setUserName(member.userName);
	cameraWidget->setUserID(member.userID);

	if (m_tabIndex != 2)
	{
		cameraWidget->startVideo(member.userID);
		m_RTCShareVideo->setUserInfo(member.userName, member.userID, true, false);
	}
	else
		m_RTCShareVideo->setUserInfo(member.userName, member.userID, true, true);
}

void MultiVideoPanel::onPusherQuit(const MemberItem& member)
{
	RTCRoom::instance()->removeRemoteView(member.userID.c_str());

	for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end(); iter++)
	{
		if ((*iter)->userID == member.userID.c_str())
		{
			(*iter)->cameraWidget->stopVideo();
			hCameraLayout->removeWidget((*iter)->cameraWidget);
			delete (*iter)->cameraWidget;
			delete (*iter);
			m_vCameraWidgets.erase(iter);
			ui.widget_camera1->update();
			ui.widget_camera2->update();
			break;
		}
	}
	initCameraWidget();
	m_RTCShareVideo->setUserInfo(member.userName, member.userID, false, false);
}

void MultiVideoPanel::onRoomClosed()
{
	for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end();)
	{
		RTCRoom::instance()->removeRemoteView((*iter)->userID.c_str());

		(*iter)->cameraWidget->stopVideo();
		hCameraLayout->removeWidget((*iter)->cameraWidget);
		delete (*iter)->cameraWidget;
		delete (*iter);
		iter = m_vCameraWidgets.erase(iter);
		ui.widget_camera1->update();
		ui.widget_camera2->update();
	}

	showLocalWnd(false);
	RTCRoom::instance()->stopLocalPreview();
}

void MultiVideoPanel::setRoomCreator(const std::string& id)
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

void MultiVideoPanel::setUserInfo(const std::string& id, const std::string& userName)
{
	m_userID = id;
	m_userName = userName;
	selfWidget->setUserName(userName);
	//m_RTCShareVideo->setUserInfo(userName, "", true, false);
}

int MultiVideoPanel::getVideoCount()
{
	int count = 0;
	for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end(); iter++)
	{
		if ((*iter)->idle == false)
		{
			count++;
		}
	}
	return count;
}

void MultiVideoPanel::updatePreview()
{
	on_tabWidget_currentChanged();
}

void MultiVideoPanel::setDeviceEnabled(bool camera, bool mic)
{
	m_cameraEnabled = camera;
	if (!camera)
	{
		RTCRoom::instance()->stopLocalPreview();
		m_cameraPreview = false;
		showLocalWnd(false);
	}
	else
		showLocalWnd(true);

	on_dis_actMic(mic);
}

void MultiVideoPanel::initConfigSetting(int size, bool whiteboard, bool screenShare, ScreenRecordType screenRecord)
{
	m_cameraSize = size;
	initCameraWidget();
	m_RTCShareVideo->setCameraSize(m_cameraSize);

	if (!screenShare)
	{
		ui.tabWidget->removeTab(2);
	}
	if (!whiteboard)
	{
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

void MultiVideoPanel::showEvent(QShowEvent * event)
{
	static bool init = true;
	RECT clientRect;
	m_bInitShowEvent = true;
	if (init)
	{
		resizeUI();
		showLocalWnd(true);
		::GetClientRect(m_hwnd, &clientRect);
		RTCRoom::instance()->startLocalPreview(m_hwnd, clientRect);
		init = false;
	}
}

void MultiVideoPanel::timerEvent(QTimerEvent *event)
{
	if (m_timerID == event->timerId())
	{
		m_recordTime++;
		QString str_recordTime = QStringLiteral("● ");
		str_recordTime.append(QDateTime::fromTime_t(m_recordTime).toUTC().toString("hh:mm:ss"));
		m_btn_record_manage->setText(str_recordTime);
	}
}

void MultiVideoPanel::resizeEvent(QResizeEvent *event)
{
	if (!m_bInitShowEvent)
	{
		return;
	}

	m_bResize = true;
	resizeUI();
	for (int i = 0; i < m_vCameraWidgets.size(); i++)
	{
		if (m_vCameraWidgets[i]->idle == false)
		{
			m_vCameraWidgets[i]->cameraWidget->updatePreview();
		}
	}
	on_tabWidget_currentChanged();
}

void MultiVideoPanel::onSwitch(HWND hwnd, QRect rect, bool bFollowWnd)
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

void MultiVideoPanel::onClose()
{
	on_btn_esc_clicked();
}

void MultiVideoPanel::keyPressEvent(QKeyEvent * event)
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
			m_shareHwnd = false;
			m_pTXShareFrameMgr->stopShareFrame();
			RTCRoom::instance()->stopScreenPreview();
			ui.stacked_screen->setCurrentIndex(0);
			RECT clientRect;
			::GetClientRect(m_hwnd, &clientRect);

			m_cameraPreview = true;

			if (m_cameraEnabled)
			{
				showLocalWnd(true);
				RTCRoom::instance()->startLocalPreview(m_hwnd, clientRect);
			}
		}

		m_btn_esc->hide();
	}
		break;
	default:
		QWidget::keyPressEvent(event);
	}
}

void MultiVideoPanel::initUI()
{
	winWidget = new WinWidget;

	widgetDisplay = winWidget->getWidget();
	m_hwnd = winWidget->getHwnd();
	QHBoxLayout * hDismainLayout = new QHBoxLayout(ui.widget_wnd);

	SetProp(m_hwnd, L"MultiVideoPanel", this);
	//::SetParent(m_hwnd, (HWND)ui.widget_wnd->winId());
	hDismainLayout->setMargin(0);
	hDismainLayout->setAlignment(Qt::AlignLeft);
	hDismainLayout->addWidget(widgetDisplay);
	ui.widget_wnd->setLayout(hDismainLayout);
	ui.widget_camera_tip->setFixedHeight(ui.dis_main->height());

	ui.widget_local->hide();

	scrollArea_camera = new QScrollArea(ui.widget_camera1);
	scrollArea_camera->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea_camera->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea_camera->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	scrollArea_camera->setWidgetResizable(true);

	scrollArea_camera->setContentsMargins(QMargins(0,0,0,0));
	ui.hLayout_camera->addWidget(scrollArea_camera);
	hCameraLayout = new QHBoxLayout(scrollArea_camera);
	hCameraLayout->setMargin(0);
	hCameraLayout->setAlignment(Qt::AlignLeft);
	hCameraLayout->setSpacing(15);
	scrollArea_camera->show();

	QWidget* widget = new QWidget(scrollArea_camera);
	widget->setLayout(hCameraLayout);
	
	scrollArea_camera->setWidget(widget);
	widget->show();

	const QString scrollBarStyle =
		R"(QScrollBar{
    background: transparent;
	 height: 7px;
}

QScrollBar::handle {
    background-color: #dbdbdb;
    border-radius: 3px;
}

QScrollBar::handle:hover {
    background-color: #dfdfdf;
}

QScrollBar::handle:pressed {
    background-color: #cccccc;
}

QScrollBar::add-line, QScrollBar::sub-line {
    background: transparent;
    height: 0px;
    width: 0px;
}

QScrollBar::add-page, QScrollBar::sub-page {
    background: transparent;
}

QScrollBar::up-arrow, QScrollBar::down-arrow {
    background: transparent;
    height: 0px;
    width: 0px;
})";

	scrollArea_camera->horizontalScrollBar()->setStyleSheet(scrollBarStyle);

	selfWidget = new RTCVideoWidget(scrollArea_camera, true);
	hCameraLayout->addWidget(selfWidget);
	selfWidget->hide();

	m_menuInfo.camera = true;
	m_menuInfo.mic = true;
	m_menuInfo.mainDis = true;
	ui.dis_main->setMenuInfo(m_menuInfo);

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
	connect(m_btn_record_manage, SIGNAL(released()), this, SLOT(on_btn_record_manage_clicked()));
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

	m_RTCShareVideo = new RTCShareVideo;
	m_RTCShareVideo->hide();
}

void MultiVideoPanel::initCameraWidget()
{
	int createSize = m_cameraSize - m_vCameraWidgets.size();
	if (createSize <= 0)
	{
		return;
	}
	for (int i = 0; i < createSize; i++)
	{
		RTCVideoWidget * cameraWidget = new RTCVideoWidget(scrollArea_camera);
		RTCCameraWidget * pusherCameraWidget = new RTCCameraWidget;
		pusherCameraWidget->cameraWidget = cameraWidget;
		pusherCameraWidget->idle = true;
		pusherCameraWidget->userID = "";
		hCameraLayout->addWidget(cameraWidget);
		m_vCameraWidgets.push_back(pusherCameraWidget);
	}
}

void MultiVideoPanel::showLocalWnd(bool show)
{
	if (show)
	{
		ui.widget_camera_tip->hide();
		ui.label_mute_main->hide();
		if (m_tabIndex == 1)
			selfWidget->show();
		
		ui.widget_wnd->show();
		widgetDisplay->show();
	}
	else
	{
		ui.widget_camera_tip->show();
		if (m_menuInfo.mic)
			ui.label_mute_main->hide();
		else
			ui.label_mute_main->show();
		selfWidget->hide();
		selfWidget->stopVideo();
		ui.widget_wnd->hide();
		widgetDisplay->hide();
	}
}

void MultiVideoPanel::resizeUI()
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
		widgetDisplay->setFixedSize(wndSize);
		ui.widget_camera_tip->setFixedSize(wndSize);

		ui.label_mute_main->setGeometry(QRect(ui.dis_main->width() - 30, 0, 30, 30));
	}
	else if (m_tabIndex == 1)
	{
		cameraWidgetSize = QSize(ui.widget_camera2->height() - 22, ui.widget_camera2->height());
	}

	for (int i = 0; i < m_vCameraWidgets.size(); i++)
	{
		m_vCameraWidgets[i]->cameraWidget->setFixedSize(cameraWidgetSize);
	}
	selfWidget->setFixedSize(cameraWidgetSize);

	//m_RTCShareVideo->setWidth(cameraWidgetSize.height() - 22);
}

void MultiVideoPanel::shareVideo()
{
	m_RTCShareVideo->show();
	m_RTCShareVideo->updateUI();

	m_bShareVideo = true;
}

void MultiVideoPanel::on_dis_actCamera(bool open)
{
	m_menuInfo.camera = open;
	ui.dis_main->setMenuInfo(m_menuInfo);
	selfWidget->setMenuInfo(m_menuInfo);

	RECT clientRect;
	::GetClientRect(m_hwnd, &clientRect);
	if (open)
	{
		m_cameraPreview = true;
		m_cameraEnabled = true;
		showLocalWnd(true);
		RTCRoom::instance()->startLocalPreview(m_hwnd, clientRect);
	}
	else
	{
		m_cameraPreview = false;
		m_cameraEnabled = false;
		showLocalWnd(false);
		RTCRoom::instance()->stopLocalPreview();
	}
}

void MultiVideoPanel::on_dis_actMic(bool open)
{
	m_menuInfo.mic = open;
	ui.dis_main->setMenuInfo(m_menuInfo);
	selfWidget->setMenuInfo(m_menuInfo);

	if (open)
		ui.label_mute_main->hide();
	else
		ui.label_mute_main->show();

	RTCRoom::instance()->setMute(!open);
}

void MultiVideoPanel::on_local_actCamera(bool open)
{
	m_menuInfo.camera = open;
	ui.dis_main->setMenuInfo(m_menuInfo);

	if (open)
	{
		showLocalWnd(true);
		m_cameraPreview = true;
		m_cameraEnabled = true;
	}
	else
	{
		showLocalWnd(false);
		m_cameraPreview = false;
		m_cameraEnabled = false;
	}
}

void MultiVideoPanel::on_selectCaptureArea(QRect rect)
{
	shareVideo();
	m_screenArea = true;
	m_areaRect.left = rect.x();
	m_areaRect.top = rect.y();
	m_areaRect.right = rect.x() + rect.width();
	m_areaRect.bottom = rect.y() + rect.height();
	ui.stacked_screen->setCurrentIndex(1);
	m_btn_esc->show();
	showLocalWnd(false);
	RTCRoom::instance()->stopLocalPreview();
	RTCRoom::instance()->startScreenPreview((HWND)ui.widget_screen_share->winId(), nullptr, RECT{ 0, 0, ui.widget_screen_share->width(), ui.widget_screen_share->height() }, m_areaRect, m_bFollowWnd);
}

void MultiVideoPanel::on_btn_record_manage_clicked()
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

void MultiVideoPanel::on_tabWidget_currentChanged()
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

	if (m_bResize)
	{
		resizeUI();
	}

	RECT clientRect;
	::GetClientRect(m_hwnd, &clientRect);

	switch (m_tabIndex)
	{
	case 0:
	{
		m_btn_beauty_manage->show();
		m_btn_device_manage->show();
		m_btn_esc->hide();

		ui.hLayout_board->removeWidget(scrollArea_camera);
		scrollArea_camera->setParent(ui.widget_camera1);
		ui.hLayout_camera->addWidget(scrollArea_camera);

		if (((/*m_screenFull ||*/ m_screenArea) && m_cameraPreview) || (!m_cameraPreview && m_cameraEnabled))
		{
			m_cameraPreview = true;
			showLocalWnd(true);
			m_pTXShareFrameMgr->stopShareFrame();
			RTCRoom::instance()->stopScreenPreview();
			RTCRoom::instance()->startLocalPreview(m_hwnd, clientRect);
		}
		if (m_cameraPreview && m_cameraEnabled)
		{
			showLocalWnd(true);
			RTCRoom::instance()->updateLocalPreview(m_hwnd, clientRect);
		}
		selfWidget->hide();
		m_RTCShareVideo->hide();
		if (m_bShareVideo)
		{
			for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end(); iter++)
			{
				if (!(*iter)->idle)
				{
					(*iter)->cameraWidget->updatePreview();
				}
			}
			m_bShareVideo = false;
		}
	}
	break;
	case 1:
	{
		m_btn_beauty_manage->show();
		m_btn_device_manage->show();
		m_btn_esc->hide();
		selfWidget->show();

		ui.hLayout_camera->removeWidget(scrollArea_camera);
		scrollArea_camera->setParent(ui.widget_camera2);
		ui.hLayout_board->addWidget(scrollArea_camera);

		if (((/*m_screenFull ||*/ m_screenArea) && m_cameraPreview) || (!m_cameraPreview && m_cameraEnabled))
		{
			m_cameraPreview = true;
			m_pTXShareFrameMgr->stopShareFrame();
			RTCRoom::instance()->stopScreenPreview();
			RTCRoom::instance()->startLocalPreview(m_hwnd, clientRect);
		}

		selfWidget->setMenuInfo(m_menuInfo);
		if (m_cameraPreview && m_cameraEnabled)
		{
			selfWidget->startVideo(m_userID);
		}

		m_RTCShareVideo->hide();
		if (m_bShareVideo)
		{
			for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end(); iter++)
			{
				if (!(*iter)->idle)
				{
					(*iter)->cameraWidget->updatePreview();
				}
			}
			m_bShareVideo = false;
		}
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

void MultiVideoPanel::on_btn_area_share_clicked()
{
	m_pTXShareFrameMgr->areaShareFrame();
}

void MultiVideoPanel::on_btn_full_share_clicked()
{
	m_pTXShareFrameMgr->fullShareFrame();
}

void MultiVideoPanel::on_btn_esc_clicked()
{
	m_RTCShareVideo->hide();
	m_btn_esc->hide();
	//m_screenFull = false;
	m_screenArea = false;
	m_shareHwnd = false;
	m_pTXShareFrameMgr->stopShareFrame();
	RTCRoom::instance()->stopScreenPreview();
	ui.stacked_screen->setCurrentIndex(0);
	RECT clientRect;
	::GetClientRect(m_hwnd, &clientRect);

	m_cameraPreview = true;

	if (m_cameraEnabled)
	{
		showLocalWnd(true);
		RTCRoom::instance()->startLocalPreview(m_hwnd, clientRect);
	}
}
