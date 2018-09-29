#include "RTCRoomList.h"
#include "RoomItemView.h"
#include "log.h"
#include "DialogMessage.h"
#include "Application.h"

#include <QListWidget>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtCore>
#include <QGraphicsDropShadowEffect>
#include <ctime>

#define DEFAULT_MULTIROOM_HOST "https://room.qcloud.com/weapp/multi_room"
#define DEFAULT_DOUBLEROOM_HOST "https://room.qcloud.com/weapp/double_room"
#define MULTIROOM_MAX 4
#define DOUBLEROOM_MAX 2

static QString nickName[4] = { QString::fromWCharArray(L"虞姬"), QString::fromWCharArray(L"嬴政")
    , QString::fromWCharArray(L"百里守约"), QString::fromWCharArray(L"诸葛亮") };

RTCRoomList::RTCRoomList(QWidget *parent)
    : QDialog(parent)
    , m_pressed(false)
    , m_point()
    , m_createRoom(nullptr)
    , m_multi(false)
    , m_authData()
    , m_httpRequest("http://xzb.qcloud.com/roomlist/weapp/webexe_room/")
    , m_roomID("")
    , m_roomType("webexe_multiroom")
    , m_listTimerID(-1)
    , m_hearbeatTimerID(0)
{
    m_ui.setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setOffset(0, 0);
    shadowEffect->setColor(Qt::gray);
    shadowEffect->setBlurRadius(5);
    m_ui.widgetMain->setGraphicsEffect(shadowEffect);

    connect(m_ui.btn_close, SIGNAL(clicked()), this, SLOT(onCloseBtnClicked()));
    connect(m_ui.btn_join, SIGNAL(clicked()), this, SLOT(onJoinBtnClicked()));
    connect(m_ui.btn_create, SIGNAL(clicked()), this, SLOT(onCreateBtnClicked()));

    m_ui.widge_no_room->setHidden(true);
    m_ui.btn_create->setEnabled(false);
    m_ui.btn_join->setEnabled(false);

    // 切换到主线程
    qRegisterMetaType<RTCRoomListfunction>("RTCRoomListfunction");
    connect(this, SIGNAL(dispatch(RTCRoomListfunction)), this, SLOT(handle(RTCRoomListfunction)), Qt::QueuedConnection);

    // 生成userID
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
	m_userID = QString("%1%2").arg("WinUser_Cpp_").arg(qrand() % 100000).toStdString();

    getLoginInfo(m_userID);

    getRoomList(0, 20);
    m_listTimerID = startTimer(5 * 1000);

    m_ui.lw_room_list->verticalScrollBar()->setStyleSheet(
        R"(QScrollBar{
    background: transparent;
	 width: 7px;
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
})");
}

RTCRoomList::~RTCRoomList()
{
    m_httpRequest.close();
}

void RTCRoomList::setMulti(bool multi)
{
    m_multi = multi;
    if (true == m_multi)
    {
        m_roomType = "webexe_multiroom";
        m_ui.title->setStyleSheet("image: url(:/Portal(Qt)/logo-court.png);");
    }
    else
    {
        m_roomType = "webexe_doubleroom";
        m_ui.title->setStyleSheet("image: url(:/Portal(Qt)/logo-cs.png);");
    }
}

void RTCRoomList::handle(RTCRoomListfunction func)
{
    func();
}

void RTCRoomList::onCloseBtnClicked()
{
    killTimer(m_listTimerID);

    this->close();
}

void RTCRoomList::onJoinBtnClicked()
{
    if (m_ui.lw_room_list->count() == 0)
    {
        DialogMessage::exec(QStringLiteral("暂时没有视频通话，请先创建房间!"), DialogMessage::OK, this);
        return;
    }

    QListWidgetItem * curentItem = m_ui.lw_room_list->currentItem();
    if (nullptr == curentItem)
    {
        DialogMessage::exec(QStringLiteral("请先选择房间!"), DialogMessage::OK, this);
        return;
    }

    RoomItemView* itemView = (RoomItemView*)(m_ui.lw_room_list->itemWidget(curentItem));
    if (true == m_multi)
    {
        if (itemView->memberNum() >= MULTIROOM_MAX)
        {
            DialogMessage::exec(QStringLiteral("房间人数超过限制，请选择其他房间!"), DialogMessage::OK, this);
            return;
        }
    }
    else
    {
        if (itemView->memberNum() >= DOUBLEROOM_MAX)
        {
            DialogMessage::exec(QStringLiteral("房间人数超过限制，请选择其他房间!"), DialogMessage::OK, this);
            return;
        }
    }

    // 进入房间
	std::string title = "";
	std::string logo = "";
	std::string domain = "";

	getConfigInfo(title, logo, domain);
  
	std::string strTemplate = (true == m_multi ? "1v4" : "1v1");
	QString userTag = (true == m_multi ? QStringLiteral("管理员") : QStringLiteral("客服"));

    Json::Value root;
    root["type"] = "RTCRoom";
    root["action"] = "enterRoom";
    root["serverDomain"] = domain;
    root["sdkAppID"] = m_authData.sdkAppID;
    root["accountType"] = m_authData.accountType;
    root["userID"] = m_authData.userID;
    root["userSig"] = m_authData.userSig;
    root["userName"] = m_authData.userName;
    root["userAvatar"] = m_authData.userAvatar;
    root["roomID"] = itemView->roomID().toStdString();
    root["roomInfo"] = itemView->roomName().toStdString();
    root["template"] = strTemplate;
	root["userTag"] = userTag.toStdString();
	root["whiteboard"] = true;
	root["screenShare"] = true;
    root["title"] = title;
    root["logo"] = logo;   // 建议从后台读取
    root["port"] = ((::time(NULL) % 15000) + 50000);    // 建议范围限制[50000, 65535]

    Json::FastWriter writer;
    std::string jsonUTF8 = writer.write(root);

    onCloseBtnClicked();

    Application::instance().openAndWait(jsonUTF8);
}

void RTCRoomList::onCreateBtnClicked()
{
    killTimer(m_listTimerID);

    this->hide();

    if (nullptr == m_createRoom)
    {
        m_createRoom = new CreateRoom(this);
    }

    QApplication::setQuitOnLastWindowClosed(false);

    QString logo = (true == m_multi ? "image: url(:/Portal(Qt)/logo-court.png);" : "image: url(:/Portal(Qt)/logo-cs.png);");
    m_createRoom->setLogo(logo);
    m_createRoom->setHanleFunction([=](bool created, const std::string& roomName) {
        if (false == created)
        {
            QApplication::setQuitOnLastWindowClosed(true);

            onCloseBtnClicked();
        }
        else
        {
            // 创建房间

            createRoom("", roomName, m_roomType);
        }
    });

    m_createRoom->show();
}

void RTCRoomList::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_pressed = true;
        m_point = event->pos();
    }
}

void RTCRoomList::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed)
    {
        move(event->pos() - m_point + pos());
    }
}

void RTCRoomList::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    m_pressed = false;
}

void RTCRoomList::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
    {
        onCloseBtnClicked();
    }
    break;
    default:
        QDialog::keyPressEvent(event);
        break;
    }
}

void RTCRoomList::timerEvent(QTimerEvent *event)
{
    getRoomList(0, 20);
}

void CALLBACK RTCRoomList::onTimerEvent(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    RTCRoomList* impl = reinterpret_cast<RTCRoomList*>(dwUser);
    if (impl)
    {
        impl->m_httpRequest.heartbeat(impl->m_roomID, impl->m_roomType, [=](const Result& res) {});
    }
}

void RTCRoomList::getLoginInfo(const std::string& userID)
{
    m_httpRequest.getLoginInfo(userID, [=](const Result& res, const AuthData& authData) {
        emit dispatch([=] {
            if (ROOM_SUCCESS != res.ec)
            {
                DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK, this);
            }
            else
            {
                m_authData = authData;

                // userName登录时得到，这样做比较合理
                qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
                m_authData.userName = nickName[qrand() % 4].toUtf8().data();
                m_authData.userAvatar = "Windows_avatar_Cpp";

                m_ui.btn_create->setEnabled(true);
            }
        });
    });
}

void RTCRoomList::createRoom(const std::string& roomID, const std::string& roomInfo, const std::string& roomType)
{
    m_httpRequest.createRoom( "", m_userID, roomInfo, roomType, [=](const Result& res, const std::string& roomID) {
        emit dispatch([=] {
            if (ROOM_SUCCESS != res.ec)
            {
                DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK, this);
            }
            else
            {
				std::string title = "";
				std::string logo = "";
				std::string domain = "";

				getConfigInfo(title, logo, domain);
				std::string strTemplate = (true == m_multi ? "1v4" : "1v1");
				QString userTag = (true == m_multi ? QStringLiteral("管理员") : QStringLiteral("客服"));

                Json::Value root;
                root["type"] = "RTCRoom";
                root["action"] = "createRoom";
                root["serverDomain"] = domain;
                root["sdkAppID"] = m_authData.sdkAppID;
                root["accountType"] = m_authData.accountType;
                root["userID"] = m_authData.userID;
                root["userSig"] = m_authData.userSig;
                root["userName"] = m_authData.userName;
                root["userAvatar"] = m_authData.userAvatar;
                root["roomID"] = roomID;
                root["roomInfo"] = roomInfo;
                root["template"] = strTemplate;
				root["userTag"] = userTag.toStdString();
				root["whiteboard"] = true;
				root["screenShare"] = true;
                root["title"] = title;
                root["logo"] = logo;
                root["port"] = ((::time(NULL) % 15000) + 50000);    // 建议范围限制[50000, 65535]

                Json::FastWriter writer;
                std::string jsonUTF8 = writer.write(root);

                QApplication::setQuitOnLastWindowClosed(true);
                onCloseBtnClicked();

                m_roomID = roomID;
                m_hearbeatTimerID = ::timeSetEvent(10000, 1, onTimerEvent, (DWORD_PTR)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION); // 开启心跳定时器

                Application::instance().openAndWait(jsonUTF8);

                ::timeKillEvent(m_hearbeatTimerID);

                m_httpRequest.destroyRoom(roomID, m_roomType, [=](const Result& res) { });
            }
        });
    });
}

void RTCRoomList::getRoomList(int index, int cnt)
{
    m_httpRequest.getRoomList(index, cnt, m_roomType, [=](const Result& res, const std::vector<RoomData>& roomList) {
        emit dispatch([=] {
            if (ROOM_SUCCESS != res.ec)
            {
                DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK, this);
            }
            else
            {
                m_ui.lw_room_list->clear();

                for (std::vector<RoomData>::const_iterator it = roomList.begin(); roomList.end() != it; ++it)
                {
                    RoomItemView *itemView = new RoomItemView(this);
                    itemView->setRoomID(it->roomID.c_str());
                    itemView->setRoomName(it->roomInfo.c_str());

                    QListWidgetItem *item = new QListWidgetItem();
                    item->setSizeHint(itemView->size());
                    m_ui.lw_room_list->addItem(item);
                    m_ui.lw_room_list->setItemWidget(item, itemView);
                }

                m_ui.widge_no_room->setHidden(m_ui.lw_room_list->count() > 0);
                m_ui.btn_join->setEnabled(true);
            }
        });
    });
}

void RTCRoomList::getConfigInfo(std::string & title, std::string & logo, std::string & domain)
{
	QString roomConfig = m_multi ? "multiRoom-config.ini" : "doubleRoom-config.ini";
	QFile file(roomConfig);

	QTextCodec *codec = QTextCodec::codecForName("UTF-8");

	if (!file.exists())
	{
		file.open(QIODevice::ReadWrite);
		file.close();
		QString defaultTilte = (true == m_multi ? QStringLiteral("多人视频") : QStringLiteral("双人视频"));
		QString defaultDomain = (true == m_multi ? DEFAULT_MULTIROOM_HOST : DEFAULT_DOUBLEROOM_HOST);
		title = defaultTilte.toStdString();
		logo = "http://liteavsdk-1252463788.cosgz.myqcloud.com/windows/Cpp/logo/liveroom_logo.png";
		domain = defaultDomain.toStdString();

		QSettings* setting = new QSettings(roomConfig, QSettings::IniFormat);
		setting->setIniCodec(codec);
		setting->beginGroup("config");
		setting->setValue("title", defaultTilte);
		setting->setValue("logo", "http://liteavsdk-1252463788.cosgz.myqcloud.com/windows/Cpp/logo/liveroom_logo.png");
		setting->setValue("domain", defaultDomain);
		setting->endGroup();
	}
	else
	{
		QSettings* setting = new QSettings(roomConfig, QSettings::IniFormat);
		setting->setIniCodec(codec);
		setting->beginGroup("config");
		title = setting->value("title").toString().toStdString();
		logo = setting->value("logo").toString().toStdString();
		domain = setting->value("domain").toString().toStdString();
		setting->endGroup();
	}
}
