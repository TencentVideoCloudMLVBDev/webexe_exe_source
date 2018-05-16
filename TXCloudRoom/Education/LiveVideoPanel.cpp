#include "LiveVideoPanel.h"
#include "CaptureScreen.h"
#include "LiveRoom.h"
#include "LiveRoomUtil.h"
#include <QScrollBar>

extern QWidget * LiveMainWindow;

LiveVideoPanel::LiveVideoPanel(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	initUI();

	connect(ui.dis_main, SIGNAL(actCamera(bool)), this, SLOT(on_dis_actCamera(bool)));
	connect(ui.dis_main, SIGNAL(actMic(bool)), this, SLOT(on_dis_actMic(bool)));
	connect(selfWidget, SIGNAL(local_actCamera(bool)), this, SLOT(on_local_actCamera(bool)));
	connect(selfWidget, SIGNAL(local_actMic(bool)), this, SLOT(on_dis_actMic(bool)));
}

LiveVideoPanel::~LiveVideoPanel()
{
	LiveRoom::instance()->stopLocalPreview();
}

void LiveVideoPanel::initShowVideo()
{
	ui.widget_camera_tip->hide();
	ui.label_mute_main->hide();
}

void LiveVideoPanel::onPusherJoin(const MemberItem& member)
{
	LiveVideoWidget * cameraWidget;

	bool idle = false;
	for (int i = 0;i<m_vCameraWidgets.size();i++)
	{
		if (m_vCameraWidgets[i]->idle==true)
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
		cameraWidget = new LiveVideoWidget(scrollArea_camera);
		LiveCameraWidget * pusherCameraWidget = new LiveCameraWidget;
		pusherCameraWidget->cameraWidget = cameraWidget;
		pusherCameraWidget->idle = false;
		pusherCameraWidget->userID = member.userID;
		hCameraLayout->addWidget(cameraWidget);
		m_vCameraWidgets.push_back(pusherCameraWidget);
	}
	
	cameraWidget->show();
	cameraWidget->startVideo(member.userID);
	cameraWidget->setUserName(member.userName);
}

void LiveVideoPanel::onPusherQuit(const MemberItem& member)
{
	LiveRoom::instance()->removeRemoteView(member.userID.c_str());

	for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end(); iter++)
	{
		if ((*iter)->userID == member.userID)
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
}

void LiveVideoPanel::onRoomClosed()
{
	for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end();)
    {
		LiveRoom::instance()->removeRemoteView((*iter)->userID.c_str());

		(*iter)->cameraWidget->stopVideo();
		hCameraLayout->removeWidget((*iter)->cameraWidget);
		delete (*iter)->cameraWidget;
		delete (*iter);
		iter = m_vCameraWidgets.erase(iter);
		ui.widget_camera1->update();
        ui.widget_camera2->update();
    }

    selfWidget->hide();
	LiveRoom::instance()->stopLocalPreview();
	ui.widget_camera_tip->show();
	ui.label_mute_main->show();
}

void LiveVideoPanel::setRoomCreator(const std::string & id)
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

void LiveVideoPanel::setUserInfo(const std::string & id, const std::string & userName)
{
	m_userID = id;
	m_userName = userName;
	selfWidget->setUserName(userName);
}

int LiveVideoPanel::getVideoCount()
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

void LiveVideoPanel::updatePreview()
{
	on_tabWidget_currentChanged();
}

void LiveVideoPanel::setDeviceEnabled(bool camera, bool mic)
{
	m_cameraEnabled = camera;
	if (!camera)
	{
		LiveRoom::instance()->stopLocalPreview();
		m_cameraPreview = false;
		ui.widget_camera_tip->show();
	}
	else
		ui.widget_camera_tip->hide();

	on_dis_actMic(mic);
}

void LiveVideoPanel::initConfigSetting(int size, bool whiteboard, bool screenShare)
{
	m_cameraSize = size;
	initCameraWidget();
	if (!screenShare)
	{
		ui.tabWidget->removeTab(2);
	}
	if (!whiteboard)
	{
		ui.tabWidget->removeTab(1);
	}
}

void LiveVideoPanel::keyPressEvent(QKeyEvent * event)
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

			LiveRoom::instance()->stopScreenPreview();
			ui.stacked_screen->setCurrentIndex(0);

            LiveRoom::instance()->setVideoQuality(LIVEROOM_VIDEO_QUALITY_HIGH_DEFINITION, LIVEROOM_ASPECT_RATIO_4_3);
            LiveRoom::instance()->startLocalPreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() });

            if (m_cameraPreview)
                LiveRoom::instance()->updateLocalPreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() });
		}
	}
		break;
	default:
		QWidget::keyPressEvent(event);
	}
}

void LiveVideoPanel::showEvent(QShowEvent * event)
{
	static bool init = true;
	if (init)
	{
		LiveRoom::instance()->startLocalPreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() });
		init = false;
	}
}

void LiveVideoPanel::initUI()
{
	ui.widget_local->hide();

	scrollArea_camera = new QScrollArea(ui.widget_camera1);
	scrollArea_camera->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea_camera->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea_camera->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	scrollArea_camera->setWidgetResizable(true);

	scrollArea_camera->setContentsMargins(QMargins(0, 0, 0, 0));
	ui.hLayout_camera->addWidget(scrollArea_camera);
	hCameraLayout = new QHBoxLayout(scrollArea_camera);
	hCameraLayout->setContentsMargins(QMargins(4,5,4,5));
	hCameraLayout->setAlignment(Qt::AlignLeft);
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

	selfWidget = new LiveVideoWidget(scrollArea_camera, true);
	hCameraLayout->addWidget(selfWidget);
	selfWidget->hide();

	m_menuInfo.camera = true;
	m_menuInfo.mic = true;
	m_menuInfo.mainDis = true;
	ui.dis_main->setMenuInfo(m_menuInfo);

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

	connect(btn_device_manage, SIGNAL(released()), LiveMainWindow, SLOT(on_btn_device_manage_clicked()));
	connect(btn_beauty_manage, SIGNAL(released()), LiveMainWindow, SLOT(on_btn_beauty_manage_clicked()));

	QHBoxLayout* hLayout = new QHBoxLayout(widget_tab_corner);
	hLayout->addWidget(btn_beauty_manage);
	hLayout->addWidget(btn_device_manage);
	hLayout->setMargin(0);
	widget_tab_corner->setStyleSheet("margin-top: 2");
	ui.tabWidget->setCornerWidget(widget_tab_corner);
	widget_tab_corner->show();
}

void LiveVideoPanel::initCameraWidget()
{
	int createSize = m_cameraSize - m_vCameraWidgets.size();
	if (createSize<=0)
	{
		return;
	}
	for (int i = 0; i< createSize; i++)
	{
		LiveVideoWidget * cameraWidget = new LiveVideoWidget(scrollArea_camera);
		LiveCameraWidget * pusherCameraWidget = new LiveCameraWidget;
		pusherCameraWidget->cameraWidget = cameraWidget;
		pusherCameraWidget->idle = true;
		pusherCameraWidget->userID = "";
		hCameraLayout->addWidget(cameraWidget);
		m_vCameraWidgets.push_back(pusherCameraWidget);
	}
}

void LiveVideoPanel::on_dis_actCamera(bool open)
{
	m_menuInfo.camera = open;

	if (open)
	{
		m_cameraPreview = true;
        LiveRoom::instance()->setVideoQuality(LIVEROOM_VIDEO_QUALITY_HIGH_DEFINITION, LIVEROOM_ASPECT_RATIO_4_3);
		LiveRoom::instance()->startLocalPreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() });
	}
	else
	{
		m_cameraPreview = false;
		LiveRoom::instance()->stopLocalPreview();
	}
}

void LiveVideoPanel::on_dis_actMic(bool open)
{
	m_menuInfo.mic = open;
	ui.dis_main->setMenuInfo(m_menuInfo);

	if (open)
		ui.label_mute_main->hide();
	else
		ui.label_mute_main->show();

	LiveRoom::instance()->setMute(!open);
}

void LiveVideoPanel::on_local_actCamera(bool open)
{
	m_menuInfo.camera = open;
	ui.dis_main->setMenuInfo(m_menuInfo);

	if (open)
	{
		ui.widget_camera_tip->hide();
		m_cameraPreview = true;
	}
	else
	{
		ui.widget_camera_tip->show();
		m_cameraPreview = false;
	}
}

void LiveVideoPanel::on_selectCaptureArea(QRect rect)
{
	m_screenArea = true;
	m_areaRect.left = rect.x();
	m_areaRect.top = rect.y();
	m_areaRect.right = rect.x() + rect.width();
	m_areaRect.bottom = rect.y() + rect.height();
	ui.stacked_screen->setCurrentIndex(1);

	LiveRoom::instance()->stopLocalPreview();
	LiveRoom::instance()->startScreenPreview((HWND)ui.page_screen_content->winId(), nullptr, RECT{ 0, 0, ui.page_screen_content->width(), ui.page_screen_content->height() }, m_areaRect);
}

void LiveVideoPanel::on_tabWidget_currentChanged()
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
		ui.hLayout_board->removeWidget(scrollArea_camera);
		scrollArea_camera->setParent(ui.widget_camera1);
		ui.hLayout_camera->addWidget(scrollArea_camera);
		selfWidget->hide();

		if (((m_screenFull || m_screenArea) && m_cameraPreview ) || (!m_cameraPreview && m_cameraEnabled))
		{
			m_cameraPreview = true;
			LiveRoom::instance()->stopScreenPreview();

            LiveRoom::instance()->setVideoQuality(LIVEROOM_VIDEO_QUALITY_HIGH_DEFINITION, LIVEROOM_ASPECT_RATIO_4_3);
			LiveRoom::instance()->startLocalPreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() });
		}
		if (m_cameraPreview)
			LiveRoom::instance()->updateLocalPreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() });
	}
	break;
	case 1:
	{
		widget_tab_corner->hide();
		ui.hLayout_camera->removeWidget(scrollArea_camera);
		scrollArea_camera->setParent(ui.widget_camera2);
		ui.hLayout_board->addWidget(scrollArea_camera);
		if (((m_screenFull || m_screenArea) && m_cameraPreview) || (!m_cameraPreview && m_cameraEnabled))
		{
			m_cameraPreview = true;
			LiveRoom::instance()->stopScreenPreview();
            LiveRoom::instance()->setVideoQuality(LIVEROOM_VIDEO_QUALITY_HIGH_DEFINITION, LIVEROOM_ASPECT_RATIO_4_3);
			LiveRoom::instance()->startLocalPreview((HWND)ui.dis_main->winId(), RECT{ 0, 0, ui.dis_main->width(), ui.dis_main->height() });
		}
		selfWidget->show();
		selfWidget->setMenuInfo(m_menuInfo);
		if (m_cameraPreview)
			selfWidget->startVideo(m_userID);
	}
	break;
	case 2:
	{
		widget_tab_corner->hide();
		if (!m_screenArea && !m_screenFull)
			break;

		LiveRoom::instance()->stopLocalPreview();
		if (m_screenFull)
			LiveRoom::instance()->startScreenPreview((HWND)ui.page_screen_content->winId(), nullptr, RECT{ 0, 0, ui.page_screen_content->width(), ui.page_screen_content->height() }, RECT{ 0 });
		else if (m_screenArea)
			LiveRoom::instance()->startScreenPreview((HWND)ui.page_screen_content->winId(), nullptr, RECT{ 0, 0, ui.page_screen_content->width(), ui.page_screen_content->height() }, m_areaRect);
	}
	break;
	default:
		break;
	}
}

void LiveVideoPanel::on_btn_area_share_clicked()
{
	CaptureScreen* captureHelper = new CaptureScreen();
	connect(captureHelper, SIGNAL(signalSelectRect(QRect)), this, SLOT(on_selectCaptureArea(QRect)));
	captureHelper->show();
}

void LiveVideoPanel::on_btn_full_share_clicked()
{
	m_screenFull = true;
	ui.stacked_screen->setCurrentIndex(1);
	LiveRoom::instance()->stopLocalPreview();
	LiveRoom::instance()->startScreenPreview((HWND)ui.page_screen_content->winId(), nullptr, RECT{ 0, 0, ui.page_screen_content->width(), ui.page_screen_content->height() }, RECT{ 0 });
}