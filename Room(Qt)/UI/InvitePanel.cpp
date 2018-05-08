#include "InvitePanel.h"

#include <QClipboard>
#include <QMouseEvent>

InvitePanel::InvitePanel(QWidget *parent)
    : QDialog(parent)
    , m_pressed(false)
    , m_point()
    , m_toast(nullptr)
{
    m_ui.setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | windowFlags());

    connect(m_ui.btn_close, SIGNAL(clicked()), this, SLOT(onCloseBtnClicked()));
    connect(m_ui.btn_copy_id, SIGNAL(clicked()), this, SLOT(onCopyIdBtnClicked()));
    connect(m_ui.btn_copy_url, SIGNAL(clicked()), this, SLOT(onCopyUrlBtnClicked()));
    connect(m_ui.btn_ok, SIGNAL(clicked()), this, SLOT(onOkBtnClicked()));
    connect(m_ui.btn_cancel, SIGNAL(clicked()), this, SLOT(onCancelBtnClicked()));
}

InvitePanel::~InvitePanel()
{

}

void InvitePanel::setRoomId(const QString& id)
{
    m_ui.tb_room_id->setText(id);
}

void InvitePanel::setJoinUrl(const QString& url)
{
    m_ui.tb_join_url->setText(url);
}

void InvitePanel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_pressed = true;
        m_point = event->pos();
    }
}

void InvitePanel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed)
    {
        move(event->pos() - m_point + pos());
    }
}

void InvitePanel::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    m_pressed = false;
}

void InvitePanel::onCloseBtnClicked()
{
    reject();
}

void InvitePanel::onCopyIdBtnClicked()
{

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_ui.tb_room_id->text());

    if (!m_toast)
    {
        m_toast = new QWidgetToast(this);
    }

    m_toast->setDuration(1000);
    m_toast->setText(QString::fromWCharArray(L"复制成功"));
}

void InvitePanel::onCopyUrlBtnClicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(m_ui.tb_join_url->text());

    if (!m_toast)
    {
        m_toast = new QWidgetToast(this);
    }

    m_toast->setDuration(1000);
    m_toast->setText(QString::fromWCharArray(L"复制成功"));
}

void InvitePanel::onOkBtnClicked()
{
    accept();
}

void InvitePanel::onCancelBtnClicked()
{
    reject();
}
