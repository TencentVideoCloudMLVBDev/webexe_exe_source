#include "LiveDemo.h"
#include "DialogMessage.h"
#include "BoardService.h"
#include "LRHttpRequest.h"
#include "Application.h"
#include "Base.h"
#include "log.h"
#include "DataReport.h"
#include <Dwmapi.h> 
#include <QDesktopWidget>

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

    LiveMainWindow = this;
    m_demoWidth = this->geometry().width();

    qRegisterMetaType<txfunction>("txfunction");
    connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);

    LiveRoom::instance()->setCallback(this);

    ui.label_logo->hide();
    ui.label_title->hide();

    QDesktopWidget* desktopWidget = QApplication::desktop();
    m_normalRect = this->geometry();
    m_deskRect = desktopWidget->availableGeometry();

    m_deviceManage = new DeviceManage(this); 
	connect(m_deviceManage, SIGNAL(device_manage_tab_changed(int)), this, SLOT(on_device_manage_tab_changed(int)));
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
    if (!mousePressedPosition.isNull() && !m_bMax) {
        QPoint delta = e->globalPos() - mousePressedPosition;
        move(windowPositionAsDrag + delta);
    }
}

void LiveDemo::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_audienceListTimerID)
    {
        if (false == m_roomID.empty())
        {
            LiveRoom::instance()->getAudienceList(m_roomID);
        }
    }
    else if (event->timerId() == m_linkmicTimerID)
    {
        if (LinkMicBegin == m_linkmicStatus)
        {
            m_linkmicStatus = LinkMicOff;

            m_toast->setDuration(3000);
            m_toast->setText(QStringLiteral("连麦无响应"));

            killTimer(m_linkmicTimerID);
        }
    }
}

void LiveDemo::showEvent(QShowEvent * event)
{
	BOOL bEnable = false;
	::DwmIsCompositionEnabled(&bEnable);
	if (bEnable)
	{
		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
		::DwmSetWindowAttribute((HWND)winId(), DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
		MARGINS margins = { -1 };
		::DwmExtendFrameIntoClientArea((HWND)winId(), &margins);
	}

    this->setAttribute(Qt::WA_Mapped);
    this->update();
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
    LOGGER;

    if (m_screenRecord != RecordScreenNone)
    {
        TXCloudRecordCmd::instance().stop();
    }

    Application::instance().pushRoomStatus(2, Wide2UTF8(L"退出房间"));

	BoardService::instance().reportELK();

    killTimer(m_audienceListTimerID);
    killTimer(m_linkmicTimerID);

    m_roomID = "";
    LiveRoom::instance()->leaveRoom();
    LiveRoom::instance()->logout();
    DataReport::instance().setResult(DataReportLeave, "success");
    HttpReportRequest::instance().reportELK(DataReport::instance().getLeaveReport());

    if (m_screenRecord != RecordScreenNone)
    {
        TXCloudRecordCmd::instance().exit();
    }
    this->close();
    Application::instance().quit(0);
}

void LiveDemo::on_btn_min_clicked()
{
    this->showMinimized();
}

void LiveDemo::on_btn_max_clicked()
{
    if (m_bMax)
    {
        m_bMax = false;
        const QString maxBtnStyle =
            R"(QPushButton{
		font: 10pt "Microsoft YaHei";
		border: none;
		image: url(:/RoomService/max-button.png);
		background-position: center;
		}
		QPushButton:hover{
		image: url(:/RoomService/max-hover.png);
		}
		QPushButton:pressed{
		image: url(:/RoomService/max-hover.png);
		})";

        ui.btn_max->setStyleSheet(maxBtnStyle);

        int panelWidth = 200;
        this->setFixedSize(QSize(m_normalRect.width(), m_normalRect.height()));
        ui.widget_main->setFixedSize(QSize(m_normalRect.width(), m_normalRect.height()));
        ui.widget_member->setFixedWidth(panelWidth);
        ui.widget_message->setFixedWidth(panelWidth);
        ui.widget_main_share->setFixedWidth(m_normalRect.width() - panelWidth * 2 - 60);
        m_imPanel->setFixedSize(QSize(panelWidth, ui.widget_message->height()));
        m_memberPanel->setFixedSize(QSize(panelWidth, ui.widget_member->height()));
        m_mainPanel->setFixedSize(QSize(ui.widget_main_share->width(), ui.widget_main_share->height()));
        this->move((m_deskRect.width() - this->width()) / 2, (m_deskRect.height() - this->height()) / 2);
    }
    else
    {
        m_bMax = true;
        const QString restoreBtnStyle =
            R"(QPushButton{
		font: 10pt "Microsoft YaHei";
		border: none;
		image: url(:/RoomService/restore-button.png);
		background-position: center;
		}
		QPushButton:hover{
		image: url(:/RoomService/restore-hover.png);
		}
		QPushButton:pressed{
		image: url(:/RoomService/restore-hover.png);
		})";

        ui.btn_max->setStyleSheet(restoreBtnStyle);

        int panelWidth = 200 * m_deskRect.width() / 1080 * 1.05;
        this->setFixedSize(QSize(m_deskRect.width(), m_deskRect.height()));
        ui.widget_main->setFixedSize(QSize(m_deskRect.width(), m_deskRect.height()));
        ui.widget_member->setFixedWidth(panelWidth);
        ui.widget_message->setFixedWidth(panelWidth);
        ui.widget_main_share->setFixedWidth(m_deskRect.width() - panelWidth * 2 - 60);
        m_imPanel->setFixedSize(QSize(panelWidth, ui.widget_message->height()));
        m_memberPanel->setFixedSize(QSize(panelWidth, ui.widget_member->height()));
        m_mainPanel->setFixedSize(QSize(ui.widget_main_share->width(), ui.widget_main_share->height()));
        this->move(0, 0);
    }
}

LiveDemo::~LiveDemo()
{
    m_imgDownloader.close();
    killTimer(m_audienceListTimerID);
    killTimer(m_linkmicTimerID);

    if (m_mainPanel)
    {
        delete m_mainPanel;
    }

    LiveRoom* roomService = LiveRoom::instance();
    delete roomService;
}

void LiveDemo::createRoom(const LRAuthData & authData, const QString & serverDomain, const std::string& ip, unsigned short port, const QString & roomID, const QString & roomInfo, bool record, int picture_id)
{
    LiveRoom::instance()->setProxy(ip, port);
    m_imgDownloader.setProxy(ip, port);

    m_bCreate = true;
    m_roomID = roomID.toStdString();
    m_roomInfo = roomInfo;

    BoardService::instance().setRoomID(roomID.toStdString());
    if (record)
    {
        LiveRoom::instance()->recordVideo(picture_id);
    }

    LiveRoom::instance()->login(serverDomain.toStdString(), authData, this);

    init(authData, roomInfo, ip, port);
}

void LiveDemo::enterRoom(const LRAuthData & authData, const QString & serverDomain, const std::string& ip, unsigned short port, const QString & roomID, const QString & roomInfo, bool record, int picture_id)
{
    LiveRoom::instance()->setProxy(ip, port);
    m_imgDownloader.setProxy(ip, port);

    m_bCreate = false;
    m_roomID = roomID.toStdString();
    m_roomInfo = roomInfo;

    BoardService::instance().setRoomID(roomID.toStdString());
    if (record)
    {
        LiveRoom::instance()->recordVideo(picture_id);
    }

    LiveRoom::instance()->login(serverDomain.toStdString(), authData, this);

    init(authData, roomInfo, ip, port);
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

void LiveDemo::init(const LRAuthData& authData, const QString& roomName, const std::string& ip, unsigned short port)
{
    m_authData = authData;

    BoardAuthData boardAuthData;
    boardAuthData.sdkAppID = authData.sdkAppID;
    boardAuthData.token = authData.token;
    boardAuthData.userID = authData.userID;

    BoardService::instance().init(boardAuthData, ip, port);

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

void LiveDemo::syncSketchpad()
{
    Json::Value root;
    root["type"] = "request";
    root["action"] = "currentPPT";

    Json::FastWriter writer;
    std::string msg = writer.write(root);

    LiveRoom::instance()->sendC2CCustomMsg(m_authData.userID.c_str(), "sketchpad", msg.c_str());
}

void LiveDemo::onCreateRoom(const LRResult& res, const std::string& roomID)
{
    if (LIVEROOM_SUCCESS != res.ec)
    {
        emit dispatch([this, res] {
            LiveRoom::instance()->stopLocalPreview();

            QString msgContent = QString("[%1] %2").arg(res.ec).arg(QString::fromUtf8(res.msg.c_str()));
            DialogMessage::exec(msgContent, DialogMessage::OK);
        });

        Application::instance().pushRoomStatus(-1004, Wide2UTF8(L"创建房间失败"));
    }
    else
    {
        emit dispatch([this, roomID] {
            LiveRoom::instance()->setBeautyStyle(LIVEROOM_BEAUTY_STYLE_NATURE, 5, 5);

            m_roomID = roomID.c_str();
            BoardService::instance().setRoomID(roomID);
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

            QString msgContent = QString("[%1] %2").arg(res.ec).arg(QString::fromUtf8(res.msg.c_str()));
            DialogMessage::exec(msgContent, DialogMessage::OK);
        });

        Application::instance().pushRoomStatus(-1006, Wide2UTF8(L"进入房间失败"));
    }
    else
    {
        emit dispatch([this] {
            LiveRoom::instance()->setBeautyStyle(LIVEROOM_BEAUTY_STYLE_NATURE, 5, 5);

            m_mainPanel->initShowVideo();

            syncSketchpad();
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
            QString msgContent = QString("[%1] %2").arg(res.ec).arg(QString::fromUtf8(res.msg.c_str()));
            DialogMessage::exec(msgContent, DialogMessage::OK);
        }
        else
        {
			static int init = 0;
			if (m_mainPanel && m_authData.userID != roomData.roomCreator && init == 0)
			{
				m_mainPanel->startAudiencePlay();
				init++;
			}

            if (roomData.roomCreator == m_authData.userID)
            {
                QString nickName = m_authData.userName.c_str();
                nickName.append("(" + m_userTag + ")");
                if (m_imPanel)
                {
                    m_imPanel->setNickName(nickName);
                }
            }
			std::string creatorName;
			for (int i = 0; i < roomData.members.size(); i++)
			{
				if (roomData.roomCreator == roomData.members[i].userID)
				{
					creatorName = roomData.members[i].userName;
				}
			}

            m_roomID = roomData.roomID;
            m_mainPanel->setRoomCreator(roomData.roomCreator, creatorName);
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
        if (m_bCreate)
        {
            QString tip = qMember.userName.c_str() + QStringLiteral("退出连麦");

            m_toast->setDuration(3000);
            m_toast->setText(tip);
        }

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
        killTimer(m_audienceListTimerID);

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
    emit dispatch([this] {
        DialogMessage::exec(QStringLiteral("IM强制下线，请检查是否重复登录同一个账号"), DialogMessage::OK);

        if (m_screenRecord != RecordScreenNone)
        {
            TXCloudRecordCmd::instance().exit();
        }

        Application::instance().pushRoomStatus(2, Wide2UTF8(L"IM强制下线，退出房间"));

        LiveRoom::instance()->leaveRoom();
        LiveRoom::instance()->logout();

        onRoomClosed("");
    });
}

void LiveDemo::onError(const LRResult& res, const std::string& userID)
{
    QString msgContent = QString("[%1] %2").arg(res.ec).arg(QString::fromLocal8Bit(res.msg.c_str()));

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
    emit dispatch([this] {
        m_linkmicStatus = LinkMicOn;
        killTimer(m_linkmicTimerID);

        m_mainPanel->startSubPusherPreview();
        m_deviceManage->setDeviceManageEnable(true);
    });
}

void LiveDemo::onRecvRejectJoinPusher(const std::string& roomID, const std::string& msg)
{
    emit dispatch([this] {
        m_linkmicStatus = LinkMicOff;
        killTimer(m_linkmicTimerID);

        m_toast->setDuration(3000);
        m_toast->setText(QStringLiteral("连麦被拒绝"));
    });
}

void LiveDemo::onRecvKickoutSubPusher(const std::string& roomID)
{
    emit dispatch([this] {
        if (LinkMicOn == m_linkmicStatus)
        {
            LiveRoom::instance()->quitPusher();

            m_mainPanel->startAudiencePlay();
            m_deviceManage->setDeviceManageEnable(false);
        }

        m_linkmicStatus = LinkMicOff;

        m_toast->setDuration(3000);
        m_toast->setText(QStringLiteral("连麦被踢下线"));
    });
}

void LiveDemo::onLogin(const LRResult & res, const LRAuthData & authData)
{
    emit dispatch([=] {
        if (LIVEROOM_SUCCESS == res.ec)
        {
            if (m_bCreate)
            {
                LiveRoom::instance()->createRoom(m_roomID, m_roomInfo.toStdString());

                if (m_mainPanel)
                {
                    m_mainPanel->startCreatorPreview();
                    m_deviceManage->setDeviceManageEnable(true);
                }
            }
            else
            {
                LiveRoom::instance()->enterRoom(m_roomID);

                if (m_mainPanel)
                {
                    m_deviceManage->setDeviceManageEnable(false);
                }
            }

            m_audienceListTimerID = startTimer(5 * 1000);
        }
        else
        {
            LiveRoom::instance()->stopLocalPreview();

            QString msgContent = QString("[%1] %2").arg(res.ec).arg(QString::fromUtf8(res.msg.c_str()));
            DialogMessage::exec(msgContent, DialogMessage::OK);
        }
    });
}

void LiveDemo::onSendIMGroupMsg(const std::string & msg)
{
    LiveRoom::instance()->sendRoomTextMsg(msg.c_str());
}

void LiveDemo::initUI(const QString& strTemplate, const QString& userTag,
    bool bUserList, bool bIMList, bool whiteboard, bool screenShare, ScreenRecordType screenRecord)
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
    m_screenRecord = screenRecord;

    m_mainPanel = new LiveVideoPanel(ui.widget_main_share);
    QVBoxLayout *tabVBoxLayout = new QVBoxLayout(ui.widget_main_share);
    tabVBoxLayout->setMargin(0);
    tabVBoxLayout->addWidget(m_mainPanel);
    ui.widget_main_share->setLayout(tabVBoxLayout);
    m_mainPanel->initConfigSetting(DEFAULT_CAMERA_SIZE, whiteboard, screenShare, m_screenRecord);
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

    /*if (!bUserList && !bIMList)
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
    }*/

    m_toast = new QWidgetToast(this);

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

void LiveDemo::on_btn_device_manage_clicked()
{
    bool ret = m_deviceManage->updateUI(2);
    if (ret)
    {
        QWidget* display = m_deviceManage->getRenderWidget();
        LiveRoom::instance()->updateLocalPreview((HWND)display->winId(), RECT{ 0, 0, display->width(), display->height() });
    }

    m_deviceManage->exec();
}

void LiveDemo::on_device_manage_tab_changed(int tabIndex)
{
	bool ret = m_deviceManage->previewCamera();
    if (ret)
    {
        QWidget* display = m_deviceManage->getRenderWidget();
        LiveRoom::instance()->updateLocalPreview((HWND)display->winId(), RECT{ 0, 0, display->width(), display->height() });
    }
}

void LiveDemo::on_btn_beauty_manage_clicked()
{
    m_deviceManage->updateUI(0);

    m_deviceManage->exec();
}

void LiveDemo::on_record_manage_clicked()
{
    m_deviceManage->updateUI(1);

    m_deviceManage->exec();
}

void LiveDemo::on_btn_linkmic_clicked()
{
    if (LinkMicOff == m_linkmicStatus)
    {
        LiveRoom::instance()->requestJoinPusher();

        m_linkmicStatus = LinkMicBegin;
        m_linkmicTimerID = startTimer(10 * 1000);
    }
    else if (LinkMicOn == m_linkmicStatus)
    {
        LiveRoom::instance()->quitPusher();

        m_mainPanel->startAudiencePlay();
        if (m_deviceManage)
        {
            m_deviceManage->setDeviceManageEnable(false);
        }

        m_linkmicStatus = LinkMicOff;
    }
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

void LiveDemo::on_record_manage_ok()
{
    QSettings* setting = new QSettings(QCoreApplication::applicationDirPath() + "/record-config.ini", QSettings::IniFormat);
    setting->beginGroup("config");
    bool localRecord = setting->value("localRecord").toBool();
    bool serverRecord = setting->value("serverRecord").toBool();

    if (!localRecord && !serverRecord)
    {
        DialogMessage::exec(QStringLiteral("请先设置录制参数!"), DialogMessage::OK);
        return;
    }

    if (localRecord && serverRecord)
    {
        m_screenRecord = RecordScreenToBoth;
    }
    else if (localRecord)
    {
        m_screenRecord = RecordScreenToClient;
    }
    else
        m_screenRecord = RecordScreenToServer;

    QString qPath = setting->value("localRecordPath").toString();

    qPath.replace("/", "\\");
    if (qPath.lastIndexOf("\\") != qPath.length() - 1)
    {
        qPath.append("\\");
    }

    qPath.append(QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss"));
    qPath.append(".mp4");

    std::string localRecordPath = qPath.toStdString();
    std::string serverUrl = setting->value("serverUrl").toString().toStdString();
    int sliceTime = setting->value("sliceTime").toInt();
    setting->endGroup();

    RecordData recordData;
    ZeroMemory(&recordData, sizeof(recordData));
    //strcpy(recordData.recordExe, "TXCloudRoom.exe");
    strcpy(recordData.recordUrl, serverUrl.c_str());
    strcpy(recordData.recordPath, localRecordPath.c_str());
    recordData.recordType = m_screenRecord;
    recordData.sliceTime = sliceTime;
	recordData.winID = (int)GetDesktopWindow();

    if (TXCloudRecordCmd::instance().isExist())
    {
        TXCloudRecordCmd::instance().update(recordData);
    }
    else
    {
        TXCloudRecordCmd::instance().cleanProcess();
        TXCloudRecordCmd::instance().runAndRecord(recordData);
    } 

    if (m_mainPanel)
    {
        m_mainPanel->on_startRecord(m_screenRecord);
    }
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
