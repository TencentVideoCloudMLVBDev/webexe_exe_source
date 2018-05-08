#include "CSLiveSetting.h"
#include "DialogMessage.h"
#include "jsoncpp/json.h"
#include "Application.h"

#include <QMouseEvent>
#include <ctime>

CSLiveSetting::CSLiveSetting(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint | windowFlags());
}

CSLiveSetting::~CSLiveSetting()
{
}

void CSLiveSetting::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_pressed = true;
		m_point = event->pos();
	}
}

void CSLiveSetting::mouseMoveEvent(QMouseEvent *event)
{
	if (m_pressed)
	{
		move(event->pos() - m_point + pos());
	}
}

void CSLiveSetting::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);

	m_pressed = false;
}

void CSLiveSetting::on_btn_close_clicked()
{
	close();
}

void CSLiveSetting::on_btn_ok_clicked()
{
	QString pushUrl = ui.edit_push_url->text().trimmed();
	if (pushUrl.isEmpty() || !pushUrl.toLower().contains("rtmp://"))
	{
		DialogMessage::exec(QStringLiteral("请输入推流地址!"), DialogMessage::OK, this);
		return;
	}

	QString playUrl = ui.edit_play_url->text().trimmed();
	if (playUrl.isEmpty() || !playUrl.toLower().contains("rtmp://"))
	{
		DialogMessage::exec(QStringLiteral("请输入播放地址!"), DialogMessage::OK, this);
		return;
	}

    QString pushURL = ui.edit_push_url->text().trimmed();
    QString playURL = ui.edit_play_url->text().trimmed();
    QString title = ui.edit_title->text().trimmed();

    Json::Value root;
    root["type"] = "CustomServiceLive";
    root["action"] = "start";
    root["pushURL"] = pushURL.toStdString();
    root["playURL"] = playUrl.toStdString();
    root["title"] = title.toStdString();
    root["port"] = ((::time(NULL) % 15000) + 50000);    // 建议范围限制[50000, 65535]

    Json::FastWriter writer;
    std::string jsonUTF8 = writer.write(root);

    close();

    Application::instance().openAndWait(jsonUTF8);
}

void CSLiveSetting::on_btn_cancel_clicked()
{
	close();
}
