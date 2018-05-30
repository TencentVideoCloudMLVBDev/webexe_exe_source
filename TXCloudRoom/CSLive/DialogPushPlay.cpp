#include "DialogPushPlay.h"
#include <QMouseEvent>
#include "json.h"
#include "DialogMessage.h"
#include "log.h"
#include "Application.h"
#include "TXLiveCommon.h"
#include <QDesktopWidget> 
#include "Base.h"
#include <iostream>  
#include <fstream>  

DialogPushPlay::DialogPushPlay(bool top_window, QWidget *parent)
	: QDialog(parent)
	, m_cameraCount(0)
	, m_pushBegin(false)
	, m_bUserIsResizing(false)
    , m_pusherSnapshotPath(L"")
    , m_playerSnapshotPath(L"")
    , m_pusherURL("")
    , m_playerURL("")
{
	ui.setupUi(this);

	m_bTopWindow = top_window;
	if (top_window)
	{
		SetForegroundWindow((HWND)winId());
		setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

		QRect rect = QApplication::desktop()->screenGeometry();
		int left = (rect.width() - this->width()) / 2;
		int top = (rect.height() - this->height()) / 2;
		::SetWindowPos((HWND)this->winId(), HWND_TOPMOST, left, top, this->width(), this->height(), SWP_SHOWWINDOW);
	}
    else
    {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    }

	setWindowIcon(QIcon(":/RoomService/customservice.ico")); 

	show();
	m_player.setCallback(this, reinterpret_cast<void*>(1));
	m_pusher.setCallback(this, reinterpret_cast<void*>(2));

	qRegisterMetaType<txfunction>("txfunction");
	qApp->installEventFilter(this);
	connect(this, SIGNAL(update_event(int)), this, SLOT(on_update_event(int)));
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);
}

DialogPushPlay::~DialogPushPlay()
{
}

void DialogPushPlay::onEventCallback(int eventId, const int paramCount, const char ** paramKeys, const char ** paramValues, void * pUserData)
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
		emit update_event(1);
		break;
	case PlayEvt::PLAY_EVT_PLAY_BEGIN: ///< 开始播放
		emit update_event(4);
		break;
    case PlayEvt::PLAY_EVT_SNAPSHOT_RESULT:
        dispatch([=] {
            Application::instance().pushSnapshotImage(m_playerSnapshotPath, m_playerURL);
        });
        break;
	case PushEvt::PUSH_ERR_NET_DISCONNECT:
		emit update_event(2);
		break;
	case PushEvt::PUSH_EVT_PUSH_BEGIN:
		emit update_event(5);
		break;
	case PushEvt::PUSH_ERR_CAMERA_OCCUPY:
		emit update_event(3);
		break;
	case PushEvt::PUSH_EVT_CAMERA_CLOSED:
		emit update_event(6);
		break;
    case PushEvt::PUSH_EVT_SNAPSHOT_RESULT:
        dispatch([=] {
            Application::instance().pushSnapshotImage(m_pusherSnapshotPath, m_pusherURL);
        });
        break;
	default:
		break;
	}

    std::map<std::string, std::string> params;
    for (int i = 0; i < paramCount; ++i)
    {
        params.insert(std::pair<std::string, std::string>(paramKeys[i], paramValues[i]));
    }

	if (eventId == 1012 || eventId == 2010)
	{
		std::string temp = paramValues[0];
		if (temp.compare("0") == 0)
		{
			std::string tempPath = paramValues[1];
			std::string imageContext;
			getImageBase64(tempPath, imageContext);
			params.insert(std::pair<std::string, std::string>("base64Img", imageContext.c_str()));
		}
	}

    Application::instance().pushSDKEvent(eventId, params);
}

bool DialogPushPlay::eventFilter(QObject* pObj, QEvent* pEvent)
{
	if ((pEvent->type() == QEvent::MouseButtonRelease) || (pEvent->type() == QEvent::NonClientAreaMouseButtonRelease)) {
		QMouseEvent* pMouseEvent = dynamic_cast<QMouseEvent*>(pEvent);
		if ((pMouseEvent->button() == Qt::MouseButton::LeftButton) && m_bUserIsResizing) {
			m_bUserIsResizing = false;
			//if (m_playing)
			//	m_player.updateRenderFrame((HWND)ui.widget_video_play->winId(), RECT{ 0, 0, ui.widget_video_play->width(), ui.widget_video_play->height() });
			//if (m_pushing)
			//	m_pusher.updatePreview((HWND)ui.widget_video_push->winId(), RECT{ 0, 0, ui.widget_video_push->width(), ui.widget_video_push->height() });
		}
	}
	return QObject::eventFilter(pObj, pEvent); // pass it on without eating it
}

void DialogPushPlay::resizeEvent(QResizeEvent *event)
{
	m_bUserIsResizing = true;
	QDialog::resizeEvent(event);
}

void DialogPushPlay::showEvent(QShowEvent *event)
{
	if (m_bTopWindow)
	{
		SetForegroundWindow((HWND)winId());
		setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

		QRect rect = QApplication::desktop()->screenGeometry();
		int left = (rect.width() - this->width()) / 2;
		int top = (rect.height() - this->height()) / 2;
		::SetWindowPos((HWND)this->winId(), HWND_TOPMOST, left, top, this->width(), this->height(), SWP_SHOWWINDOW);
	}
}

void DialogPushPlay::startPush(const QString& url)
{
	stopPush();
	if (url.isEmpty() || !url.toLower().contains("rtmp://"))
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
	m_pusher.setCallback(this, reinterpret_cast<void*>(2));
	m_pusher.setRenderMode(TXE_RENDER_MODE_ADAPT);
	m_pusher.setBeautyStyle(TXE_BEAUTY_STYLE_NATURE, 5, 5);
	m_pusher.setAutoAdjustStrategy(TXE_AUTO_ADJUST_REALTIME_VIDEOCHAT_STRATEGY);
	m_pusher.startPreview((HWND)ui.widget_video_push->winId(), RECT{ 0, 0, ui.widget_video_push->width(), ui.widget_video_push->height() }, 0);
	m_pusher.startPush(url.toLocal8Bit());
	m_pusher.startAudioCapture();

	m_pushing = true;
    m_pusherURL = url.toLocal8Bit();
}

void DialogPushPlay::startPlay(const QString& url)
{
	stopPlay();
	if (url.isEmpty() || !url.toLower().contains("rtmp://"))
	{
		DialogMessage::exec(QStringLiteral("请输入合法的RTMP地址!"), DialogMessage::OK);
		return;
	}
	m_player.setCallback(this, reinterpret_cast<void*>(1));
	m_player.setRenderYMirror(false);
	m_player.setRenderMode(TXE_RENDER_MODE_ADAPT);
	m_player.setRenderFrame((HWND)ui.widget_video_play->winId(), RECT{ 0, 0, ui.widget_video_play->width(), ui.widget_video_play->height() });
	m_player.startPlay(url.toLocal8Bit(), PLAY_TYPE_LIVE_RTMP_ACC);

    m_playing = true;
    m_playerURL = url.toLocal8Bit();
}

void DialogPushPlay::stopPush()
{
    m_pusher.setCallback(nullptr, nullptr);
	m_pusher.stopAudioCapture();
	m_pusher.stopPreview();
	m_pusher.stopPush();

	ui.widget_video_push->update();
	ui.widget_video_push->setUpdatesEnabled(true);
	m_pushing = false;
	m_pushBegin = false;
}

void DialogPushPlay::stopPlay()
{
    m_player.setCallback(nullptr, nullptr);
	m_player.stopPlay();
	m_player.closeRenderFrame();
	ui.widget_video_play->update();
	ui.widget_video_play->setUpdatesEnabled(true);
	m_playing = false;
}

void DialogPushPlay::getImageBase64(std::string filePath, std::string& base64Buff)
{
	std::filebuf *pbuf;
	std::ifstream filestr;
	long size = 0;
	char * buffer = nullptr;
	base64Buff = "";
	// 要读入整个文件，必须采用二进制打开   
	filestr.open(filePath.c_str(), std::ios::binary);
	if (filestr.is_open())
	{
		// 获取filestr对应buffer对象的指针   
		pbuf = filestr.rdbuf();
		// 调用buffer对象方法获取文件大小  
		size = pbuf->pubseekoff(0, std::ios::end, std::ios::in);
		pbuf->pubseekpos(0, std::ios::in);
		// 分配内存空间  
		buffer = new char[size];
		// 获取文件内容  
		pbuf->sgetn(buffer, size);
		filestr.close();
		base64Buff = EncodeBase64((unsigned char*)buffer, size);
		delete[] buffer;
		buffer = nullptr;
	}
}

void DialogPushPlay::setTitle(const QString& title)
{
	ui.label_title->setText(title);
}

void DialogPushPlay::setLogo(const QString& logo)
{
    // todo
}

void DialogPushPlay::setProxy(const std::string& ip, unsigned short port)
{
    if (false == ip.empty())
    {
        TXLiveCommon::getInstance()->setProxy(ip.c_str(), port);
    }
}

void DialogPushPlay::snapShotPusher(const QString& path)
{
    dispatch([=] {
        m_pusherSnapshotPath = path.toStdWString();
        int ret = m_pusher.captureVideoSnapShot(m_pusherSnapshotPath.c_str(), m_pusherSnapshotPath.size());
    });
}

void DialogPushPlay::snapShotPlayer(const QString& path)
{
    dispatch([=] {
        m_playerSnapshotPath = path.toStdWString();
        int ret = m_player.captureVideoSnapShot(m_playerSnapshotPath.c_str(), m_playerSnapshotPath.size());
    });
}

void DialogPushPlay::quit()
{
    stopPlay();
    stopPush();
}

void DialogPushPlay::on_btn_min_clicked()
{
	showMinimized();
}

void DialogPushPlay::on_update_event(int status)
{
	switch (status)
	{
	case 1:
	{
		DialogMessage::exec(QStringLiteral("播放连接失败!"), DialogMessage::OK);
		stopPlay();
	}
	break;
	case 2:
	{
		DialogMessage::exec(QStringLiteral("推流连接失败!"), DialogMessage::OK);
		stopPush();
	}
	break;
	case 3:
	{
		DialogMessage::exec(QStringLiteral("摄像头已被占用!"), DialogMessage::OK);
		stopPush();
	}
	break; 
	case 4:
	{
		ui.widget_video_play->setUpdatesEnabled(false);
	}
	break;
	case 5:
	{
		m_pushBegin = true;
		ui.widget_video_push->setUpdatesEnabled(false);
	}
	break;
	case 6:
	{
		ui.widget_video_play->update();
		ui.widget_video_play->setUpdatesEnabled(true);
		ui.widget_video_push->update();
		ui.widget_video_push->setUpdatesEnabled(true);
	}
	break;
	default:
		break;
	}
}

void DialogPushPlay::handle(txfunction func)
{
	func();
}

void DialogPushPlay::mousePressEvent(QMouseEvent *e)
{
	mousePressedPosition = e->globalPos();
	windowPositionAsDrag = pos();
}

void DialogPushPlay::mouseReleaseEvent(QMouseEvent *e)
{
	Q_UNUSED(e) 
		mousePressedPosition = QPoint();
}

void DialogPushPlay::mouseMoveEvent(QMouseEvent *e)
{
	if (!mousePressedPosition.isNull()) {
		QPoint delta = e->globalPos() - mousePressedPosition;
		move(windowPositionAsDrag + delta);
	}
}

void DialogPushPlay::on_btn_close_clicked()
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

	this->hide();
    Application::instance().quit(0);
}
