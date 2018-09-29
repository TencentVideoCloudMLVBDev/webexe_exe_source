#pragma once

#include <QDialog>
#include "ui_DialogPushPlay.h"
#include "TXLivePlayer.h"
#include "TXLivePusher.h"
#include "TXLiveSDKEventDef.h"
#include <set> 
#include <string>
#include <mutex>
#include <functional>
#include "BeautyManage.h"

typedef std::function<void(void)>     txfunction;

class DialogPushPlay : public QDialog
	, public TXLivePlayerCallback
	, public TXLivePusherCallback
{
	Q_OBJECT

public:
	DialogPushPlay(bool top_window, QWidget *parent = Q_NULLPTR);
	~DialogPushPlay();
	void creatsession(QString pushUrl, QString playUrl);
	void destroysession();
	void startPush(QString url);
	void startPlay(QString url);
	void setTitle(const QString& title);
    void setLogo(const QString& logo);
	void setMute(const bool bMute);
    void setProxy(const std::string& ip, unsigned short port);
	void snapShotPlayer(const QString& path);
	void snapShotPusher(const QString& path);
	bool addPlayer(QString& url);
	bool delPlayer(QString& url);
    
public:
	virtual void onEventCallback(int eventId, const int paramCount, const char **paramKeys, const char **paramValues, void *pUserData) override;

protected:
	void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	bool eventFilter(QObject* pObj, QEvent* pEvent) Q_DECL_OVERRIDE;
	void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
	void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

signals:
	void update_event(int status, int index);
	void dispatch(txfunction func);

protected slots:
	void TicketTimeout();

private slots:
	void on_btn_close_clicked();
	void on_btn_min_clicked();
	void on_update_event(int status, int index); //1.拉流连接失败 2.推流连接失败 3.摄像头被占用 4.拉流开始 5.推流开始 6.摄像头关闭
	void handle(txfunction func);
	void on_btn_beauty_manage_clicked();
	void on_beauty_manage_ok(int beautyStyle, int beautyLevel, int whitenessLevel);
	void on_checkbox_mute_clicked();

private:
	void stopPush();
	void stopPlay();
	void stopAddPusher1(bool changeui =  true);
	void stopAddPusher2(bool changeui = true);
	void getImageBase64(std::string filePath, std::string& base64Buff);
	int  comparePlayerUrl(QString& url, int& nPlayer, bool addOpt = false);	//0 无任何匹配，  1-player1匹配， 2-player2匹配。
	QString parseUrlKey(QString& url);

	QPoint mousePressedPosition;
	QPoint windowPositionAsDrag;

	TXLivePlayer m_player;
	TXLivePusher m_pusher;
	bool m_pushing;
	bool m_playing;
	int m_cameraCount;
	bool m_pushBegin;

	BeautyManage * m_beautyManage = nullptr;

	bool m_bUserIsResizing;
	bool m_bTopWindow;
	Ui::DialogPushPlay ui;

    std::wstring m_pusherSnapshotPath;
    std::wstring m_playerSnapshotPath;
    std::string m_pusherURL;
    std::string m_playerURL;
	std::mutex m_mutex;

	TXLivePlayer m_addPlayer1;
	TXLivePlayer m_addPlayer2;
	std::string m_addPlayerUrl1;
	std::string m_addPlayerUrl2;
	int		_hidePlayerTicket1 = -1;
	int		_hidePlayerTicket2 = -1;
	std::map<int, QString> m_mapUrlKey;


	QTimer *_sessionTicketTimer = nullptr;
	bool m_bMutePusher = false;
	uint32_t m_curSessionTicket = 0;
};
