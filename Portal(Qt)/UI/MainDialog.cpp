#include "MainDialog.h"
#include "Application.h"
#include "jsoncpp/json.h"

#include <QMouseEvent>
#include <ctime>

MainDialog::MainDialog(QWidget *parent)
    : QDialog(parent)
    , m_csLiveSetting(nullptr)
    , m_liveRoomList(nullptr)
    , m_RTCRoomList(nullptr)
{
    ui.setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

MainDialog::~MainDialog()
{
    if (nullptr != m_csLiveSetting)
    {
        m_csLiveSetting->close();
        delete m_csLiveSetting;
        m_csLiveSetting = nullptr;
    }

    if (nullptr != m_liveRoomList)
    {
        m_liveRoomList->close();
        delete m_liveRoomList;
        m_liveRoomList = nullptr;
    }

    if (nullptr != m_RTCRoomList)
    {
        m_RTCRoomList->close();
        delete m_RTCRoomList;
        m_RTCRoomList = nullptr;
    }
}

void MainDialog::mousePressEvent(QMouseEvent *e)
{
    mousePressedPosition = e->globalPos();
    windowPositionAsDrag = pos();
}

void MainDialog::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);

    // 鼠标放开始设置鼠标按下的位置为 null，表示鼠标没有被按下
    mousePressedPosition = QPoint();
}

void MainDialog::mouseMoveEvent(QMouseEvent *e)
{
    if (!mousePressedPosition.isNull())
    {
        // 鼠标按下并且移动时，移动窗口, 相对于鼠标按下时的位置计算，是为了防止误差累积
        QPoint delta = e->globalPos() - mousePressedPosition;
        move(windowPositionAsDrag + delta);
    }
}

void MainDialog::on_btn_close_clicked()
{
    this->close();
}

void MainDialog::on_btn_video_clicked()
{
    // 标准直播

    this->close();

    Json::Value root;
    root["type"] = "NormalLive";
    root["action"] = "";
    root["port"] = ((::time(NULL) % 15000) + 50000);    // 建议范围限制[50000, 65535]

    Json::FastWriter writer;
    std::string jsonUTF8 = writer.write(root);

    Application::instance().openAndWait(jsonUTF8);
}

void MainDialog::on_btn_cslive_clicked()
{
    this->hide();

    // 客服通话

    if (nullptr == m_csLiveSetting)
    {
        m_csLiveSetting = new CSLiveSetting(this);
    }

    m_csLiveSetting->show();
}

void MainDialog::on_btn_liveroom_clicked()
{
    this->hide();

    if (nullptr == m_liveRoomList)
    {
        m_liveRoomList = new LiveRoomList(this);
    }

    m_liveRoomList->show();
}

void MainDialog::on_btn_custom_clicked()
{
    this->hide();

    if (nullptr == m_RTCRoomList)
    {
        m_RTCRoomList = new RTCRoomList(this);
    }

    m_RTCRoomList->setMulti(false);
    m_RTCRoomList->show();
}

void MainDialog::on_btn_multiroom_clicked()
{
    this->hide();

    if (nullptr == m_RTCRoomList)
    {
        m_RTCRoomList = new RTCRoomList(this);
    }

    m_RTCRoomList->setMulti(true);
    m_RTCRoomList->show();
}