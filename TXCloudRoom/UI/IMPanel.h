#pragma once

#include <QWidget>
#include "ui_IMPanel.h"
#include <memory>
#include "commonType.h"

typedef std::function<void(void)> txfunction;

class IIMPanelCallback
{
public:
	virtual ~IIMPanelCallback() {}
	virtual void onSendIMGroupMsg(const std::string& msg) = 0;
};

class IMPanel
	: public QWidget
{
	Q_OBJECT

public:
	IMPanel(IIMPanelCallback * callback, QWidget *parent = Q_NULLPTR);
	virtual ~IMPanel();

	void setUserId(const QString& id);
	void setRoomCreator(const QString& id);
	void setNickName(const QString& nick);

	void onRecvC2CTextMsg(const char * userId, const char * userName, const char * msg);
	void onRecvGroupTextMsg(const char * groupId, const char * userId, const char * userName, const char * msg);

    void onRoomClosed();
private:
	void addMsgContent(const QString& userId, const QString& userName, const QString& msg);
	void sendGroupMsg(const std::string& msg);
	void sendC2CMsg(const std::string& destUserId, const std::string& msg);

signals:
	void dispatch(txfunction func);     // 投递线程队列
	protected slots:
	void handle(txfunction func);       // 执行函数

	void onSendBtnClicked();
	void onEnterPress();
	void onCtrlEnterPress();
private:
	Ui::IMPanel ui;

	QString m_nickName;
	QString m_userID;
	QString m_roomCreator;
	QString m_userTag;
	IIMPanelCallback * m_callback;
};
