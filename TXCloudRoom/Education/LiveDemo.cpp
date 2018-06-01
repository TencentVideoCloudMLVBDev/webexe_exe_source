#include "LiveDemo.h"
#include "DialogMessage.h"
#include "BoardService.h"
#include "LRHttpRequest.h"
#include "Application.h"
#include "Base.h"
#include "log.h"

#include <Dwmapi.h> 

QWidget * LiveMainWindow = nullptr;
#define PANEL_WIDTH 200
#define LIVE_PLAYER_SIZE (3)
#define DEFAULT_CAMERA_SIZE 4

LiveDemo::LiveDemo(QWidget *parent)
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

	LiveMainWindow = this;
	m_demoWidth = this->geometry().width();

	qRegisterMetaType<txfunction>("txfunction");
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);

	LiveRoom::instance()->setCallback(this);

	ui.label_logo->hide();
	ui.label_title->hide();
}

void LiveDemo::mousePressEvent(QMouseEvent *e)
{
	mousePressedPosition = e->globalPos();
	windowPositionAsDrag = pos();
}

void LiveDemo::mouseReleaseEvent(QMouseEvent *e)
{
	Q_UNUSED(e)
		mousePressedPosition = QPoint();
}

void LiveDemo::mouseMoveEvent(QMouseEvent *e)
{
	if (!mousePressedPosition.isNull()) {
		QPoint delta = e->globalPos() - mousePressedPosition;
		move(windowPositionAsDrag + delta);
	}
}

void LiveDemo::timerEvent(QTimerEvent *event)
{
    if (false == m_roomID.empty())
    {
        LiveRoom::instance()->getAudienceList(m_roomID);
    }
}

void LiveDemo::showEvent(QShowEvent * event)
{
	this->setAttribute(Qt::WA_Mapped);
	QMainWindow::showEvent(event);
}

void LiveDemo::onDownloadFinished(bool success, QByteArray image)
{
    QPixmap pixmap;
    pixmap.loadFromData(image);
    ui.label_logo->setPixmap(pixmap);
}

void LiveDemo::on_btn_close_clicked()
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

	killTimer(m_timerID);
	m_roomID = "";
	LiveRoom::instance()->leaveRoom();
	LiveRoom::instance()->logout();

	this->close();
    Application::instance().quit(0);
}

void LiveDemo::on_btn_min_clicked()
{
    this->showMinimized();
}

LiveDemo::~LiveDemo()
{
    m_imgDownloader.close();
	killTimer(m_timerID);

	if (m_mainPanel)
	{
		delete m_mainPanel;
	}

	LiveRoom* roomService = LiveRoom::instance();
	delete roomService;
}

void LiveDemo::createRoom(const LRAuthData & authData, const QString & serverDomain, const QString & roomID, const QString & roomInfo, bool record, int picture_id, ScreenRecordType screenRecord)
{
	m_bCreate = true;
    m_roomID = roomID.toStdString();
	m_roomInfo = roomInfo;
	m_screenRecord = screenRecord;
	BoardService::instance().setRoomID(roomID.toStdString());
	if (record)
	{
		LiveRoom::instance()->recordVideo(picture_id);
	}
	LiveRoom::instance()->login(serverDomain.toStdString(), authData, this);

	init(authData, roomInfo);
}

void LiveDemo::enterRoom(const LRAuthData & authData, const QString & serverDomain, const QString & roomID, const QString & roomInfo, bool record, int picture_id, ScreenRecordType screenRecord)
{
	m_bCreate = false;
    m_roomID = roomID.toStdString();
	m_roomInfo = roomInfo;
	m_screenRecord = screenRecord;
	BoardService::instance().setRoomID(roomID.toStdString());
	if (record)
	{
		LiveRoom::instance()->recordVideo(picture_id);
	}
	LiveRoom::instance()->login(serverDomain.toStdString(), authData, this);

	init(authData, roomInfo);
}

void LiveDemo::setLogo(QString logoURL)
{
	LINFO(L"logoURL: %s", logoURL.toStdWString().c_str());

    if (true == logoURL.isEmpty())
    {
        ui.label_logo->setStyleSheet("image: url(:/RoomService/logo-edu-demo.png)");
    }
    else
    {
        m_imgDownloader.download(logoURL.toStdWString(), 1500);
    }

    ui.label_logo->show();
}

void LiveDemo::setTitle(QString title)
{
	ui.label_title->show();
	ui.label_title->setText(title);
}

void LiveDemo::leaveRoom()
{
    on_btn_close_clicked();
}

void LiveDemo::init(const LRAuthData& authData, const QString& roomName)
{
    m_authData = authData;

	BoardAuthData boardAuthData;
	boardAuthData.sdkAppID = authData.sdkAppID;
	boardAuthData.token = authData.token;
	boardAuthData.userID = authData.userID;

	BoardService::instance().init(boardAuthData);

    m_mainPanel->setUserInfo(authData.userID, authData.userName);
	if (m_imPanel)
	{
		m_imPanel->setUserId(authData.userID.c_str());
		m_imPanel->setNickName(authData.userName.c_str());
	}
}

void LiveDemo::setInitBeautyValue()
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

	LiveRoom::instance()->setBeautyStyle((LRBeautyStyle)m_beautyStyle, m_beautyLevel, m_whitenessLevel);
}

void LiveDemo::onCreateRoom(const LRResult& res, const std::string& roomID)
{
	if (LIVEROOM_SUCCESS != res.ec)
	{
		emit dispatch([this, res] {
			LiveRoom::instance()->stopLocalPreview();

			DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK);
		});

        Application::instance().pushRoomStatus(-1004, Wide2UTF8(L"创建房间失败"));
	}
	else
	{
		emit dispatch([this, roomID] {
            LiveRoom::instance()->setBeautyStyle(LIVEROOM_BEAUTY_STYLE_NATURE, 5, 5);

			m_roomID = roomID.c_str();
			BoardService::instance().setRoomID(roomID);
			m_timerID = startTimer(5 * 1000);
			m_mainPanel->initShowVideo();
		});
		setInitBeautyValue();
        Application::instance().pushRoomStatus(0, Wide2UTF8(L"创建房间成功"));
	}
}

void LiveDemo::onEnterRoom(const LRResult& res)
{
	if (LIVEROOM_SUCCESS != res.ec)
	{
		emit dispatch([this, res] {
			LiveRoom::instance()->stopLocalPreview();
			DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK);
		});

        Application::instance().pushRoomStatus(-1006, Wide2UTF8(L"进入房间失败"));
	}
	else
	{
		emit dispatch([this] {
            LiveRoom::instance()->setBeautyStyle(LIVEROOM_BEAUTY_STYLE_NATURE, 5, 5);

			m_timerID = startTimer(5 * 1000);
			m_mainPanel->initShowVideo();
		});
		setInitBeautyValue();
        Application::instance().pushRoomStatus(1, Wide2UTF8(L"进入房间成功"));
	}
}

void LiveDemo::onUpdateRoomData(const LRResult& res, const LRRoomData& roomData)
{
    emit dispatch([this, res, roomData] {
        if (LIVEROOM_SUCCESS != res.ec)
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

            m_roomID = roomData.roomID;
            m_mainPanel->setRoomCreator(roomData.roomCreator.c_str());
			if (m_imPanel)
			{
				m_imPanel->setRoomCreator(roomData.roomCreator.c_str());
			}
            // 添加pusher
            std::list<MemberItem> newMembers;
            for (std::vector<LRMemberData>::const_iterator it = roomData.members.begin()
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

void LiveDemo::onGetAudienceList(const LRResult& res, const std::vector<LRAudienceData>& audiences)
{
    if (LIVEROOM_SUCCESS == res.ec)
    {
		std::vector<LRAudienceData> qAudiences = audiences;
        emit dispatch([this, qAudiences] {
            // 添加pusher
            std::list<MemberItem> newMembers;
            for (std::list<MemberItem>::iterator it = m_members.begin(); m_members.end() != it; ++it)
            {
                if (MasterPusherRole == it->role || SlavePusherRole == it->role)
                {
                    newMembers.push_back(*it);
                }
            }

            // 添加audience
            for (std::vector<LRAudienceData>::const_iterator it = qAudiences.begin(); qAudiences.end() != it; ++it)
            {
                bool flag = false;
                for (std::list<MemberItem>::iterator newIt = newMembers.begin(); newMembers.end() != newIt; ++newIt)
                {
                    if (newIt->userID == it->userID)
                    {
                        flag = true;
                        break;
                    }
                }

                if (false == flag)
                {
                    MemberItem item;
                    item.role = AudienceRole;
                    item.userID = it->userID;
                    item.status = "";

                    QByteArray data(it->userInfo.c_str(), it->userInfo.size());

                    QJsonParseError jsonError;
                    QJsonDocument document = QJsonDocument::fromJson(data, &jsonError);
                    if (jsonError.error != QJsonParseError::NoError || !document.isObject())
                    {
                        continue;
                    }

                    QJsonObject root = document.object();

                    QString userName;
                    if (root.contains("userName"))
                    {
                        userName = root.value("userName").toString();
                        item.userName = userName.toStdString();
                    }

                    QString userAvatar;
                    if (root.contains("userAvatar"))
                    {
                        userAvatar = root.value("userAvatar").toString();
                        item.userAvatar = userAvatar.toStdString();
                    }

                    newMembers.push_back(item);
                }
            }

            if (m_members.size() != newMembers.size())
            {
                Application::instance().pushMemberChange(m_members);
            }

            m_members = newMembers;
			if (m_memberPanel)
			{
				m_memberPanel->updateList(m_members);
			}
        });
    }
}

void LiveDemo::onPusherJoin(const LRMemberData & member)
{
	LRMemberData qMember = member;
	LiveRoom::instance()->getAudienceList(m_roomID);

	emit dispatch([this, qMember] {
		MemberItem member;
        member.userID = qMember.userID;
        member.userName = qMember.userName;
		m_mainPanel->onPusherJoin(member);
	});
}

void LiveDemo::onPusherQuit(const LRMemberData & member)
{
	LRMemberData qMember = member;
	LiveRoom::instance()->getAudienceList(m_roomID);

	emit dispatch([this, qMember] {
		QString tip = qMember.userName.c_str() + QStringLiteral("退出连麦");

		m_toast->setDuration(3000);
		m_toast->setText(tip);

        MemberItem member;
        member.userID = qMember.userID;
        member.userName = qMember.userName;
		m_mainPanel->onPusherQuit(member);

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

void LiveDemo::onRoomClosed(const std::string& roomID)
{
    emit dispatch([this] {
		QString tip = QStringLiteral("房间已解散!");
		DialogMessage::exec(tip, DialogMessage::OK);

		if (m_imPanel)
		{
			m_imPanel->onRoomClosed();
		}
        m_mainPanel->onRoomClosed();

        m_members.clear();
		if (m_memberPanel)
		{
			m_memberPanel->updateList(m_members);
		}
        killTimer(m_timerID);

        Application::instance().pushMemberChange(m_members);
    });

    Application::instance().pushRoomStatus(2, Wide2UTF8(L"退出房间"));
}

void LiveDemo::onRecvRoomTextMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * msg)
{
	if (m_imPanel)
	{
		m_imPanel->onRecvGroupTextMsg(roomID, userID, userName, msg);
	}
    Application::instance().pushRoomTextMsg(roomID, userID, userName, userAvatar, msg);
}

void LiveDemo::onRecvRoomCustomMsg(const char * roomID, const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * message)
{
	if (m_imPanel)
	{
		m_imPanel->onRecvGroupTextMsg(roomID, userID, userName, message);
	}
}

void LiveDemo::onRecvC2CCustomMsg(const char * userID, const char * userName, const char * userAvatar, const char * cmd, const char * msg)
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

void LiveDemo::onTIMKickOffline()
{
    emit dispatch([] {
        DialogMessage::exec(QStringLiteral("IM强制下线，请检查是否重复登录同一个账号"), DialogMessage::OK);
    });
}

void LiveDemo::onError(const LRResult& res, const std::string& userID)
{
    QString msgContent = QString::fromLocal8Bit(res.msg.c_str());

	emit dispatch([this, res, userID, msgContent] {
		DialogMessage::exec(msgContent, DialogMessage::OK);
	});

    if (LIVEROOM_SUCCESS != res.ec)
    {
        Application::instance().pushRoomStatus(res.ec, msgContent.toUtf8().toStdString());
    }
}

void LiveDemo::onRecvJoinPusherRequest(const std::string& roomID, const std::string& userID, const std::string& userName, const std::string& userAvatar)
{
	if (m_mainPanel->getVideoCount() >= LIVE_PLAYER_SIZE || m_mainPanel->getVideoCount() >= DEFAULT_CAMERA_SIZE)
	{
		LiveRoom::instance()->rejectJoinPusher(userID, QString::fromStdWString(L"主播端连麦人数超过最大限制").toUtf8().data());
		return;
	}
	QString qUserName = QString::fromUtf8(userName.c_str());
	emit dispatch([this, userID, qUserName] {
		QString tip = qUserName + QStringLiteral("正在请求连麦，是否同意？");
		int ret = DialogMessage::exec(tip, DialogMessage::OK | DialogMessage::CANCEL, 10000);
		if (ret == DialogMessage::Accepted)
		{
			if (m_mainPanel->getVideoCount() >= LIVE_PLAYER_SIZE)
			{
				LiveRoom::instance()->rejectJoinPusher(userID, QString::fromStdWString(L"连麦人数超过限制").toUtf8().data());
				return;
			}
			else
				LiveRoom::instance()->acceptJoinPusher(userID);
		}
        else if (ret == DialogMessage::Timeout)
        {
        }
		else
		{
			LiveRoom::instance()->rejectJoinPusher(userID, QString::fromStdWString(L"主播拒绝了您的连麦请求").toUtf8().data());
		}
	});
}

void LiveDemo::onRecvAcceptJoinPusher(const std::string& roomID, const std::string& msg)
{
}

void LiveDemo::onRecvRejectJoinPusher(const std::string& roomID, const std::string& msg)
{
}

void LiveDemo::onRecvKickoutSubPusher(const std::string& roomID)
{
}

void LiveDemo::onLogin(const LRResult & res, const LRAuthData & authData)
{
	if (LIVEROOM_SUCCESS == res.ec)
	{
		if (m_bCreate)
			LiveRoom::instance()->createRoom(m_roomID, m_roomInfo.toStdString());
	}
	else
	{
		emit dispatch([=] {
			LiveRoom::instance()->stopLocalPreview();
			DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK);
		});
	}
}

void LiveDemo::onSendIMGroupMsg(const std::string & msg)
{
	LiveRoom::instance()->sendRoomTextMsg(msg.c_str());
}

void LiveDemo::initUI(const QString& strTemplate, const QString& userTag,
	bool bUserList, bool bIMList, bool whiteboard, bool screenShare)
{
	if (strTemplate == "1v2")
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

	m_mainPanel = new LiveVideoPanel(ui.widget_main_share);
	QVBoxLayout *tabVBoxLayout = new QVBoxLayout(ui.widget_main_share);
	tabVBoxLayout->setMargin(0);
	tabVBoxLayout->addWidget(m_mainPanel);
	ui.widget_main_share->setLayout(tabVBoxLayout);
	m_mainPanel->initConfigSetting(DEFAULT_CAMERA_SIZE, whiteboard, screenShare);
	m_mainPanel->show();

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

	m_toast = new QWidgetToast(this);
}

void LiveDemo::setProxy(const std::string& ip, unsigned short port)
{
    LiveRoom::instance()->setProxy(ip, port);

    m_imgDownloader.setProxy(ip, port);
}

void LiveDemo::on_btn_device_manage_clicked()
{
	if (!m_deviceManage)
	{
		m_deviceManage = new DeviceManage(this);
		int cameraCount = LiveRoom::instance()->enumCameras();
		wchar_t **camerasName = new wchar_t *[cameraCount];
		for (int i = 0; i < cameraCount; ++i)
		{
			camerasName[i] = new wchar_t[256];
		}
		LiveRoom::instance()->enumCameras(camerasName, cameraCount);

		int micCount = LiveRoom::instance()->micDeviceCount();
		char **micsName = new char *[micCount];
		for (int i = 0; i < micCount; ++i)
		{
			micsName[i] = new char[512];
			LiveRoom::instance()->micDeviceName(i, micsName[i]);
		}

		m_deviceManage->setCameras(camerasName, cameraCount);
		m_deviceManage->setMics(micsName, micCount);
		m_deviceManage->setMicVolume(LiveRoom::instance()->micVolume(), true);

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
			LiveRoom::instance()->updateLocalPreview((HWND)display->winId(), RECT{ 0, 0, display->width(), display->height() });
		}
	}

	m_deviceManage->exec();
}

void LiveDemo::on_btn_beauty_manage_clicked()
{
	if (!m_beautyManage)
	{
		m_beautyManage = new BeautyManage(this);
	}

	m_beautyManage->exec();
}

void LiveDemo::on_cmb_mic_currentIndexChanged(int index)
{
	LiveRoom::instance()->selectMicDevice(index);
	m_deviceManage->setMicVolume(LiveRoom::instance()->micVolume());
}

void LiveDemo::on_cmb_camera_currentIndexChanged(int index)
{
	LiveRoom::instance()->switchCamera(index);
	QWidget* display = m_deviceManage->getRenderWidget();
	LiveRoom::instance()->updateLocalPreview((HWND)display->winId(), RECT{ 0, 0, display->width(), display->height() });
}

void LiveDemo::on_slider_volume_valueChanged(int value)
{
	LiveRoom::instance()->setMicVolume(value);
}

void LiveDemo::on_device_manage_ok(bool enableCamera, bool enableMic)
{
	m_mainPanel->setDeviceEnabled(enableCamera, enableMic);
	m_mainPanel->updatePreview();
}

void LiveDemo::on_device_manage_cancel(int cameraIndex, int micIndex, int micVolume)
{
	LiveRoom::instance()->selectMicDevice(micIndex);
	LiveRoom::instance()->setMicVolume(micVolume);
	LiveRoom::instance()->switchCamera(cameraIndex);
	m_mainPanel->updatePreview();
}

void LiveDemo::on_beauty_manage_ok(int beautyStyle, int beautyLevel, int whitenessLevel)
{
	m_beautyStyle = beautyStyle;
	m_beautyLevel = beautyLevel;
	m_whitenessLevel = whitenessLevel;
	LiveRoom::instance()->setBeautyStyle((LRBeautyStyle)beautyStyle, beautyLevel, whitenessLevel);
}

void LiveDemo::on_chb_camera_stateChanged(int state)
{
	if (state == Qt::Unchecked)
	{
		LiveRoom::instance()->stopLocalPreview();
		QWidget* display = m_deviceManage->getRenderWidget();
		display->update();
	}
	else
	{
		QWidget* display = m_deviceManage->getRenderWidget();
		LiveRoom::instance()->startLocalPreview((HWND)display->winId(), RECT{ 0, 0, display->width(), display->height() });
	}
}

void LiveDemo::handle(txfunction func)
{
	func();
}
