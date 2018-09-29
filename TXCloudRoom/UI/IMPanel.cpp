#include "IMPanel.h"
#include "IMItemView.h"
#include <QScrollBar>
#include <QTime>

IMPanel::IMPanel(IIMPanelCallback * callback, QWidget *parent)
	: QWidget(parent)
	, m_nickName("")
	, m_userID("")
	, m_roomCreator("")
	, m_callback(callback)
{
	ui.setupUi(this);

	ui.lw_msg_list->setSelectionMode(QListWidget::NoSelection);

	// ÇÐ»»Ö÷Ïß³Ì
	qRegisterMetaType<txfunction>("txfunction");
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);

	connect(ui.btn_send, SIGNAL(clicked()), this, SLOT(onSendBtnClicked()));
	connect(ui.tb_msg, SIGNAL(enterPressed()), this, SLOT(onEnterPress()));
	connect(ui.tb_msg, SIGNAL(ctrlEnterPressed()), this, SLOT(onCtrlEnterPress()));

	const QString scrollBarStyle =
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
})";
	ui.tb_msg->verticalScrollBar()->setStyleSheet(scrollBarStyle);
	ui.lw_msg_list->verticalScrollBar()->setStyleSheet(scrollBarStyle);

    QTime now = QTime::currentTime();
    ui.groupBoxTime->setTitle(now.toString("hh:mm:ss"));
}

IMPanel::~IMPanel()
{
}

void IMPanel::setUserId(const QString& id)
{
	m_userID = id;
}

void IMPanel::setRoomCreator(const QString& id)
{
	m_roomCreator = id;
}

void IMPanel::setNickName(const QString & nick)
{
	m_nickName = nick;
}

void IMPanel::addMsgContent(const QString& userId, const QString& userName, const QString& msg)
{
	QString content;
	if (m_roomCreator == userId)
	{
		content = QString("<b>%1: %2</b>").arg(userName).arg(msg);
	}
	else
	{
		content = QString("<font color=\'blue\'>%1</font>: %2").arg(userName).arg(msg);
	}

	IMItemView *itemView = new IMItemView();
	itemView->setMsgContent(content);
	itemView->setFixedWidth(ui.lw_msg_list->width());

	QListWidgetItem *item = new QListWidgetItem();
	item->setSizeHint(itemView->size());
	ui.lw_msg_list->addItem(item);
	ui.lw_msg_list->setItemWidget(item, itemView);
	ui.lw_msg_list->scrollToItem(item);
}

void IMPanel::sendGroupMsg(const std::string& msg)
{
	m_callback->onSendIMGroupMsg(msg);
}

void IMPanel::sendC2CMsg(const std::string& destUserId, const std::string& msg)
{

}

void IMPanel::onRecvC2CTextMsg(const char * userId, const char * userName, const char * msg)
{
	if (!userId || !*userId || !msg || !*msg)
	{
		return;
	}

	QString msgUserId = userId;
	QString msgUserName = userName;
	QString msgContent = msg;
	emit dispatch([this, msgUserId, msgUserName, msgContent] {
		addMsgContent(msgUserId, msgUserName, msgContent);
	});
}

void IMPanel::onRecvGroupTextMsg(const char * groupId, const char * userId, const char * userName, const char * msg)
{
	if (!userId || !*userId || !msg || !*msg)
	{
		return;
	}

	QString msgUserId = userId;
	QString msgUserName = userName;
	QString msgContent = msg;
	emit dispatch([this, msgUserId, msgUserName, msgContent] {
		addMsgContent(msgUserId, msgUserName, msgContent);
	});
}

void IMPanel::onRoomClosed()
{
    ui.lw_msg_list->clear();
}

void IMPanel::handle(txfunction func)
{
	func();
}

void IMPanel::onSendBtnClicked()
{
	QString msg = ui.tb_msg->toPlainText();
	if (msg.isEmpty())
	{
		return;
	}

	addMsgContent(m_userID, m_nickName, msg);
	sendGroupMsg(msg.toStdString().c_str());

	ui.tb_msg->clear();
}

void IMPanel::onEnterPress()
{
	if (!ui.tb_msg->hasFocus())
	{
		return;
	}

	onSendBtnClicked();
}

void IMPanel::onCtrlEnterPress()
{
	if (!ui.tb_msg->hasFocus())
	{
		return;
	}

	QString msg = ui.tb_msg->toPlainText();
	msg.append("\n");

	ui.tb_msg->setPlainText(msg);
	ui.tb_msg->moveCursor(QTextCursor::End);
}

void IMPanel::resizeEvent(QResizeEvent* event)
{
	ui.lw_msg_list->setFixedWidth(this->geometry().width());
}