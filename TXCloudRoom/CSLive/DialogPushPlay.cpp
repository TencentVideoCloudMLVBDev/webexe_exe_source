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
	ui.widget_video_player_add1->hide();
	ui.widget_video_player_add2->hide();

	m_bTopWindow = top_window;

	SetForegroundWindow((HWND)winId());
	setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
	setWindowIcon(QIcon(":/PushPlay/live.ico"));
	show();

	m_player.setCallback(this, reinterpret_cast<void*>(1));
	m_pusher.setCallback(this, reinterpret_cast<void*>(2));

	qRegisterMetaType<txfunction>("txfunction");
	qApp->installEventFilter(this);
	connect(this, SIGNAL(update_event(int, int)), this, SLOT(on_update_event(int, int)));
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);
	
	_sessionTicketTimer = new QTimer(this);
	connect(_sessionTicketTimer, SIGNAL(timeout()), this, SLOT(TicketTimeout()));//timeoutslot()为自定义槽
	_sessionTicketTimer->start(1000);

	m_curSessionTicket++;
	QString str_recordTime = QStringLiteral("●");
	//str_recordTime.append(QDateTime::fromTime_t(m_recordTime).toUTC().toString("hh:mm:ss"));
	str_recordTime.append(QDateTime::fromTime_t(m_curSessionTicket).toUTC().toString("mm:ss"));
	ui.label_time->setText(str_recordTime);

	m_mapUrlKey.insert(std::pair<int, QString>(1, ""));
	m_mapUrlKey.insert(std::pair<int, QString>(2, ""));
	m_mapUrlKey.insert(std::pair<int, QString>(3, ""));
	m_mapUrlKey.insert(std::pair<int, QString>(4, ""));
}

DialogPushPlay::~DialogPushPlay()
{
	if (_sessionTicketTimer)
	{
		disconnect(_sessionTicketTimer, SIGNAL(timeout()), this, SLOT(TicketTimeout()));
		_sessionTicketTimer->stop();
		delete _sessionTicketTimer;
		_sessionTicketTimer = nullptr;
	}
}

void DialogPushPlay::creatsession(QString pushUrl, QString playUrl)
{
	m_curSessionTicket = 0;
	dispatch([=] {
		stopAddPusher1();
		stopAddPusher2();
		startPush(pushUrl);
		startPlay(playUrl);
	});
}

void DialogPushPlay::onEventCallback(int eventId, const int paramCount, const char ** paramKeys, const char ** paramValues, void * pUserData)
{
	const int index = reinterpret_cast<int>(pUserData);
	std::string urlKey;
	if (1 <= index && index <= 4)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_mapUrlKey.size() > 0)
			urlKey = m_mapUrlKey[index].toStdString();
	}

	if (index == 1 && eventId != TXE_STATUS_UPLOAD_EVENT)
	{
		std::stringstream eventLog;
		eventLog << "receive sdk event: target: pusher, event," << eventId;
		for (int i = 0; i < paramCount; ++i)
		{
			eventLog << " " << paramKeys[i] << "=" << paramValues[i];
		}
		LINFO(L"%s", QString::fromStdString(eventLog.str().c_str()).toStdWString().c_str());
	}
	else if ((index == 2 || index == 3 || index == 4) && eventId != TXE_STATUS_DOWNLOAD_EVENT)
	{
		std::stringstream eventLog;
		eventLog << "receive sdk event: target: player, event:" << eventId << ", url:[" << urlKey.c_str() << "] : ";
		for (int i = 0; i < paramCount; ++i)
		{
			eventLog << " " << paramKeys[i] << "=" << paramValues[i];
		}
		LINFO(L"%s", QString::fromStdString(eventLog.str().c_str()).toStdWString().c_str());
	}

	switch (eventId)
	{
	case PlayEvt::PLAY_ERR_NET_DISCONNECT: ///< 三次重连失败，断开。
		emit update_event(1, index);
		break;
	case PlayEvt::PLAY_EVT_PLAY_BEGIN: ///< 开始播放
		emit update_event(4, index);
		break;
    case PlayEvt::PLAY_EVT_SNAPSHOT_RESULT:
        dispatch([=] {
            Application::instance().pushSnapshotImage(m_playerSnapshotPath, m_playerURL);
        });
        break;
	case PushEvt::PUSH_ERR_NET_DISCONNECT:
		emit update_event(2, index);
		break;
	case PushEvt::PUSH_EVT_PUSH_BEGIN:
		emit update_event(5, index);
		break;
	case PushEvt::PUSH_ERR_CAMERA_OCCUPY:
		emit update_event(3, index);
		break;
	case PushEvt::PUSH_EVT_CAMERA_CLOSED:
		emit update_event(6, index);
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
	params.insert(std::pair<std::string, std::string>("url", urlKey));
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
	SetForegroundWindow((HWND)winId());
	setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

	QRect rect = QApplication::desktop()->screenGeometry();
	int width = this->width();
	int height = this->height();
	int left = (rect.width() - width) / 2;
	int top = (rect.height() - height) / 2;

	::SetWindowPos((HWND)this->winId(), HWND_TOPMOST, left, top, width, height, SWP_SHOWWINDOW);
	this->setAttribute(Qt::WA_Mapped);
	QWidget::showEvent(event);
}

void DialogPushPlay::startPush(QString url)
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
	m_pusher.setCallback(this, reinterpret_cast<void*>(1));
	m_pusher.setRenderMode(TXE_RENDER_MODE_ADAPT);
	m_pusher.setBeautyStyle(TXE_BEAUTY_STYLE_NATURE, 5, 5);
	m_pusher.setAutoAdjustStrategy(TXE_AUTO_ADJUST_REALTIME_VIDEOCHAT_STRATEGY);
	m_pusher.startPreview((HWND)ui.widget_video_push->winId(), RECT{ 0, 0, ui.widget_video_push->width(), ui.widget_video_push->height() }, 0);
	m_pusher.startPush(url.toLocal8Bit());
	m_pusher.startAudioCapture();

	m_pushing = true;
    m_pusherURL = url.toLocal8Bit();
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_mapUrlKey[1] = parseUrlKey(url);
	}
}

void DialogPushPlay::startPlay(QString url)
{
	stopPlay();
	showNormal();
	if (_sessionTicketTimer->isActive() == false)
		_sessionTicketTimer->start(1000);
	if (url.isEmpty() || !url.toLower().contains("rtmp://"))
	{
		DialogMessage::exec(QStringLiteral("请输入合法的RTMP地址!"), DialogMessage::OK);
		return;
	}
	m_player.setCallback(this, reinterpret_cast<void*>(2));
	m_player.setRenderYMirror(false);
	m_player.setRenderMode(TXE_RENDER_MODE_ADAPT);
	m_player.setRenderFrame((HWND)ui.widget_video_play->winId(), RECT{ 0, 0, ui.widget_video_play->width(), ui.widget_video_play->height() });
	m_player.startPlay(url.toLocal8Bit(), PLAY_TYPE_LIVE_RTMP_ACC);
	m_bMutePusher = false;
    m_playing = true;
    m_playerURL = url.toLocal8Bit();
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_mapUrlKey[2] = parseUrlKey(url);
	}
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

void DialogPushPlay::stopAddPusher1(bool changeui)
{
	if (m_addPlayerUrl1.size() > 0)
	{
		m_addPlayer1.setCallback(nullptr, nullptr);
		m_addPlayer1.stopPlay();
		m_addPlayer1.closeRenderFrame();
		if (changeui)
		{
			ui.widget_video_player_add1->update();
			ui.widget_video_player_add1->setUpdatesEnabled(true);
			//ui.widget_video_player_add1->hide(); //ui不能直接隐藏，会有异步线程操作ui，会crash
			_hidePlayerTicket1 = 0;
		}
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_mapUrlKey[3] = "";
		}
		m_addPlayerUrl1.clear();
	}
	
}

void DialogPushPlay::stopAddPusher2(bool changeui)
{
	if (m_addPlayerUrl2.size() > 0)
	{
		m_addPlayer2.setCallback(nullptr, nullptr);
		m_addPlayer2.stopPlay();
		m_addPlayer2.closeRenderFrame();
		if (changeui)
		{
			ui.widget_video_player_add2->update();
			ui.widget_video_player_add2->setUpdatesEnabled(true);
			//ui.widget_video_player_add2->hide(); //ui不能直接隐藏，会有异步线程操作ui，会crash
			_hidePlayerTicket2 = 0;
		}
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_mapUrlKey[4] = "";
		}
		m_addPlayerUrl2.clear();
	}
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

void DialogPushPlay::setMute(const bool bMute)
{
	dispatch([=] {
		m_pusher.setMute(bMute);
	});
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

int DialogPushPlay::comparePlayerUrl(QString& url, int& nPlayer, bool addOpt)
{
	nPlayer = 0;
	QStringList listNewUrl = url.split("?");
	QString newUlr = listNewUrl[0];

	if (m_addPlayerUrl1.size() > 0)
		nPlayer++;
	if (m_addPlayerUrl2.size() > 0)
		nPlayer++;

	if (m_addPlayerUrl2.size() > 0)
	{
		QString player = m_addPlayerUrl2.c_str();
		QStringList listUrl = player.split("?");
		QString oldUlr = listNewUrl[0];
		if (oldUlr == newUlr) return 2;
	}

	if (m_addPlayerUrl1.size() > 0)
	{
		QString player = m_addPlayerUrl1.c_str();
		QStringList listUrl = player.split("?");
		QString oldUlr = listNewUrl[0];
		if (oldUlr == newUlr) return 1;
	}
	
	return 0;
}

QString DialogPushPlay::parseUrlKey(QString & url)
{
	QStringList listUrl = url.split("?");
	return listUrl[0];
	/*
	if (listUrl[0].indexOf("/") != -1)
	{
		QStringList listParam = listUrl[0].split("/");
		QString headUrl = listParam[listParam.size() - 1];
		return headUrl;
	}
	else if (listUrl[0].indexOf("\\") != -1)
	{
		QStringList listParam = listUrl[0].split("\\");
		QString headUrl = listParam[listParam.size() - 1];
		return headUrl;
	}
	return "";*/
}

bool DialogPushPlay::addPlayer(QString& url)
{
	dispatch([=] {
		QString tempUrl = url;
		if (tempUrl.isEmpty() || !tempUrl.toLower().contains("rtmp://"))
		{
			DialogMessage::exec(QStringLiteral("addPusher请输入合法的RTMP地址!"), DialogMessage::OK);
			return false;
		}
		int nPlayer = 0;
		int iRet = comparePlayerUrl(tempUrl, nPlayer, true);
		if (iRet > 0)
		{
			DialogMessage::exec(QStringLiteral("addPusher 已存在url=") + url, DialogMessage::OK);
			return false;
		}
		if (nPlayer >= 2)
		{
			DialogMessage::exec(QStringLiteral("addPusher 人数已满!"), DialogMessage::OK);
			return false;
		}
		if (nPlayer == 0)
		{
			ui.widget_video_player_add1->show();
			m_addPlayer1.setCallback(this, reinterpret_cast<void*>(3));
			m_addPlayer1.setRenderYMirror(false);
			m_addPlayer1.setRenderMode(TXE_RENDER_MODE_ADAPT);
			m_addPlayer1.setRenderFrame((HWND)ui.widget_video_player_add1->winId(),
				RECT{ 0, 0, ui.widget_video_player_add1->width(), ui.widget_video_player_add1->height() });
			m_addPlayer1.startPlay(url.toLocal8Bit(), PLAY_TYPE_LIVE_RTMP_ACC);
			m_addPlayerUrl1 = url.toLocal8Bit();
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_mapUrlKey[3] = parseUrlKey(tempUrl);
			}
			_hidePlayerTicket1 = -1;
		}
		if (nPlayer == 1)
		{
			ui.widget_video_player_add2->show();
			m_addPlayer2.setCallback(this, reinterpret_cast<void*>(4));
			m_addPlayer2.setRenderYMirror(false);
			m_addPlayer2.setRenderMode(TXE_RENDER_MODE_ADAPT);
			m_addPlayer2.setRenderFrame((HWND)ui.widget_video_player_add2->winId(),
				RECT{ 0, 0, ui.widget_video_player_add2->width(), ui.widget_video_player_add2->height() });
			m_addPlayer2.startPlay(url.toLocal8Bit(), PLAY_TYPE_LIVE_RTMP_ACC);
			m_addPlayerUrl2 = url.toLocal8Bit();
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_mapUrlKey[4] = parseUrlKey(tempUrl);
			}
			_hidePlayerTicket2 = -1;
		}
	});
	return true;
}

bool DialogPushPlay::delPlayer(QString& url)
{
	dispatch([=] {
		QString tempUrl = url;
		if (tempUrl.isEmpty() || !tempUrl.toLower().contains("rtmp://"))
		{
			DialogMessage::exec(QStringLiteral("delPusher请输入合法的RTMP地址!"), DialogMessage::OK);
			return false;
		}
		int nPlayer = 0;
		int iRet = comparePlayerUrl(tempUrl, nPlayer);
		if (iRet == 1)
		{
			stopAddPusher1();
			return true;
		}
		else if (iRet == 2)
		{
			stopAddPusher2();
			return true;
		}
	});
	return false;

}

void DialogPushPlay::destroysession()
{
	dispatch([=] {
		stopPlay();
		stopPush();
		stopAddPusher1();
		stopAddPusher2();
		_sessionTicketTimer->stop();
		showMinimized();
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_mapUrlKey[1] = "";
			m_mapUrlKey[2] = "";
		}
	});
}

void DialogPushPlay::on_btn_min_clicked()
{
	showMinimized();
}

void DialogPushPlay::on_update_event(int status, int index)
{
	switch (status)
	{
	case 1:
	{

		DialogMessage::exec(QStringLiteral("播放连接失败! url = ") + m_mapUrlKey[index], DialogMessage::OK);
		stopPlay();
	}
	break;
	case 2:
	{
		DialogMessage::exec(QStringLiteral("推流连接失败! url = ") + m_mapUrlKey[index], DialogMessage::OK);
		stopPush();
	}
	break;
	case 3:
	{
		DialogMessage::exec(QStringLiteral("摄像头已被占用! url = ") + m_mapUrlKey[index], DialogMessage::OK);
		stopPush();
	}
	break; 
	case 4:
	{
		if (index == 2)
			ui.widget_video_play->setUpdatesEnabled(false);
		if (index == 3)
			ui.widget_video_player_add1->setUpdatesEnabled(false);
		if (index == 4)
			ui.widget_video_player_add2->setUpdatesEnabled(false);
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

void DialogPushPlay::on_btn_beauty_manage_clicked()
{
	if (!m_beautyManage)
	{
		m_beautyManage = new BeautyManage(this);
	}

	m_beautyManage->exec();
}

void DialogPushPlay::on_beauty_manage_ok(int beautyStyle, int beautyLevel, int whitenessLevel)
{
	m_pusher.setBeautyStyle((TXEBeautyStyle)beautyStyle, beautyLevel, whitenessLevel);
}

void DialogPushPlay::on_checkbox_mute_clicked()
{
	if (ui.checkbox_mute->checkState() == Qt::CheckState::Unchecked)
		m_bMutePusher = false;
	else
		m_bMutePusher = true;

	if (m_pushing)
	{
		m_pusher.setMute(m_bMutePusher);
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
	stopAddPusher1(false);
	stopAddPusher2(false);

	this->hide();
    Application::instance().quit(0);
}

void DialogPushPlay::TicketTimeout()
{
	if (m_pushing)
	{
		m_curSessionTicket++;
		QString str_recordTime = QStringLiteral("●");
		//str_recordTime.append(QDateTime::fromTime_t(m_recordTime).toUTC().toString("hh:mm:ss"));
		str_recordTime.append(QDateTime::fromTime_t(m_curSessionTicket).toUTC().toString("mm:ss"));
		ui.label_time->setText(str_recordTime);
		ui.label_time->update();
	}
	if (_hidePlayerTicket1 >= 0)
	{
		ui.widget_video_player_add1->update();
		if (_hidePlayerTicket1 >= 3)
		{
			ui.widget_video_player_add1->hide();
			_hidePlayerTicket1 = -1;
		}
		_hidePlayerTicket1++;
	}
	if (_hidePlayerTicket2 >= 0)
	{
		ui.widget_video_player_add2->update();
		if (_hidePlayerTicket2 >= 3)
		{
			ui.widget_video_player_add2->hide();
			_hidePlayerTicket2 = -1;
		}
		_hidePlayerTicket2++;
	}
}