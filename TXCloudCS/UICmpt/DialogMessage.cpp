#include "DialogMessage.h"

#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>

DialogMessage::DialogMessage(QWidget *parent)
    : QDialog(parent)
    , m_pressed(false)
    , m_point()
    , m_timerID(0)
{
    ui.setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | windowFlags());
	setAttribute(Qt::WA_TranslucentBackground);

	QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
	shadowEffect->setOffset(0, 0);
	shadowEffect->setColor(Qt::gray);
	shadowEffect->setBlurRadius(5);
	ui.widgetMain->setGraphicsEffect(shadowEffect);
}

DialogMessage::~DialogMessage()
{

}

void DialogMessage::setTitle(const QString& title)
{
    ui.label_title->setText(title);
}

void DialogMessage::setDelayClose(int delayCloseTime)
{
    if (delayCloseTime > 0)
    {
        m_timerID = startTimer(delayCloseTime);
    }
}

DialogMessage::ResultCode DialogMessage::exec(const QString& title, int btns)
{
    DialogMessage msgBox;
    msgBox.ui.label_title->setText(title);
    msgBox.ui.btn_ok->setVisible(DialogMessage::OK == (DialogMessage::OK & btns));
    msgBox.ui.btn_cancel->setVisible(DialogMessage::CANCEL == (DialogMessage::CANCEL & btns));

    QDialog* dlg = static_cast<QDialog*>(&msgBox);

    return static_cast<DialogMessage::ResultCode>(dlg->exec());
}

DialogMessage::ResultCode DialogMessage::exec(const QString& title, int btns, int delayCloseMs)
{
    DialogMessage msgBox;
    msgBox.ui.label_title->setText(title);
    msgBox.ui.btn_ok->setVisible(DialogMessage::OK == (DialogMessage::OK & btns));
    msgBox.ui.btn_cancel->setVisible(DialogMessage::CANCEL == (DialogMessage::CANCEL & btns));
    msgBox.setDelayClose(delayCloseMs);

    QDialog* dlg = static_cast<QDialog*>(&msgBox);

    return static_cast<DialogMessage::ResultCode>(dlg->exec());
}

void DialogMessage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_pressed = true;
        m_point = event->pos();
    }
}

void DialogMessage::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed)
    {
        move(event->pos() - m_point + pos());
    }
}

void DialogMessage::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    m_pressed = false;
}

void DialogMessage::closeEvent(QCloseEvent *event)
{
    QDialog::closeEvent(event);

    done(DialogMessage::Rejected);
}

void DialogMessage::timerEvent(QTimerEvent *event)
{
    done(DialogMessage::Timeout);
}

void DialogMessage::on_btn_close_clicked()
{
    done(DialogMessage::Rejected);
}

void DialogMessage::on_btn_ok_clicked()
{
    done(DialogMessage::Accepted);
}

void DialogMessage::on_btn_cancel_clicked()
{
    done(DialogMessage::Rejected);
}
