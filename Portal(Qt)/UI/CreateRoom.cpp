#include "CreateRoom.h"
#include <QGraphicsDropShadowEffect>
#include <QMouseEvent>
#include <cstdlib>
#include <ctime>
#include "DialogMessage.h"

CreateRoom::CreateRoom(QWidget *parent)
    : QDialog(parent)
    , m_pressed(false)
    , m_point()
    , m_func()
    , m_roomName("")
{
    ui.setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setOffset(0, 0);
    shadowEffect->setColor(Qt::gray);
    shadowEffect->setBlurRadius(5);
    ui.widgetMain->setGraphicsEffect(shadowEffect);

    connect(ui.btn_close, SIGNAL(clicked()), this, SLOT(onCloseBtnClicked()));
    connect(ui.btn_operate, SIGNAL(clicked()), this, SLOT(onOperateBtnClicked()));
    connect(ui.btn_cancel, SIGNAL(clicked()), this, SLOT(onCancelBtnClicked()));
}

CreateRoom::~CreateRoom()
{

}

void CreateRoom::setLogo(QString styleSheet)
{
    ui.title->setStyleSheet(styleSheet);
}

void CreateRoom::setHanleFunction(handlefunction func)
{
    m_func = func;
}

void CreateRoom::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_pressed = true;
        m_point = event->pos();
    }
}

void CreateRoom::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed)
    {
        move(event->pos() - m_point + pos());
    }
}

void CreateRoom::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    m_pressed = false;
}

void CreateRoom::keyPressEvent(QKeyEvent *event)
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
    }
}

void CreateRoom::onCloseBtnClicked()
{
    if (m_func)
    {
        m_func(false, "");
    }

    this->close();
}

void CreateRoom::onOperateBtnClicked()
{
    QString courtName = ui.tb_court_name->text();

    if (courtName.isEmpty())
    {
        DialogMessage msgBox;
        msgBox.exec(QStringLiteral("请先输入房间名称!"), DialogMessage::OK, this);
        return;
    }

    this->close();

    if (m_func)
    {
        m_func(true, courtName.toStdString());
    }
}

void CreateRoom::onCancelBtnClicked()
{
    onCloseBtnClicked();
}
