#include "RTCDemo.h"
#include "DialogMessage.h"
#include "BoardService.h"
#include "RTCRoom.h"
#include "RTCRoomUtil.h"
#include "RTCRHttpRequest.h"
#include "Application.h"
#include "Base.h"

#include <Dwmapi.h> 

QWidget * RTCMainWindow = nullptr;
#define PANEL_WIDTH 200
#define DEFAULT_CAMERA_SIZE 4

RTCDemo::RTCDemo(QWidget *parent)
    : QMainWindow(parent)
    , m_members()
    , m_imgDownloader()
{
    connect(&m_imgDownloader, SIGNAL(downloadFinished(bool, QByteArray)), this, SLOT(onDownloadFinished(bool, QByteArray)));

	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint | windowFlags());

	BOOL bEnable = false;
	::DwmIsCompositionEnabled(&bEnable);
	if (bEnable)
	{
		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
		::DwmSetWindowAttribute((HWND)winId(), DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
		MARGINS margins = { -1 };
		::DwmExtendFrameIntoClientArea((HWND)winId(), &margins);
	}

	RTCMainWindow = this;
	m_demoWidth = this->geometry().width();

	qRegisterMetaType<txfunction>("txfunction");
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);

	RTCRoom::instance()->setCallback(this);

	ui.label_logo->hide();
	ui.label_title->hide();
}

void RTCDemo::mousePressEvent(QMouseEvent *e)
{
	mousePressedPosition = e->globalPos();
	windowPositionAsDrag = pos();
}

void RTCDemo::mouseReleaseEvent(QMouseEvent *e)
{
	Q_UNUSED(e)
		mousePressedPosition = QPoint();
}

void RTCDemo::mouseMoveEvent(QMouseEvent *e)
{
	if (!mousePressedPosition.isNull()) {
		QPoint delta = e->globalPos() - mousePressedPosition;
		move(windowPositionAsDrag + delta);
	}
}

void RTCDemo::showEvent(QShowEvent * event)
{
	this->setAttribute(Qt::WA_Mapped);
	QMainWindow::showEvent(event);
}

void RTCDemo::onDownloadFinished(bool success, QByteArray image)
{
    QPixmap pixmap;
    pixmap.loadFromData(image);
    ui.label_logo->setPixmap(pixmap);
}

void RTCDemo::on_btn_close_clicked()
{
	if (m_screenRecord != RecordScreenNone)
	{
		HWND recordHwnd = FindWindowExA(HWND_MESSAGE, NULL, "TXCloudRecord", "TXCloudRecordCaption");
		if (recordHwnd)
		{
			std::string message = "RecordExit";
			COPYDATASTRUCT copy_data = { ScreenRecordExit, message.length() + 1,(LPVOID)message.c_str() };
			::SendMessage(recordHwnd, WM_COPYDATA, ScreenRecordExit, reinterpret_cast<LPARAM>(&copy_data));
		}
	}

    Application::instance().pushRoomStatus(2, Wide2UTF8(L"退出房间"));
	BoardService::instance().reportELK();

	RTCRoom::instance()->leaveRoom();
	RTCRoom::instance()->logout();

	this->close();
	Application::instance().quit(0);
}

void RTCDemo::on_btn_min_clicked()
{
    this->showMinimized();
}

RTCDemo::~RTCDemo()
{
    m_imgDownloader.close();
	if (m_doublePanel)
	{
		delete m_doublePanel;
	}
	if (m_multiPanel)
	{
		delete m_multiPanel;
	}

	RTCRoom* roomService = RTCRoom::instance();
	delete roomService;
}

void RTCDemo::createRoom(RTCAuthData authData, const QString& serverDomain, const QString& roomID, const QString& roomInfo, bool record, int picture_id, ScreenRecordType screenRecord)
{
	m_bCreate = true;
    m_roomID = roomID;
	m_roomInfo = roomInfo;
	m_screenRecord = screenRecord;
	BoardService::instance().setRoomID(roomID.toStdString());
	if (record)
	{
		RTCRoom::instance()->recordVideo(m_multi, picture_id);
	}
	RTCRoom::instance()->login(serverDomain.toStdString(), authData, this);	
	init(authData, roomInfo);
}

void RTCDemo::enterRoom(RTCAuthData authData, const QString& serverDomain, const QString& roomID, const QString& roomInfo, bool record, int picture_id, ScreenRecordType screenRecord)
{
	m_bCreate = false;
    m_roomID = roomID;
	m_roomInfo = roomInfo;
	m_screenRecord = screenRecord;
	BoardService::instance().setRoomID(roomID.toStdString());
	if (record)
	{
		RTCRoom::instance()->recordVideo(m_multi, picture_id);
	}
	RTCRoom::instance()->login(serverDomain.toStdString(), authData, this);
	init(authData, roomInfo);
}

void RTCDemo::setLogo(QString logoURL, bool multi)
{
    if (true == logoURL.isEmpty())
    {
        if (true == multi)
        {
            ui.label_logo->setStyleSheet("image: url(:/RoomService/logo-court-demo.png)");
        }
        else
        {
            ui.label_logo->setStyleSheet("image: url(:/RoomService/logo-cs-demo.png)");
        }
    }
    else
    {
        m_imgDownloader.download(logoURL.toStdWString(), 1500);
    }

    ui.label_logo->show();
}

void RTCDemo::setTitle(QString title)
{
	ui.label_title->show();
	ui.label_title->setText(title);
}

void RTCDemo::leaveRoom()
{
    on_btn_close_clicked();
}

void RTCDemo::init(const RTCAuthData& authData, const QString& roomName)
{
    m_authData = authData;

	BoardAuthData boardAuthData;
	boardAuthData.sdkAppID = authData.sdkAppID;
	boardAuthData.token = authData.token;
	boardAuthData.userID = authData.userID;

	BoardService::instance().init(boardAuthData);

	if (m_multiPanel)
	{
		m_multiPanel->setUserInfo(authData.userID, authData.userName);
	}
	if (m_doublePanel)
	{
		m_doublePanel->setUserInfo(authData.userID, authData.userName);
	}

	if (m_imPanel)
	{
		m_imPanel->setNickName(authData.userName.c_str());
		m_imPanel->setUserId(authData.userID.c_str());
	}
}

void RTCDemo::setInitBeautyValue()
{
	QFile file(QCoreApplication::applicationDirPath() + "/beauty-config.ini");

	if (!file.exists())
	{
		file.open(QIODevice::ReadWrite);
		file.close();

		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/beauty-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		setting->setValue("style", m_beautyStyle);
		setting->setValue("beauty", m_beautyLevel);
		setting->setValue("white", m_whitenessLevel);
		setting->endGroup();
	}
	else
	{
		QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/beauty-config.ini", QSettings::IniFormat);
		setting->beginGroup("config");
		m_beautyStyle = setting->value("style").toInt();
		m_beautyLevel = setting->value("beauty").toInt();
		m_whitenessLevel = setting->value("white").toInt();
		setting->endGroup();
	}

	RTCRoom::instance()->setBeautyStyle((RTCBeautyStyle)m_beautyStyle, m_beautyLevel, m_whitenessLevel);
}

void RTCDemo::onCreateRoom(const RTCResult& res, const std::string& roomID)
{
	if (RTCROOM_SUCCESS != res.ec)
	{
		emit dispatch([this, res] {
			RTCRoom::instance()->stopLocalPreview();

			DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK);
		});

        Application::instance().pushRoomStatus(-1004, Wide2UTF8(L"创建房间失败"));
	}
	else
	{
		emit dispatch([this, roomID] {
			if (m_multiPanel)
			{
				m_multiPanel->initShowVideo();
			}
			if (m_doublePanel)
			{
				m_doublePanel->initShowVideo();
			}
			m_roomID = roomID.c_str();
			BoardService::instance().setRoomID(roomID);
		});
		setInitBeautyValue();
        Application::instance().pushRoomStatus(0, Wide2UTF8(L"创建房间成功"));
	}
}

void RTCDemo::onEnterRoom(const RTCResult& res)
{
	if (RTCROOM_SUCCESS != res.ec)
	{
		emit dispatch([this, res] {
			RTCRoom::instance()->stopLocalPreview();

			DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK);
		});

        Application::instance().pushRoomStatus(-1006, Wide2UTF8(L"进入房间失败"));
	}
	else
	{
		emit dispatch([this] {
			if (m_multiPanel)
			{
				m_multiPanel->initShowVideo();
			}
			if (m_doublePanel)
			{
				m_doublePanel->initShowVideo();
			}
		});
		setInitBeautyValue();
        Application::instance().pushRoomStatus(1, Wide2UTF8(L"进入房间成功"));
	}
}

void RTCDemo::onUpdateRoomData(const RTCResult& res, const RTCRoomData& roomData)
{
    emit dispatch([this, res, roomData] {
        if (RTCROOM_SUCCESS != res.ec)
        {
            DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK);
        }
        else
        {
            if (roomData.roomCreator == m_authData.userID)
            {
				QString nickName = m_authData.userName.c_str();
				nickName.append("(" + m_userTag + ")");
				if (m_imPanel)
				{
					m_imPanel->setNickName(nickName);
				}
            }

			if (m_imPanel)
			{
				m_imPanel->setRoomCreator(roomData.roomCreator.c_str());
			}
            
            if (m_multiPanel)
            {
                m_multiPanel->setRoomCreator(roomData.roomCreator.c_str());
            }
            if (m_doublePanel)
            {
                m_doublePanel->setRoomCreator(roomData.roomCreator.c_str());
            }

            // 添加pusher
            std::list<MemberItem> newMembers;
            for (std::vector<RTCMemberData>::const_iterator it = roomData.members.begin()
                ; roomData.members.end() != it; ++it)
            {
                MemberItem item;
                item.userID = it->userID;
                item.userAvatar = it->userAvatar;
                if (roomData.roomCreator == it->userID)
                {
                    item.role = MasterPusherRole;
                    item.userName = it->userName;
                    item.userName.append(QString("(" + m_userTag + ")").toStdString());
                    item.status = QString(QStringLiteral("主播")).toStdString();
                }
                else
                {
                    item.role = SlavePusherRole;
                    item.userName = it->userName;
                    item.status = QString(QStringLiteral("互动中")).toStdString();
                }

                newMembers.push_back(item);
            }

            // 添加audience
            for (std::list<MemberItem>::iterator it = m_members.begin(); m_members.end() != it; ++it)
            {
                if (it->role != AudienceRole)
                {
                    continue;
                }

                bool flag = false;
                for (std::list<MemberItem>::iterator newIt = newMembers.begin(); newMembers.end() != newIt; ++newIt)
                {
                    if (newIt->userID == it->userID)
                    {
                        flag = true;
                        break;
                    }
                }

                if (true == flag)
                {
                    newMembers.push_back(*it);
                }
            }

            m_members = newMembers;
			if (m_memberPanel)
			{
				m_memberPanel->updateList(m_members);
			}

            Application::instance().pushMemberChange(m_members);
        }
    });
}

void RTCDemo::onPusherJoin(const RTCMemberData & member)
{
	RTCMemberData qMember = member;

	emit dispatch([this, qMember] {
		MemberItem tmp;
		tmp.userID = qMember.userID;
		tmp.userName = qMember.userName;
		if (m_multiPanel && m_multiPanel->getVideoCount()< m_cameraSize)
		{
			m_multiPanel->onPusherJoin(tmp);
		}
		if (m_doublePanel)
		{
			m_doublePanel->onPusherJoin(tmp);
		}
		//BoardService::instance().gotoCurrentPage();
	});
}

void RTCDemo::onPusherQuit(const RTCMemberData & member)
{
	RTCMemberData qMember = member;

	emit dispatch([this, qMember] {
		QString tip = qMember.userName.c_str() + QStringLiteral("退出房间");

		m_toast->setDuration(3000);
		m_toast->setText(tip);

		MemberItem tmp;
		tmp.userID = qMember.userID;
		tmp.userName = qMember.userName;
		if (m_multiPanel)
		{
			m_multiPanel->onPusherQuit(tmp);
		}
		if (m_doublePanel)
		{
			m_doublePanel->onPusherQuit(tmp);
		}

        for (std::list<MemberItem>::iterator it = m_members.begin(); m_members.end() != it; ++it)
        {
            if (it->userID == qMember.userID)
            {
                m_members.erase(it);
                break;
            }
        }

		if (m_memberPanel)
		{
			m_memberPanel->updateList(m_members);
		}
        Application::instance().pushMemberChange(m_members);
	});
}

void RTCDemo::onRoomClosed(const std::string & roomID)
{
	emit dispatch([this] {
		QString tip = QStringLiteral("房间已解散!");
		DialogMessage::exec(tip, DialogMessage::OK);

        m_members.clear();
		if (m_memberPanel)
		{
			m_memberPanel->updateList(m_members);
		}
		if (m_imPanel)
		{
			m_imPanel->onRoomClosed();
		}
		if (m_multiPanel)
		{
			m_multiPanel->onRoomClosed();
		}
		if (m_doublePanel)
		{
			m_doublePanel->onRoomClosed();
		}

        Application::instance().pushMemberChange(m_members);
	});

    Application::instance().pushRoomStatus(2, Wide2UTF8(L"退出房间"));
}

void RTCDemo::onRecvRoomTextMsg(const char * roomid, const char * userid, const char * userName, const char * userAvatar, const char * msg)
{
	if (m_imPanel)
	{
		m_imPanel->onRecvGroupTextMsg(roomid, userid, userName, msg);
	}
	
    Application::instance().pushRoomTextMsg(roomid, userid, userName, userAvatar, msg);
}

void RTCDemo::onRecvRoomCustomMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * message)
{
	if (m_imPanel)
	{
		m_imPanel->onRecvGroupTextMsg(roomID, userID, userName, message);
	}
}

void RTCDemo::onRecvC2CCustomMsg(const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * msg)
{
    if (QString(cmd) == "sketchpad")
    {
        Json::Value root;
        Json::Reader reader;
        if (false == reader.parse(msg, root))
        {
            return;
        }

        std::string type;
        if (root.isMember("type"))
        {
            type = root["type"].asString();
        }

        std::string action;
        if (root.isMember("action"))
        {
            action = root["action"].asString();
        }

        if (type == "request" && action == "currentPPT")
        {
            emit dispatch([this] {
                BoardService::instance().gotoCurrentPage();
            });
        }
    }
}

void RTCDemo::onTIMKickOffline()
{
    emit dispatch([] {
        DialogMessage::exec(QStringLiteral("IM强制下线，请检查是否重复登录同一个账号"), DialogMessage::OK);
    });
}

void RTCDemo::onError(const RTCResult & res, const std::string & userID)
{
    QString msgContent = QString::fromLocal8Bit(res.msg.c_str());

	emit dispatch([this, res, msgContent] {
		DialogMessage::exec(msgContent, DialogMessage::OK);
	});

    if (RTCROOM_SUCCESS != res.ec)
    {
        Application::instance().pushRoomStatus(res.ec, msgContent.toUtf8().toStdString());
    }
}

void RTCDemo::onLogin(const RTCResult & res, const RTCAuthData & authData)
{
	if (RTCROOM_SUCCESS == res.ec)
	{
		if (m_bCreate)
			RTCRoom::instance()->createRoom(m_roomID.toStdString(), m_roomInfo.toStdString());
		else
			RTCRoom::instance()->enterRoom(m_roomID.toStdString());
	}
	else
	{
		emit dispatch([=] {
			RTCRoom::instance()->stopLocalPreview();
			DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK);
		});
	}
}

void RTCDemo::onSendIMGroupMsg(const std::string & msg)
{
	RTCRoom::instance()->sendRoomTextMsg(msg.c_str());
}

void RTCDemo::initUI(const QString& strTemplate, const QString& userTag, bool bUserList, bool bIMList, bool whiteboard, bool screenShare)
{
	if (strTemplate == "1v1")
	{
		m_multi = false;
	}
	else if (strTemplate == "1v2")
	{
		m_cameraSize = 2;
	}
	else if (strTemplate == "1v3")
	{
		m_cameraSize = 3;
	}
	else if (strTemplate == "1v4")
	{
		m_cameraSize = 4;
	}
	m_userTag = userTag;

	QVBoxLayout *tabVBoxLayout = new QVBoxLayout(ui.widget_main_share);
	tabVBoxLayout->setMargin(0);
	ui.widget_main_share->setLayout(tabVBoxLayout);
	if (m_multi)
	{
		m_multiPanel = new MultiVideoPanel(ui.widget_main_share);
		tabVBoxLayout->addWidget(m_multiPanel);
		m_multiPanel->initConfigSetting(DEFAULT_CAMERA_SIZE, whiteboard, screenShare);
		m_multiPanel->show();
	}
	else
	{
		m_doublePanel = new DoubleVideoPanel(ui.widget_main_share);
		tabVBoxLayout->addWidget(m_doublePanel);
		m_doublePanel->initConfigSetting(whiteboard, screenShare);
		m_doublePanel->show();
	}

	if (!m_memberPanel && bUserList)
	{
		m_memberPanel = new MemberPanel(ui.widget_member);
		QVBoxLayout *memberVBoxLayout = new QVBoxLayout(ui.widget_member);
		memberVBoxLayout->setMargin(0);
		memberVBoxLayout->addWidget(m_memberPanel);
		ui.widget_member->setLayout(memberVBoxLayout);
		m_memberPanel->show();
	}

	if (!m_imPanel && bIMList)
	{
		m_imPanel = new IMPanel(this, ui.widget_message);
		QVBoxLayout *msgVBoxLayout = new QVBoxLayout(ui.widget_message);
		msgVBoxLayout->setMargin(0);
		msgVBoxLayout->addWidget(m_imPanel);
		ui.widget_message->setLayout(msgVBoxLayout);
		m_imPanel->show();
	}

	if (!bUserList && !bIMList)
	{
		ui.widget_member->hide();
		ui.widget_message->hide();
		this->setFixedWidth(m_demoWidth - 2 * PANEL_WIDTH);
		ui.widget_bottom->setFixedWidth(m_demoWidth - 2 * PANEL_WIDTH);
		ui.widget_top->setFixedWidth(m_demoWidth - 2 * PANEL_WIDTH);
		ui.widget_bottom->setContentsMargins(20, 0, 20, 0);
	}
	else if (!bUserList)
	{
		ui.widget_member->hide();
		this->setFixedWidth(m_demoWidth - PANEL_WIDTH);
		ui.widget_bottom->setFixedWidth(m_demoWidth - PANEL_WIDTH);
		ui.widget_top->setFixedWidth(m_demoWidth - PANEL_WIDTH);
		ui.widget_bottom->setContentsMargins(10, 0, 0, 0);
	}
	else if (!bIMList)
	{
		ui.widget_message->hide();
		this->setFixedWidth(m_demoWidth - PANEL_WIDTH);
		ui.widget_bottom->setFixedWidth(m_demoWidth - PANEL_WIDTH);
		ui.widget_top->setFixedWidth(m_demoWidth - PANEL_WIDTH);
		ui.widget_bottom->setContentsMargins(0, 0, 10, 0);
	}

	if (!m_toast)
	{
		m_toast = new QWidgetToast(this);
	}
}

void RTCDemo::setProxy(const std::string& ip, unsigned short port)
{
    RTCRoom::instance()->setProxy(ip, port);

    m_imgDownloader.setProxy(ip, port);
}

void RTCDemo::handle(txfunction func)
{
	func();
}

void RTCDemo::on_btn_device_manage_clicked()
{
	if (!m_deviceManage)
	{
		m_deviceManage = new DeviceManage(this);
		int cameraCount = RTCRoom::instance()->enumCameras();
		wchar_t **camerasName = new wchar_t *[cameraCount];
		for (int i = 0; i < cameraCount; ++i)
		{
			camerasName[i] = new wchar_t[256];
		}
		RTCRoom::instance()->enumCameras(camerasName, cameraCount);

		int micCount = RTCRoom::instance()->micDeviceCount();
		char **micsName = new char *[micCount];
		for (int i = 0; i < micCount; ++i)
		{
			micsName[i] = new char[512];
			RTCRoom::instance()->micDeviceName(i, micsName[i]);
		}

		m_deviceManage->setCameras(camerasName, cameraCount);
		m_deviceManage->setMics(micsName, micCount);
		m_deviceManage->setMicVolume(RTCRoom::instance()->micVolume(), true);

		for (int i = 0; i < cameraCount; ++i)
		{
			delete[] camerasName[i];
		}
		delete[] camerasName;

		for (int i = 0; i < micCount; ++i)
		{
			delete[] micsName[i];
		}
		delete[] micsName;
	}
	else
	{
		bool ret = m_deviceManage->updateUI();
		if (ret)
		{
			QWidget* display = m_deviceManage->getRenderWidget();
			RTCRoom::instance()->updateLocalPreview((HWND)display->winId(), RECT{ 0, 0, display->width(), display->height() });
		}
	}
	m_deviceManage->exec();
}

void RTCDemo::on_btn_beauty_manage_clicked()
{
	if (!m_beautyManage)
	{
		m_beautyManage = new BeautyManage(this);
	}

	m_beautyManage->exec();
}

void RTCDemo::on_cmb_mic_currentIndexChanged(int index)
{
	RTCRoom::instance()->selectMicDevice(index);
	m_deviceManage->setMicVolume(RTCRoom::instance()->micVolume());
}

void RTCDemo::on_cmb_camera_currentIndexChanged(int index)
{
	RTCRoom::instance()->switchCamera(index);
	QWidget* display = m_deviceManage->getRenderWidget();
	RTCRoom::instance()->updateLocalPreview((HWND)display->winId(), RECT{ 0, 0, display->width(), display->height() });
}

void RTCDemo::on_slider_volume_valueChanged(int value)
{
	RTCRoom::instance()->setMicVolume(value);
}

void RTCDemo::on_device_manage_ok(bool enableCamera, bool enableMic)
{
	if (m_multiPanel)
	{
		m_multiPanel->setDeviceEnabled(enableCamera, enableMic);
		m_multiPanel->updatePreview();

	}
	if (m_doublePanel)
	{
		m_doublePanel->setDeviceEnabled(enableCamera, enableMic); 
		m_doublePanel->updatePreview();
	}
}

void RTCDemo::on_device_manage_cancel(int cameraIndex, int micIndex, int micVolume)
{
	RTCRoom::instance()->selectMicDevice(micIndex);
	RTCRoom::instance()->setMicVolume(micVolume);
	RTCRoom::instance()->switchCamera(cameraIndex);
	if (m_multiPanel)
	{
		m_multiPanel->updatePreview();

	}
	if (m_doublePanel)
	{
		m_doublePanel->updatePreview();
	}
}

void RTCDemo::on_beauty_manage_ok(int beautyStyle, int beautyLevel, int whitenessLevel)
{
	m_beautyStyle = beautyStyle;
	m_beautyLevel = beautyLevel;
	m_whitenessLevel = whitenessLevel;
	RTCRoom::instance()->setBeautyStyle((RTCBeautyStyle)beautyStyle, beautyLevel, whitenessLevel);
}

void RTCDemo::on_chb_camera_stateChanged(int state)
{
	if (state == Qt::Unchecked)
	{
		RTCRoom::instance()->stopLocalPreview();
		QWidget* display = m_deviceManage->getRenderWidget();
		display->update();
	}
	else
	{
		QWidget* display = m_deviceManage->getRenderWidget();
		RTCRoom::instance()->startLocalPreview((HWND)display->winId(), RECT{ 0, 0, display->width(), display->height() });
	}
}