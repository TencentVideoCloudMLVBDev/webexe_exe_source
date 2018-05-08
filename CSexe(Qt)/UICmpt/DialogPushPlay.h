#pragma once

#include <QDialog>
#include "ui_DialogPushPlay.h"
#include "TXLivePlayer.h"
#include "TXLivePusher.h"
#include "TXLiveSDKEventDef.h"
#include "ipchandshake.h"
#include "ipcconnection.h"
#include<set> 
#include <functional>

typedef std::function<void(void)>     txfunction;

class DialogPushPlay : public QDialog
	, public TXLivePlayerCallback
	, public TXLivePusherCallback
	, public IIPCConnectionCallback
	, public IIPCHandShakeCallback
{
	Q_OBJECT

public:
	DialogPushPlay(QWidget *parent = Q_NULLPTR);
	~DialogPushPlay();
	void onRecvCmd(const std::string& json);

public:
	virtual void onEventCallback(int eventId, const int paramCount, const char **paramKeys, const char **paramValues, void *pUserData) override;

	virtual void onClose(IPCConnection* connection);
	virtual DWORD onRecv(IPCConnection* connection, const void* data, size_t dataSize);
	virtual void onLog(LogLevel level, const char* content);

	virtual IPCConnection* onCreateBegin();
	virtual void onCreateEnd(bool success, IPCConnection* connection);

protected:
	void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	bool eventFilter(QObject* pObj, QEvent* pEvent) Q_DECL_OVERRIDE;
	void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

signals:
	void update_event(int status);
	void dispatch(txfunction func);

private slots:
	void on_btn_close_clicked();
	void on_btn_min_clicked();
	void on_update_event(int status); //1.拉流连接失败 2.推流连接失败 3.摄像头被占用 4.拉流开始 5.推流开始
	void handle(txfunction func);

private:
	void send(QString data, size_t dataSize, DWORD timeout);
	void on_startPush(QString id, QString url);
	void on_startPlay(QString id, QString url);
	void on_stopPush();
	void on_stopPlay();
	void on_setTitle(QString title);

	QPoint mousePressedPosition;
	QPoint windowPositionAsDrag;

	TXLivePlayer m_player;
	TXLivePusher m_pusher;
	bool m_pushing;
	bool m_playing;
	QString m_playID;
	QString m_pushID;
	int m_cameraCount;
	bool m_pushBegin;

	std::set<IPCConnection*> m_setIpc;
	IPCHandShake m_handShake;
	IPCConnection* m_activeConnect;

	bool m_bUserIsResizing;
	Ui::DialogPushPlay ui;
};
