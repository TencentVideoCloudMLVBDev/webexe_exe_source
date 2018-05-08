#pragma once

#include <QMainWindow>
#include "ui_PushPlayDemo.h"
#include "TXLivePlayer.h"
#include "TXLivePusher.h"
#include "TXLiveSDKEventDef.h"

enum PushPlayRatio
{
	RATIO_4_3 = 0,  
	RATIO_16_9 = 1,
	RATIO_9_16 = 2,
};

class PushPlayDemo : public QMainWindow
	, public TXLivePlayerCallback
	, public TXLivePusherCallback
{
	Q_OBJECT

public:
	PushPlayDemo(QString pushUrl = "", int cameraIndex = 0, int width = 640, int height = 480,
		int rotation = 0, int bitrate = 900, QWidget *parent = Q_NULLPTR);
	virtual ~PushPlayDemo();

    void setProxy(const std::string& ip, unsigned short port);

    void quit();
protected:
	void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
private:
	QPoint mousePressedPosition; // 鼠标按下时的坐标
	QPoint windowPositionAsDrag; // 鼠标按下时窗口左上角的坐标

	TXLivePlayer m_player;
	TXLivePusher m_pusher;
	bool m_pushing;
	bool m_playing;
	bool m_pushBegin;
	bool m_beauty;
	QString m_pushUrl;
	int m_cameraIndex;
	int m_height;
	int m_width;
	int m_rotation;
	int m_bitrate;
	int m_cameraCount;
	TXEVideoResolution m_resolution;
	PushPlayRatio m_ratio;
	int m_maxBitrate;
	int m_minBitrate;
public:
	//TXLivePlayerCallback
	//TXLivePusherCallback
	void onEventCallback(int eventId, const int paramCount, const char **paramKeys, const char **paramValues, void *pUserData) override;
protected:
	void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
signals:
	void update_connection(int status);
	void update_event(int eventId, QString paramKey, QString paramValue);

private slots:
	void on_btn_close_clicked();
	void on_btn_push_clicked();
	void on_btn_play_clicked();
	void on_btn_play_log_clicked();
	void on_chb_push_mirror_clicked();
	void on_chb_push_beauty_clicked();
	void on_update_connection(int status); //1.拉流连接失败 2.推流连接失败
	void on_cmb_cameras_activated();
	void on_cmb_windows_activated();
	void on_cmb_bitrate_activated();
	void on_cmb_rotation_activated();
	void on_cmb_render_activated();
	void on_cmb_resolution_activated();
	void on_cmb_FPS_activated();
	void on_btn_push_url_clicked();
	void on_update_event(int eventId, QString paramKey, QString paramValue);
	void on_getPushUrl_finished(int errorCode, QString errorInfo, QString pushUrl);

	void on_cmb_delay_min_activated();
	void on_cmb_delay_max_activated();

	void on_btn_process_clicked();
	void onOpenSystemVoiceSlots(int);

	void onBtnShotPusher();
	void onBtnShotPlayer();
private:
	HWND m_hChooseHwnd;
	HWND *m_hHwndList;
	int m_iHwndListCnt;
	bool m_bPusherScreenCapture;
	int m_qurityIndex;
	Ui::PushPlayDemo ui;
	void initUI();
	void on_stop_pusher();
	void initPushParam();
};
