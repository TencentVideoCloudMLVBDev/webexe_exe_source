#include "LiveRoomList.h"
#include "RoomItemView.h"
#include "log.h"
#include "DialogMessage.h"
#include "Application.h"
#include "jsoncpp/json.h"

#include <QListWidget>
#include <QMouseEvent>
#include <QScrollBar>
#include <QtCore>
#include <QGraphicsDropShadowEffect>
#include <ctime>

#define DEFAULT_UTIL_HOST     "https://room.qcloud.com/weapp/utils"
#define DEFAULT_LIVEROOM_HOST "https://room.qcloud.com/weapp/live_room"

static QString nickName[4] = { QString::fromWCharArray(L"虞姬"), QString::fromWCharArray(L"嬴政")
                                , QString::fromWCharArray(L"百里守约"), QString::fromWCharArray(L"诸葛亮") };

LiveRoomList::LiveRoomList(QWidget *parent)
    : QDialog(parent)
    , m_pressed(false)
    , m_point()
    , m_createRoom(nullptr)
    , m_authData()
    , m_httpRequest("http://xzb.qcloud.com:8080/roomlist/weapp/webexe_room/")
    , m_roomID("")
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

    m_ui.btn_join->hide();

    m_ui.btn_create->setEnabled(false);
    m_ui.btn_join->setEnabled(false);

    // 切换到主线程
    qRegisterMetaType<LiveRoomListfunction>("LiveRoomListfunction");
    connect(this, SIGNAL(dispatch(LiveRoomListfunction)), this, SLOT(handle(LiveRoomListfunction)), Qt::QueuedConnection);

    // 生成userID
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    m_userID = QString("%1%2").arg("WinUser_Cpp_").arg(qrand() % 100000).toStdString();

    getLoginInfo(m_userID);

    getRoomList(0, 20);
    m_listTimerID = startTimer(3 * 1000);

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

LiveRoomList::~LiveRoomList()
{
    m_httpRequest.close();
}

void LiveRoomList::handle(LiveRoomListfunction func)
{
    func();
}

void LiveRoomList::onCloseBtnClicked()
{
    killTimer(m_listTimerID);

    this->close();
}

void LiveRoomList::onJoinBtnClicked()
{
    if (m_ui.lw_room_list->count() == 0)
    {
        DialogMessage::exec(QStringLiteral("暂时没有课堂，请先创建房间!"), DialogMessage::OK, this);
        return;
    }

    QListWidgetItem * curentItem = m_ui.lw_room_list->currentItem();
    if (nullptr == curentItem)
    {
        DialogMessage::exec(QStringLiteral("请先选择房间!"), DialogMessage::OK, this);
        return;
    }

    RoomItemView* itemView = (RoomItemView*)(m_ui.lw_room_list->itemWidget(curentItem));

    // 进入房间

    QString domain = DEFAULT_LIVEROOM_HOST;
    QString title = QStringLiteral("互动课堂");
	QString userTag = QStringLiteral("教师");

    Json::Value root;
    root["type"] = "LiveRoom";
    root["action"] = "enterRoom";
    root["serverDomain"] = domain.toStdString();
    root["sdkAppID"] = m_authData.sdkAppID;
    root["accountType"] = m_authData.accountType;
    root["userID"] = m_authData.userID;
    root["userSig"] = m_authData.userSig;
    root["userName"] = m_authData.userName;
    root["userAvatar"] = m_authData.userAvatar;
    root["roomID"] = itemView->roomID().toStdString();
    root["roomInfo"] = itemView->roomName().toStdString();
    root["title"] = title.toStdString();
    root["logo"] = "http://liteavsdk-1252463788.cosgz.myqcloud.com/windows/Cpp/logo/liveroom_logo.png";
    root["port"] = ((::time(NULL) % 15000) + 50000);    // 建议范围限制[50000, 65535]
	root["template"] = "1v4";
	root["userTag"] = userTag.toStdString();
	root["whiteboard"] = true;
	root["screenShare"] = true;
	root["record"] = true;

    Json::FastWriter writer;
    std::string jsonUTF8 = writer.write(root);

    onCloseBtnClicked();

    Application::instance().openAndWait(jsonUTF8);
}

void LiveRoomList::onCreateBtnClicked()
{
    killTimer(m_listTimerID);

    this->hide();

    killTimer(m_listTimerID);

    this->hide();

    if (nullptr == m_createRoom)
    {
        m_createRoom = new CreateRoom(this);
    }

    QApplication::setQuitOnLastWindowClosed(false);

    m_createRoom->setLogo("image: url(:/Portal(Qt)/logo-edu.png);");
    m_createRoom->setHanleFunction([=](bool created, const std::string& roomName) {
        if (false == created)
        {
            QApplication::setQuitOnLastWindowClosed(true);
            onCloseBtnClicked();
        }
        else
        {
            // 创建房间

            createRoom("", roomName, "webexe_liveroom");
        }
    });

    m_createRoom->show();
}

void LiveRoomList::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_pressed = true;
        m_point = event->pos();
    }
}

void LiveRoomList::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed)
    {
        move(event->pos() - m_point + pos());
    }
}

void LiveRoomList::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    m_pressed = false;
}

void LiveRoomList::keyPressEvent(QKeyEvent *event)
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

void LiveRoomList::timerEvent(QTimerEvent *event)
{
    getRoomList(0, 20);
}

void CALLBACK LiveRoomList::onTimerEvent(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    LiveRoomList* impl = reinterpret_cast<LiveRoomList*>(dwUser);
    if (impl)
    {
        impl->m_httpRequest.heartbeat(impl->m_roomID, "webexe_liveroom", [=](const Result& res) {});
    }
}

void LiveRoomList::getLoginInfo(const std::string& userID)
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

void LiveRoomList::createRoom(const std::string& roomID, const std::string& roomInfo, const std::string& roomType)
{
    m_httpRequest.createRoom("", m_userID, roomInfo, roomType, [=](const Result& res, const std::string& roomID) {
        emit dispatch([=] {
            if (ROOM_SUCCESS != res.ec)
            {
                DialogMessage::exec(QString::fromStdString(res.msg), DialogMessage::OK, this);
            }
            else
            {
                QString domain = DEFAULT_LIVEROOM_HOST;
                QString title = QStringLiteral("互动课堂");
				QString userTag = QStringLiteral("教师");

                Json::Value root;
                root["type"] = "LiveRoom";
                root["action"] = "createRoom";
                root["serverDomain"] = domain.toStdString();
                root["sdkAppID"] = m_authData.sdkAppID;
                root["accountType"] = m_authData.accountType;
                root["userID"] = m_authData.userID;
                root["userSig"] = m_authData.userSig;
                root["userName"] = m_authData.userName;
                root["userAvatar"] = m_authData.userAvatar;
                root["roomID"] = roomID;    // 后台生成roomID
                root["roomInfo"] = roomInfo;
                root["title"] = title.toStdString();
                root["logo"] = "http://liteavsdk-1252463788.cosgz.myqcloud.com/windows/Cpp/logo/liveroom_logo.png";
                root["port"] = ((::time(NULL) % 15000) + 50000);    // 建议范围限制[50000, 65535]
				root["template"] = "1v4";
				root["userTag"] = userTag.toStdString();
				root["whiteboard"] = true;
				root["screenShare"] = true;
				root["record"] = true;

                Json::FastWriter writer;
                std::string jsonUTF8 = writer.write(root);

                QApplication::setQuitOnLastWindowClosed(true);
                onCloseBtnClicked();

                m_roomID = roomID;
                m_hearbeatTimerID = ::timeSetEvent(10000, 1, onTimerEvent, (DWORD_PTR)this, TIME_PERIODIC | TIME_CALLBACK_FUNCTION); // 开启心跳定时器

                Application::instance().openAndWait(jsonUTF8);

                ::timeKillEvent(m_hearbeatTimerID);

                m_httpRequest.destroyRoom(roomID, roomType, [=](const Result& res) {});
            }
        });
    });
}

void LiveRoomList::getRoomList(int index, int cnt)
{
    m_httpRequest.getRoomList(index, cnt, "webexe_liveroom", [=](const Result& res, const std::vector<RoomData>& roomList) {
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
