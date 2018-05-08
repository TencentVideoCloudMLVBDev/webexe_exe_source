#include "PushPlayDemo.h"
#include <QMouseEvent>
#include "DialogMessage.h"
#include "TXLiveSDKTypeDef.h"
#include "TXLiveSDKEventDef.h"
#include "log.h"
#include "Application.h"

#include <sstream>
#include <QFileDialog.h>
#include <QDesktopServices>
#include "GetPushUrlInfo.h"
#include "TXLiveCommon.h"

#include <shlobj.h>
#include <CommDlg.h>
#include "ActiveXUtil.h"
#include <sstream>
#include <tchar.h>
#include <string.h>
#include <windows.h>
#include <Shlwapi.h>

#pragma comment (lib,"Shlwapi.lib")

#define DEFAULT_HOST L"https://lvbcloud.com/weapp/multi_room"

int bitrateRange[2][3] = { {400,300,200},{ 900,600,400 } };
PushPlayDemo::PushPlayDemo(QString pushUrl, int cameraIndex, int width, int height,
	int rotation, int bitrate, QWidget *parent)
	: QMainWindow(parent),
	m_pushing(false),
	m_playing(false),
	m_pushUrl(pushUrl),
	m_cameraIndex(cameraIndex),
	m_width(width),
	m_height(height),
	m_rotation(rotation),
	m_bitrate(bitrate),
	m_resolution(TXE_VIDEO_RESOLUTION_640x480),
	m_ratio(RATIO_4_3),
	m_bPusherScreenCapture(false)
{
	ui.setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint);
   // setAttribute(Qt::WA_TranslucentBackground);

	m_player.setCallback(this, reinterpret_cast<void*>(1));
	m_pusher.setCallback(this, reinterpret_cast<void*>(2));

	m_hChooseHwnd = nullptr;
	m_hHwndList = nullptr;
	m_iHwndListCnt = 0;
	ui.cbOpenSystemVoice->setEnabled(false);
	m_qurityIndex = 0;
	m_maxBitrate = 400;
	m_minBitrate = 200;

	initUI();
	m_pushBegin = false; 
	connect(this, SIGNAL(update_connection(int)), this, SLOT(on_update_connection(int)));
	connect(this, SIGNAL(update_event(int, QString, QString)), this, SLOT(on_update_event(int, QString, QString)));

	connect(GetPushUrlInfo::shared_instance(), SIGNAL(getPushUrl_finished(int, QString, QString)),
		this, SLOT(on_getPushUrl_finished(int, QString, QString)));
	
	//connect(ui.cbOpenRecordPush, SIGNAL(stateChanged(int)), this, SLOT(onOpenScreenRecordPushSlots(int)));
	connect(ui.cbOpenSystemVoice, SIGNAL(stateChanged(int)), this, SLOT(onOpenSystemVoiceSlots(int)));

	connect(ui.btnShotPusher, SIGNAL(clicked()), this, SLOT(onBtnShotPusher()));
	connect(ui.btnShotPlayer, SIGNAL(clicked()), this, SLOT(onBtnShotPlayer()));
}

PushPlayDemo::~PushPlayDemo()
{
    m_player.setCallback(NULL, NULL);
    m_player.stopPlay();
    m_player.closeRenderFrame();

    m_pusher.setCallback(NULL, NULL);
    m_pusher.stopAudioCapture();
    m_pusher.stopPreview();
    m_pusher.stopPush();
}

void PushPlayDemo::setProxy(const std::string& ip, unsigned short port)
{
    if (false == ip.empty())
    {
        TXLiveCommon::getInstance()->setProxy(ip.c_str(), port);

        GetPushUrlInfo::shared_instance()->setProxy(ip, port);
    }
}

void PushPlayDemo::quit()
{
    on_btn_close_clicked();
}

void PushPlayDemo::showEvent(QShowEvent *event) 
{
	this->setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(event);
}

void PushPlayDemo::onEventCallback(int eventId, const int paramCount, const char ** paramKeys, const char ** paramValues, void * pUserData)
{
	const int index = reinterpret_cast<int>(pUserData);
	if (index == 2 && eventId != TXE_STATUS_UPLOAD_EVENT)
	{
		std::stringstream eventLog;
		eventLog << "receive sdk event: target: pusher, event:" << eventId;
		for (int i = 0; i < paramCount; ++i)
		{
			eventLog << " " << paramKeys[i] << "=" << paramValues[i];
		}
        LINFO(L"%s", QString::fromStdString(eventLog.str().c_str()).toStdWString().c_str());
	}
	else if (index == 1 && eventId != TXE_STATUS_DOWNLOAD_EVENT)
	{
		std::stringstream eventLog;
		eventLog << "receive sdk event: target: player, event:" << eventId;
		for (int i = 0; i < paramCount; ++i)
		{
			eventLog << " " << paramKeys[i] << "=" << paramValues[i];
		}
        LINFO(L"%s", QString::fromStdString(eventLog.str().c_str()).toStdWString().c_str());
	}

	switch (eventId)
	{
	case PlayEvt::PLAY_ERR_NET_DISCONNECT: ///< 三次重连失败，断开。
		emit update_connection(1);
		break;
	case PlayEvt::PLAY_EVT_PLAY_BEGIN: ///< 开始播放
		break;
	case PushEvt::PUSH_ERR_NET_DISCONNECT:
		emit update_connection(2);
		break;
	case PushEvt::PUSH_EVT_PUSH_BEGIN:
		m_pushBegin = true;
        break;
    case PushEvt::PUSH_ERR_CAMERA_OCCUPY:
        emit update_event(PushEvt::PUSH_ERR_CAMERA_OCCUPY, "", "");
        break;
    default:
        break;
	}
	if (eventId == TXE_STATUS_UPLOAD_EVENT && m_pushBegin)
	{
		for (int i = 0; i < paramCount; ++i)
		{
			emit update_event(eventId, QString(paramKeys[i]), QString(paramValues[i]));
		}
	}

    std::map<std::string, std::string> params;
    for (int i = 0; i < paramCount; ++i)
    {
        params.insert(std::pair<std::string, std::string>(paramKeys[i], paramValues[i]));
    }

    Application::instance().pushSDKEvent(eventId, params);
}

void PushPlayDemo::on_btn_push_clicked()
{
	m_pushBegin = false;
	if (!m_pushing)
	{
		QString pushUrl = ui.lineEdit_push->text().trimmed();
		if (pushUrl.isEmpty() || !pushUrl.contains("txSecret") || !pushUrl.toLower().contains("rtmp://"))
		{
			DialogMessage::exec(QStringLiteral("请输入合法的RTMP地址!"), DialogMessage::OK);
			return;
		}
		if (m_cameraCount <= 0)
		{
			m_cameraCount = m_pusher.enumCameras(); //重新检查摄像头
			if (m_cameraCount <= 0)
			{
				DialogMessage::exec(QStringLiteral("请先接入摄像头!"), DialogMessage::OK);
				return;
			}
		}

		int index_camera = ui.cmb_cameras->currentIndex();
		int index_window = ui.cmb_windows->currentIndex();
		if (index_camera == 0 && index_window == 0)
		{
			DialogMessage::exec(QStringLiteral("请先选择合法视频源"), DialogMessage::OK);
			return;
		}
		bool bCamera = false;
		if (index_camera > 0)
		{
			bCamera = true;
		}


        on_cmb_bitrate_activated();

        int index = ui.cmb_FPS->currentIndex();
        switch (index)
        {
        case 0:
            m_pusher.setVideoFPS(15);
            break;
        case 1:
            m_pusher.setVideoFPS(20);
            break;
        case 2:
            m_pusher.setVideoFPS(25);
            break;
        case 3:
            m_pusher.setVideoFPS(30);
            break;
        default:
            break;
        }
		if (bCamera)
		{
			m_cameraIndex = ui.cmb_cameras->currentIndex() - 1;
			m_qurityIndex = ui.cmb_bitrate->currentIndex();
			if (m_qurityIndex == 0)
			{
				if (m_minBitrate > m_maxBitrate)
				{
					m_maxBitrate = m_minBitrate;
					m_bitrate = (m_maxBitrate + m_minBitrate) / 2;

				}
				m_pusher.setAutoAdjustStrategy(TXE_AUTO_ADJUST_REALTIME_VIDEOCHAT_STRATEGY);
				m_pusher.setVideoResolution(m_resolution);
				m_pusher.setVideoBitRateMin(m_minBitrate);
				m_pusher.setVideoBitRateMax(m_maxBitrate);
			}
			else if (m_qurityIndex == 1)
				m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_SUPER_DEFINITION);
			else if (m_qurityIndex == 2)
				m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_HIGH_DEFINITION);
			else if (m_qurityIndex == 3)
				m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_STANDARD_DEFINITION);
			else if (m_qurityIndex == 4)
				m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_REALTIME_VIDEOCHAT);
			m_pusher.setRenderMode(TXE_RENDER_MODE_ADAPT);
			m_pusher.startPreview((HWND)ui.widget_video_push->winId(), RECT{ 0, 0, ui.widget_video_push->width(), ui.widget_video_push->height() }, m_cameraIndex);
			m_pusher.startPush(pushUrl.toLocal8Bit());
			m_pusher.startAudioCapture();
			ui.cbOpenSystemVoice->setCheckState(Qt::Unchecked);
			ui.cbOpenSystemVoice->setEnabled(true);
		}
		else
		{
			int left = ui.lineEdit_left->text().toInt();
			int right = ui.lineEdit_right->text().toInt();
			int top = ui.lineEdit_top->text().toInt();
			int bottom = ui.lineEdit_bottom->text().toInt();

			RECT captureRect = { left,top,right,bottom };
			RECT renderRect = { 0 };
			::GetWindowRect((HWND)ui.widget_video_push->winId(), &renderRect);
			m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_STILLIMAGE_DEFINITION);
			m_pusher.setVideoFPS(10);
			m_pusher.setScreenCaptureParam(m_hChooseHwnd, captureRect);
			m_pusher.startPreview(TXE_VIDEO_SRC_SDK_SCREEN, (HWND)ui.widget_video_push->winId(), renderRect);
			m_pusher.startPush(pushUrl.toLocal8Bit());
			m_pusher.startAudioCapture();

			//sdk内部默认是镜像模式（针对摄像头），但是录屏出来的源数据本来就是镜像模式。
			m_pusher.setRenderYMirror(false);
			m_pusher.setOutputYMirror(false);
			ui.cbOpenSystemVoice->setCheckState(Qt::Unchecked);
			ui.cbOpenSystemVoice->setEnabled(true);
		}

		initPushParam();
		ui.btn_push->setStyleSheet("background-image:url(:/PushPlayTest/suspend.png);");
		m_pushing = true;
		ui.widget_video_push->setUpdatesEnabled(false);
	}
	else
	{
        m_pusher.stopAudioCapture();
		m_pusher.stopPreview();
		m_pusher.stopPush();
		ui.widget_video_push->update();
		ui.widget_video_push->setUpdatesEnabled(true);
		ui.btn_push->setStyleSheet("background-image:url(:/PushPlayTest/start.png);");
		m_pushing = false;
		on_stop_pusher();
		ui.cbOpenSystemVoice->setEnabled(false);
	}
}

void PushPlayDemo::on_btn_play_clicked()
{
	if (!m_playing)
	{
		QString playUrl = ui.lineEdit_play->text().trimmed();
		if (playUrl.isEmpty() || !playUrl.toLower().contains("rtmp://"))
		{
			DialogMessage::exec(QStringLiteral("请输入合法的RTMP地址!"), DialogMessage::OK);
			return;
		}

        m_player.setRenderYMirror(false);
		m_player.setRenderMode(TXE_RENDER_MODE_ADAPT);
        m_player.setRenderFrame((HWND)ui.widget_video_play->winId(), RECT{ 0, 0, ui.widget_video_play->width(), ui.widget_video_play->height() });
		m_player.startPlay(playUrl.toLocal8Bit(), PLAY_TYPE_LIVE_RTMP_ACC);
		ui.btn_play->setStyleSheet("background-image:url(:/PushPlayTest/suspend.png);");
		m_playing = true;
		ui.widget_video_play->setUpdatesEnabled(false);
	}
	else
	{
        m_player.stopPlay();
		m_player.closeRenderFrame();
		ui.widget_video_play->update();
		ui.widget_video_play->setUpdatesEnabled(true);
		ui.btn_play->setStyleSheet("background-image:url(:/PushPlayTest/start.png);");
		m_playing = false;
	}
}

void PushPlayDemo::on_btn_play_log_clicked()
{
	QDesktopServices::openUrl(QUrl(QDir::currentPath()+"/demoLog", QUrl::TolerantMode));
}

void PushPlayDemo::on_chb_push_mirror_clicked()
{
	if (ui.chb_push_mirror->checkState() == Qt::CheckState::Unchecked) {
		m_pusher.setRenderYMirror(false);
		m_pusher.setOutputYMirror(false);
	}
	else {
		m_pusher.setRenderYMirror(true);
		m_pusher.setOutputYMirror(true);
	}
}

void PushPlayDemo::on_chb_push_beauty_clicked()
{
	if (ui.chb_push_beauty->checkState() == Qt::CheckState::Unchecked) {
		m_beauty = false;
		m_pusher.setBeautyStyle(TXE_BEAUTY_STYLE_NATURE, 0, 0);
	}
	else {
		m_beauty = true;
		m_pusher.setBeautyStyle(TXE_BEAUTY_STYLE_NATURE, 5, 5);
	}
}

void PushPlayDemo::on_update_connection(int status)
{
	switch (status)
	{
	case 1:
	{
		DialogMessage::exec(QStringLiteral("播放连接失败!"), DialogMessage::OK);
        m_player.stopPlay();
		m_player.closeRenderFrame();
		ui.widget_video_play->update();
		ui.widget_video_play->setUpdatesEnabled(true);
		ui.btn_play->setStyleSheet("background-image:url(:/PushPlayTest/start.png);");
		m_playing = false;
	}
	break;
	case 2:
	{
		DialogMessage::exec(QStringLiteral("推流连接失败!"), DialogMessage::OK);
        m_pusher.stopAudioCapture();
		m_pusher.stopPreview();
		m_pusher.stopPush();
		ui.widget_video_push->update();
		ui.widget_video_push->setUpdatesEnabled(true);
		ui.btn_push->setStyleSheet("background-image:url(:/PushPlayTest/start.png);");
		m_pushing = false;
		on_stop_pusher();
	}
	break;
	default:
		break;
	}
}

void PushPlayDemo::on_cmb_cameras_activated()
{
	int index = ui.cmb_cameras->currentIndex();
	m_bPusherScreenCapture = false;
	if (index == 0)
	{
		ui.cmb_windows->setEnabled(true);
		
		//重新刷新窗口列表
		ui.cmb_windows->clear();
		int iCntWnd = m_pusher.enumCaptureWindow();
		ui.cmb_windows->addItem(QString::fromStdWString(L"无"));
		ui.cmb_windows->addItem(QString::fromStdWString(L"录制指定区域"));
		if (iCntWnd > 0)
		{
			HWND *hwndList = new HWND[iCntWnd];
			m_pusher.enumCaptureWindow(hwndList, iCntWnd);
			m_iHwndListCnt = iCntWnd;
			m_hChooseHwnd = nullptr;
			m_hHwndList = hwndList;
			for (int i = 0; i < iCntWnd; ++i)
			{
				wchar_t szTitleName[MAX_PATH] = { 0 };
				GetWindowTextW(hwndList[i], szTitleName, MAX_PATH);
				ui.cmb_windows->addItem(QString::fromStdWString(szTitleName));
			}
		}
		ui.cmb_windows->setCurrentIndex(0);

	}
	else
	{
		ui.cmb_windows->setEnabled(false);
		ui.lineEdit_left->setEnabled(false);
		ui.lineEdit_right->setEnabled(false);
		ui.lineEdit_top->setEnabled(false);
		ui.lineEdit_bottom->setEnabled(false);

		m_pusher.switchCamera(index - 1);
		m_cameraIndex = index - 1;
	}
}

void PushPlayDemo::on_cmb_windows_activated()
{
	int index = ui.cmb_windows->currentIndex();
	ui.lineEdit_left->setEnabled(false);
	ui.lineEdit_right->setEnabled(false);
	ui.lineEdit_top->setEnabled(false);
	ui.lineEdit_bottom->setEnabled(false);
	if (index == 0)
	{
		ui.cmb_cameras->setEnabled(true);
		ui.cmb_cameras->setCurrentText(0);
		m_hChooseHwnd = nullptr;
	}
	
	else
	{
		if (index == 1)
		{
			ui.lineEdit_left->setEnabled(true);
			ui.lineEdit_right->setEnabled(true);
			ui.lineEdit_top->setEnabled(true);
			ui.lineEdit_bottom->setEnabled(true);
			m_hChooseHwnd = nullptr;
		}
		m_bPusherScreenCapture = true;
		ui.cmb_cameras->setEnabled(false);
		if (index > 1 && m_hHwndList && m_iHwndListCnt > 0 && index < m_iHwndListCnt + 2)
		{
			m_hChooseHwnd = m_hHwndList[index - 2];
		}
	}
}

void PushPlayDemo::on_cmb_bitrate_activated()
{
	int index = ui.cmb_bitrate->currentIndex();
	if (m_qurityIndex != index)
	{
		if (index == 0)
		{
			ui.cmb_resolution->setEnabled(true);
			ui.cmb_FPS->setEnabled(true);
			ui.cmb_resolution->setCurrentIndex(0);
			ui.cmb_delay_min->setEnabled(true);
			ui.cmb_delay_max->setEnabled(true);

			ui.cmb_delay_min->setCurrentIndex(0);
			ui.cmb_delay_max->setCurrentIndex(0);
		}
		else
		{
			if (index == 1)
			{
				ui.cmb_resolution->setCurrentIndex(7);
				ui.cmb_delay_min->setCurrentIndex(5);
				ui.cmb_delay_max->setCurrentIndex(6);
			}
			if (index == 2)
			{
				ui.cmb_resolution->setCurrentIndex(6);
				ui.cmb_delay_min->setCurrentIndex(4);
				ui.cmb_delay_max->setCurrentIndex(4);
			}
			if (index == 3)
			{
				ui.cmb_resolution->setCurrentIndex(5);
				ui.cmb_delay_min->setCurrentIndex(1);
				ui.cmb_delay_max->setCurrentIndex(2);
			}
			if (index == 4)
			{
				ui.cmb_resolution->setCurrentIndex(0);
				ui.cmb_delay_min->setCurrentIndex(0);
				ui.cmb_delay_max->setCurrentIndex(0);
			}
			ui.cmb_resolution->setEnabled(false);
			ui.cmb_FPS->setEnabled(false);
			ui.cmb_delay_min->setEnabled(false);
			ui.cmb_delay_max->setEnabled(false);
		}

		m_qurityIndex = index;
		int index_camera = ui.cmb_cameras->currentIndex();
		if (index_camera > 0)
		{
			if (m_qurityIndex == 0)
			{
				m_pusher.setAutoAdjustStrategy(TXE_AUTO_ADJUST_REALTIME_VIDEOCHAT_STRATEGY);
				m_pusher.setVideoResolution(m_resolution);
			}
			else if (m_qurityIndex == 1)
				m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_SUPER_DEFINITION);
			else if (m_qurityIndex == 2)
				m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_HIGH_DEFINITION);
			else if (m_qurityIndex == 3)
				m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_STANDARD_DEFINITION);
			else if (m_qurityIndex == 4)
				m_pusher.setVideoQualityParamPreset(TXE_VIDEO_QUALITY_REALTIME_VIDEOCHAT);
		}
	}

}
/*
void PushPlayDemo::on_cmb_bitrate_activated()
{
    // 根据不同分辨率设置不同清晰度对应的码率
    // 下面是一个粗略的算法，方便演示如何调整码率，建议不要采用，而是根据实际的应用场景下，选用合适的算法
    // 分辨率参数，宽度平方加上高度的平方，再开平方，即sqrt(w^2 + h^2)
    // 清晰度参数，超清为1.2，高清为0.8，标清为0.5
    // 限制最后的取值范围是[200,2200]

    // "超清", "高清", "标清"
    int bitrateIndex = ui.cmb_bitrate->currentIndex();
    double quality = 1.2;
    switch (bitrateIndex)
    {
    case 0:
        quality = 1.2;
        break;
    case 1:
        quality = 0.8;
        break;
    case 2:
        quality = 0.5;
        break;
    default:
        break;
    }

    // "320x240", "640x360", "640x480", "1280x720"
    int width = 640;
    int height = 480;
    int index = ui.cmb_resolution->currentIndex();
    PushPlayRatio ratio;
    switch (index)
    {
    case 0:
    {
        width = 320;
        height = 240;
    }
    break;
    case 1:
    {
        width = 640;
        height = 360;
    }
    break;
    case 2:
    {
        width = 640;
        height = 480;
    }
    break;
    case 3:
    {
        width = 1280;
        height = 720;
    }
    break;
    default:
        break;
    }

    uint bitrate = (uint)(::sqrt(width * width + height * height) * quality);
    if (bitrate < 200)
    {
        bitrate = 200;
    }

    if (bitrate > 2200)
    {
        bitrate = 2200;
    }

    m_pusher.setVideoBitRate(bitrate);
}*/

void PushPlayDemo::on_cmb_rotation_activated()
{
	int index = ui.cmb_rotation->currentIndex();
	m_rotation = index;
	switch (index)
	{
	case 0:
	{
		m_pusher.setRotation(TXE_VIDEO_ROTATION_NONE);
	}
		break;
	case 1:
	{
		m_pusher.setRotation(TXE_VIDEO_ROTATION_90);
	}
		break;
	case 2:
	{
		m_pusher.setRotation(TXE_VIDEO_ROTATION_180);
	}
		break;
	case 3:
	{
		m_pusher.setRotation(TXE_VIDEO_ROTATION_270);
	}
		break;
	default:
		break;
	}
}

void PushPlayDemo::on_cmb_render_activated()
{
	int index = ui.cmb_render->currentIndex();
	switch (index)
	{
	case 0:
	{
		m_pusher.setRenderMode(TXE_RENDER_MODE_ADAPT);
		m_player.setRenderMode(TXE_RENDER_MODE_ADAPT);
	}
		break;
	case 1:
	{
		m_pusher.setRenderMode(TXE_RENDER_MODE_FILLSCREEN);
		m_player.setRenderMode(TXE_RENDER_MODE_FILLSCREEN);
	}
		break;
	default:
		break;
	}
}

void PushPlayDemo::on_cmb_resolution_activated()
{
	int index = ui.cmb_resolution->currentIndex();
	PushPlayRatio ratio;
	switch (index)
	{
	case 0:{ m_resolution = TXE_VIDEO_RESOLUTION_320x240; ratio = RATIO_4_3; break; }
	case 1:{m_resolution = TXE_VIDEO_RESOLUTION_480x360;ratio = RATIO_4_3; break; }
	case 2: {m_resolution = TXE_VIDEO_RESOLUTION_640x480; ratio = RATIO_4_3; break; }
	case 3: { m_resolution = TXE_VIDEO_RESOLUTION_960x720; ratio = RATIO_4_3; break; }
	case 4: { m_resolution = TXE_VIDEO_RESOLUTION_320x180; ratio = RATIO_16_9; break; }
	case 5: {m_resolution = TXE_VIDEO_RESOLUTION_480x272; ratio = RATIO_16_9; break; }
	case 6: { m_resolution = TXE_VIDEO_RESOLUTION_640x360; ratio = RATIO_16_9; break; }
	case 7: {m_resolution = TXE_VIDEO_RESOLUTION_960x540; ratio = RATIO_16_9; break; }
	case 8:{m_resolution = TXE_VIDEO_RESOLUTION_180x320;ratio = RATIO_9_16;break; }
	case 9: { m_resolution = TXE_VIDEO_RESOLUTION_272x480; ratio = RATIO_9_16; break; }
	case 10: {m_resolution = TXE_VIDEO_RESOLUTION_360x640; ratio = RATIO_9_16; break; }
	case 11: { m_resolution = TXE_VIDEO_RESOLUTION_540x960; ratio = RATIO_9_16; break; }
	default:
		break;
	}
	if (m_pushing && ratio != m_ratio)
	{
		DialogMessage::exec(QStringLiteral("宽高比改变将在下次推流时生效!"), DialogMessage::OK);
		return;
	}
	else
	{
		m_pusher.setVideoResolution(m_resolution);
		m_ratio = ratio;
	}	
}

void PushPlayDemo::on_cmb_FPS_activated()
{
	int index = ui.cmb_FPS->currentIndex();
	switch (index)
	{
	case 0:
		m_pusher.setVideoFPS(15);
		break;
	case 1:
		m_pusher.setVideoFPS(20);
		break;
	case 2:
		m_pusher.setVideoFPS(25);
		break;
	case 3:
		m_pusher.setVideoFPS(30);
		break;
	default:
		break;
	}
	DialogMessage::exec(QStringLiteral("FPS设置将在下次推流时生效!"), DialogMessage::OK);
}

void PushPlayDemo::on_btn_push_url_clicked()
{
	GetPushUrlInfo::shared_instance()->getPushUrl();
}

void PushPlayDemo::on_update_event(int eventId, QString paramKey, QString paramValue)
{
	if (eventId == TXE_STATUS_UPLOAD_EVENT)
	{
		if (0 == paramKey.compare(NET_STATUS_NET_SPEED))
		{
			ui.label_up_speed->setText(QString(paramValue) + "kb/s");
		}
		if (0 == paramKey.compare(NET_STATUS_VIDEO_BITRATE))
		{
			ui.label_up_video_bitrate->setText(QString(paramValue) + "kb/s");
		}
		if (0 == paramKey.compare(NET_STATUS_AUDIO_BITRATE))
		{
			ui.label_up_audio_bitrate->setText(QString(paramValue) + "kb/s");
		}
		if (0 == paramKey.compare(NET_STATUS_CACHE_SIZE))
		{
			ui.label_up_video_queue->setText(QString(paramValue) + QStringLiteral("帧"));
		}
		if (0 == paramKey.compare(NET_STATUS_CODEC_CACHE))
		{
			ui.label_up_audio_queue->setText(QString(paramValue) + QStringLiteral("帧"));
		}
		if (0 == paramKey.compare(NET_STATUS_VIDEO_FPS))
		{
			ui.label_up_fps->setText(QString(paramValue));
		}
		if (0 == paramKey.compare(NET_STATUS_VIDEO_GOP))
		{
			ui.label_up_gop->setText(QString(paramValue) + "s");
		}
	}

    if (eventId == PushEvt::PUSH_ERR_CAMERA_OCCUPY)
    {
        DialogMessage::exec(QStringLiteral("摄像头已被占用!"), DialogMessage::OK);
    }
}

void PushPlayDemo::on_getPushUrl_finished(int errorCode, QString errorInfo, QString pushUrl)
{
	if (errorCode == 0) {
		ui.lineEdit_push->setText(pushUrl);
		ui.lineEdit_push->setCursorPosition(0);
		pushUrl.replace(QString("livepush"), QString("liveplay"));
		ui.lineEdit_play->setText(pushUrl);
		ui.lineEdit_play->setCursorPosition(0);
	}
	else
		DialogMessage::exec(errorInfo, DialogMessage::OK);
}

void PushPlayDemo::on_cmb_delay_min_activated()
{
	int index = ui.cmb_delay_min->currentIndex();
	switch (index)
	{
	case 0: { m_minBitrate = 100; break; }
	case 1: {m_minBitrate = 200; break; }
	case 2: {m_minBitrate = 300; break; }
	case 3: { m_minBitrate = 400; break; }
	case 4: { m_minBitrate = 500; break; }
	case 5: {m_minBitrate = 600; break; }
	default:
		break;
	}
}

void PushPlayDemo::on_cmb_delay_max_activated()
{
	int index = ui.cmb_delay_max->currentIndex();
	switch (index)
	{
	case 0: { m_maxBitrate = 400; break; }
	case 1: {m_maxBitrate = 500; break; }
	case 2: {m_maxBitrate = 600; break; }
	case 3: { m_maxBitrate = 800; break; }
	case 4: { m_maxBitrate = 1000; break; }
	case 5: {m_maxBitrate = 1200; break; }
	case 6: {m_maxBitrate = 1500; break; }
	default:
		break;
	}
	//ui.cmb_delay_min->setEnabled(true);
	//ui.cmb_delay_max->setEnabled(true);
}

//static bool bTest = true;
void PushPlayDemo::on_btn_process_clicked()
{
	//m_pusher.setPauseVideo(bTest);
	//bTest = !bTest;
	//return;

	TCHAR szBuffer[MAX_PATH] = { 0 };
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = (HWND)this->winId();;
	ofn.lpstrFilter = L"Exe文件(*.exe)\0*.exe\0所有文件(*.*)\0*.*\0";//要选择的文件后缀   
	ofn.lpstrInitialDir = L"D:\\Program Files";//默认的文件路径   
	ofn.lpstrFile = szBuffer;//存放文件的缓冲区   
	ofn.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
	ofn.nFilterIndex = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;//标志如果是多选要加上OFN_ALLOWMULTISELECT  
	if (GetSaveFileName(&ofn))
	{
		std::wstring tempPath = szBuffer;
		//int ret = m_pusher.captureVideoSnapShot((wchar_t *)tempPath.data(), tempPath.size());
		QString str0(QString::fromStdWString(tempPath.c_str()));
		//ui.lineEdit_process->setText(str0);
		return;
	}
	DWORD ret1 = CommDlgExtendedError();
	LINFO(L"CPushStreamModule::captureVideoSnapShot GetSaveFileName error");
}

void PushPlayDemo::onOpenSystemVoiceSlots(int stats)
{
	if (stats == Qt::Checked)
	{
		bool bPlayer = true;
		QString strPlayer = "";//ui.lineEdit_process->text().trimmed();
		if (strPlayer.isEmpty() || !strPlayer.contains(".exe"))
		{
			if (strPlayer.isEmpty() == false && !strPlayer.contains(".exe"))
			{
				DialogMessage::exec(QStringLiteral("请选择合法的进程!"), DialogMessage::OK);
				ui.cbOpenSystemVoice->setCheckState(Qt::Unchecked);
				return;
			}
			
			if (strPlayer.isEmpty() == false)
			{
				const wchar_t * encodedName = reinterpret_cast<const wchar_t *>(strPlayer.utf16());
				if (::PathFileExists(encodedName) == FALSE)
				{
					DialogMessage::exec(QStringLiteral("文件不存在在"), DialogMessage::OK);
					ui.cbOpenSystemVoice->setCheckState(Qt::Unchecked);
					return;
				}
			}
			bPlayer = false;
		}
		if (bPlayer)
		{
			QByteArray ba = strPlayer.toLatin1(); // must
			m_pusher.openSystemVoiceInput(ba.data());
		}
		else
			m_pusher.openSystemVoiceInput();
	}
	else
	{
		m_pusher.closeSystemVoiceInput();
	}
}

void PushPlayDemo::onBtnShotPusher()
{
	TCHAR lpszFileName[MAX_PATH + 1] = L"未命名图片";
	TCHAR lpszTitleName[MAX_PATH + 1] = L"另存为图片文件";
	TCHAR lpszFilter[MAX_PATH + 1] = L"JPEG(*.jpg; *.jpeg)\t*.jpg; *.jpeg\t\t";//L"PNG(*.png)\t*.png\tJPEG(*.jpg; *.jpeg)\t*.jpg; *.jpeg\tBMP(*.bmp)\t*.bmp\tGIF(*.gif)\t*.gif\t\t"; // L"JPEG(*.jpg; *.jpeg)\t*.jpg; *.jpeg\t\t"
	TCHAR lpszDefExt[MAX_PATH + 1] = L".jpg";
	std::wstring strDefaultName = L"截图未命名-_";
	strDefaultName = LiteAvActiveX::screenShotDefaultName();
	wcscpy_s(lpszFileName, MAX_PATH, strDefaultName.c_str());
	int nLen = wcslen(lpszFilter);
	for (int i = 0; i < nLen; ++i)
	{
		if (lpszFilter[i] == _T('\t'))
		{
			lpszFilter[i] = _T('\0');
		}
	}

	TCHAR lpszInitialPath[MAX_PATH + 1] = { 0 };
	// 默认是在桌面中
	if (!::SHGetSpecialFolderPath(NULL, lpszInitialPath, CSIDL_DESKTOP, FALSE))
	{
		lpszInitialPath[0] = _T('C');
		lpszInitialPath[1] = _T(':');
		lpszInitialPath[2] = _T('\\');
		lpszInitialPath[3] = _T('\0');
	}

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.hwndOwner = (HWND)this->winId();
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFile = lpszFileName;
	ofn.lpstrFileTitle = lpszTitleName;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_EXPLORER;
	ofn.lpstrDefExt = lpszDefExt;
	ofn.lpstrFilter = lpszFilter;
	ofn.lpstrInitialDir = lpszInitialPath;
	ofn.lpfnHook = (LPOFNHOOKPROC)&LiteAvActiveX::OFNHookProc;
	ofn.nFilterIndex = 1;
	if (GetSaveFileName(&ofn))
	{
		std::wstring tempPath = lpszFileName;
		int ret = m_pusher.captureVideoSnapShot((wchar_t *)tempPath.data(), tempPath.size());
		return;
	}
	DWORD ret1 = CommDlgExtendedError();
	LINFO(L"CPushStreamModule::captureVideoSnapShot GetSaveFileName error");
	return;
}

void PushPlayDemo::onBtnShotPlayer()
{
	TCHAR lpszFileName[MAX_PATH + 1] = L"未命名图片";
	TCHAR lpszTitleName[MAX_PATH + 1] = L"另存为图片文件";
	TCHAR lpszFilter[MAX_PATH + 1] = L"JPEG(*.jpg; *.jpeg)\t*.jpg; *.jpeg\t\t";//L"PNG(*.png)\t*.png\tJPEG(*.jpg; *.jpeg)\t*.jpg; *.jpeg\tBMP(*.bmp)\t*.bmp\tGIF(*.gif)\t*.gif\t\t"; // L"JPEG(*.jpg; *.jpeg)\t*.jpg; *.jpeg\t\t"
	TCHAR lpszDefExt[MAX_PATH + 1] = L".jpg";
	std::wstring strDefaultName = L"截图未命名-_";
	strDefaultName = LiteAvActiveX::screenShotDefaultName();
	wcscpy_s(lpszFileName, MAX_PATH, strDefaultName.c_str());
	int nLen = wcslen(lpszFilter);
	for (int i = 0; i < nLen; ++i)
	{
		if (lpszFilter[i] == _T('\t'))
		{
			lpszFilter[i] = _T('\0');
		}
	}

	TCHAR lpszInitialPath[MAX_PATH + 1] = { 0 };
	// 默认是在桌面中
	if (!::SHGetSpecialFolderPath(NULL, lpszInitialPath, CSIDL_DESKTOP, FALSE))
	{
		lpszInitialPath[0] = _T('C');
		lpszInitialPath[1] = _T(':');
		lpszInitialPath[2] = _T('\\');
		lpszInitialPath[3] = _T('\0');
	}

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.hwndOwner = (HWND)this->winId();
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrFile = lpszFileName;
	ofn.lpstrFileTitle = lpszTitleName;
	ofn.nMaxFile = MAX_PATH;
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_EXPLORER;
	ofn.lpstrDefExt = lpszDefExt;
	ofn.lpstrFilter = lpszFilter;
	ofn.lpstrInitialDir = lpszInitialPath;
	ofn.lpfnHook = (LPOFNHOOKPROC)&LiteAvActiveX::OFNHookProc;
	ofn.nFilterIndex = 1;
	if (GetSaveFileName(&ofn))
	{
		std::wstring tempPath = lpszFileName;
		int ret = m_player.captureVideoSnapShot((wchar_t *)tempPath.data(), tempPath.size());
		return;
	}
	DWORD ret1 = CommDlgExtendedError();
	LINFO(L"CPushStreamModule::captureVideoSnapShot GetSaveFileName error");
	return;
}

void PushPlayDemo::initUI()
{

	m_cameraCount = m_pusher.enumCameras();
	wchar_t **camerasName = new wchar_t *[m_cameraCount];
	for (int i = 0; i < m_cameraCount; ++i)
	{
		camerasName[i] = new wchar_t[256];
	}
	m_pusher.enumCameras(camerasName, m_cameraCount);
	ui.cmb_cameras->addItem(QString::fromStdWString(L"无"));
	for (int i = 0; i < m_cameraCount; ++i)
	{
		ui.cmb_cameras->addItem(QString::fromStdWString(camerasName[i]));
		delete[] camerasName[i];
	}
	delete[] camerasName;

	QStringList bitrateList = QStringList()
		<< QStringLiteral("自定义")
		<< QStringLiteral("FHD")
		<< QStringLiteral("HD")
		<< QStringLiteral("SD")
		<< QStringLiteral("RTC");

	ui.cmb_bitrate->addItems(bitrateList);
	ui.cmb_bitrate->setCurrentIndex(4);
	m_qurityIndex = 4;

	QStringList rotationList = QStringList()
		<< QStringLiteral("还原")
		<< QStringLiteral("顺时针90°")
		<< QStringLiteral("顺时针180°")
		<< QStringLiteral("顺时针270°");
	ui.cmb_rotation->addItems(rotationList);
	
	QStringList renderList = QStringList()
		<< QStringLiteral("适应")
		<< QStringLiteral("填充");
	ui.cmb_render->addItems(renderList);

	ui.cmb_FPS->addItem("15");
	ui.cmb_FPS->addItem("20");
	ui.cmb_FPS->addItem("25");
	ui.cmb_FPS->addItem("30");
	ui.cmb_FPS->setEnabled(false);
	ui.chb_push_mirror->setChecked(true);

	ui.lineEdit_push->setText(m_pushUrl);
	ui.lineEdit_push->setCursorPosition(0);

	if (m_cameraIndex < m_cameraCount && m_cameraIndex >=0)
	{
		ui.cmb_cameras->setCurrentIndex(m_cameraIndex);
	}
	else
	{
		DialogMessage::exec(QStringLiteral("请检查摄像头接入或输入合法的摄像头参数!"), DialogMessage::OK);
		m_cameraIndex = 0;
	}

	if (m_rotation < 4 && m_rotation >= 0)
	{
		ui.cmb_rotation->setCurrentIndex(m_rotation);
	}
	else
		m_rotation = TXE_VIDEO_ROTATION_NONE;

	/*
	if (m_bitrate>=900)
	{
		ui.cmb_bitrate->setCurrentIndex(0);
	}
	else if (m_bitrate < 900 && m_bitrate >= 600)
	{
		ui.cmb_bitrate->setCurrentIndex(1);
	}
	else if (m_bitrate < 600)
	{
		ui.cmb_bitrate->setCurrentIndex(2);
	}*/

	ui.cmb_resolution->addItem("320x240");	//0
	ui.cmb_resolution->addItem("480x360");	//1
	ui.cmb_resolution->addItem("640x480");	//2
	ui.cmb_resolution->addItem("960x720");	//3

	ui.cmb_resolution->addItem("320x180");//5
	ui.cmb_resolution->addItem("480x272");//6
	ui.cmb_resolution->addItem("640x360");//7
	ui.cmb_resolution->addItem("960x540");//8

	ui.cmb_resolution->addItem("180x320");//9
	ui.cmb_resolution->addItem("272x480");//10
	ui.cmb_resolution->addItem("360x640");//11
	ui.cmb_resolution->addItem("540x960");//12

	ui.cmb_resolution->setCurrentIndex(0);
	ui.cmb_resolution->setEnabled(false);


	ui.cmb_delay_min->addItem("100");
	ui.cmb_delay_min->addItem("200");
	ui.cmb_delay_min->addItem("300");
	ui.cmb_delay_min->addItem("400");
	ui.cmb_delay_min->addItem("500");
	ui.cmb_delay_min->addItem("600");
	ui.cmb_delay_min->setEnabled(false);

	ui.cmb_delay_max->addItem("400");
	ui.cmb_delay_max->addItem("500");
	ui.cmb_delay_max->addItem("600");
	ui.cmb_delay_max->addItem("800");
	ui.cmb_delay_max->addItem("1000");
	ui.cmb_delay_max->addItem("1200");
	ui.cmb_delay_max->addItem("1500");
	ui.cmb_delay_max->setEnabled(false);

	if (m_hHwndList != NULL)
	{
		delete[] m_hHwndList;
		m_hHwndList = NULL;
	}
	int iCntWnd = m_pusher.enumCaptureWindow();
	ui.cmb_windows->addItem(QString::fromStdWString(L"无"));
	ui.cmb_windows->addItem(QString::fromStdWString(L"录制指定区域"));
	if (iCntWnd > 0)
	{
		HWND *hwndList = new HWND[iCntWnd];
		m_pusher.enumCaptureWindow(hwndList, iCntWnd);
		m_iHwndListCnt = iCntWnd;
		m_hChooseHwnd = nullptr;
		m_hHwndList = hwndList;
		for (int i = 0; i < iCntWnd; ++i)
		{
			wchar_t szTitleName[MAX_PATH] = { 0 };
			GetWindowTextW(hwndList[i], szTitleName, MAX_PATH);
			ui.cmb_windows->addItem(QString::fromStdWString(szTitleName));
		}
	}
	ui.cmb_windows->setCurrentIndex(0);

	on_cmb_resolution_activated();

	m_beauty = false;
}

void PushPlayDemo::on_stop_pusher()
{
	ui.label_up_speed->setText("");
	ui.label_up_video_bitrate->setText("");
	ui.label_up_audio_bitrate->setText("");
	ui.label_up_video_queue->setText("");
	ui.label_up_audio_queue->setText("");
	ui.label_up_fps->setText("");
	ui.label_up_gop->setText("");
}

void PushPlayDemo::initPushParam()
{
	//m_pusher.setVideoBitRate(m_bitrate);
	m_pusher.setRotation((TXEVideoRotation)(m_rotation + 1));
	//m_pusher.switchCamera(m_cameraIndex);
	on_cmb_render_activated();
	if (m_beauty)
	{
		m_pusher.setBeautyStyle(TXE_BEAUTY_STYLE_NATURE, 5, 5);
	}
}

void PushPlayDemo::mousePressEvent(QMouseEvent *e)
{
	mousePressedPosition = e->globalPos();
	windowPositionAsDrag = pos();
}

void PushPlayDemo::mouseReleaseEvent(QMouseEvent *e)
{
	Q_UNUSED(e)
		// 鼠标放开始设置鼠标按下的位置为 null，表示鼠标没有被按下
		mousePressedPosition = QPoint();
}

void PushPlayDemo::mouseMoveEvent(QMouseEvent *e)
{
	if (!mousePressedPosition.isNull()) {
		// 鼠标按下并且移动时，移动窗口, 相对于鼠标按下时的位置计算，是为了防止误差累积
		QPoint delta = e->globalPos() - mousePressedPosition;
		move(windowPositionAsDrag + delta);
	}
}

void PushPlayDemo::on_btn_close_clicked()
{
	if (m_playing)
	{
		m_player.setCallback(NULL, NULL);
        m_player.stopPlay();
		m_player.closeRenderFrame();
	}
	if (m_pushing)
	{
		m_pusher.setCallback(NULL, NULL);
        m_pusher.stopAudioCapture();
		m_pusher.stopPreview();
		m_pusher.stopPush();
	}

	this->close();
    Application::instance().quit(0);
}