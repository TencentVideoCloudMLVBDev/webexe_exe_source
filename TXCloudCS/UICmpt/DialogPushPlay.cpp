#include "DialogPushPlay.h"
#include <QMouseEvent>
#include "json.h"
#include "DialogMessage.h"
#include "log.h"

DialogPushPlay::DialogPushPlay(QWidget *parent)
	: QDialog(parent)
	, m_cameraCount(0)
	, m_pushBegin(false)
	, m_handShake(10000, this)
	, m_bUserIsResizing(false)
{
	ui.setupUi(this);
	setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
	setWindowIcon(QIcon(":/PushPlay/live.ico")); 

	show();
	m_player.setCallback(this, reinterpret_cast<void*>(1));
	m_pusher.setCallback(this, reinterpret_cast<void*>(2));

	DWORD ret = m_handShake.listen();

	qRegisterMetaType<IPCConnection*>("IPCConnection*");
	qRegisterMetaType<txfunction>("txfunction");
	qApp->installEventFilter(this);
	connect(this, SIGNAL(update_event(int)), this, SLOT(on_update_event(int)));
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);
}

DialogPushPlay::~DialogPushPlay()
{
	m_handShake.close();

	for (auto iter = m_setIpc.begin(); iter != m_setIpc.end();)
	{
		delete *iter;
		m_setIpc.erase(iter++);
	}
}

void DialogPushPlay::onRecvCmd(const std::string& json)
{
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(json, root))
	{
		return;
	}

	std::string cmd;
	std::string data;
	if (root.isMember("cmd"))
	{
		cmd = root["cmd"].asString();
		std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
		if (cmd == "startpush" || cmd == "startplay")
		{
			Json::Value data;
			if (root.isMember("data"))
			{
				data = root["data"];
			}

			std::string url;
			if (data.isMember("url"))
			{
				url = data["url"].asString();
			}

			std::string id;
			if (data.isMember("id"))
			{
				id = data["id"].asString();
			}

			if (cmd == "startpush")
				on_startPush(id.c_str(), url.c_str());
			else
				on_startPlay(id.c_str(), url.c_str());
		}
		else if (cmd == "settitle")
		{
			Json::Value data;
			if (root.isMember("data"))
			{
				data = root["data"];
			}

			std::string title;
			if (data.isMember("title"))
			{
				title = data["title"].asString(); 
				on_setTitle(QString::fromUtf8(title.c_str()));
			}
		}
		else if (cmd == "stoppush")
		{
			on_stopPush();
		}
		else if (cmd == "stopplay")
		{
			on_stopPlay();
		}
		else if (cmd == "quit")
		{
			m_activeConnect = nullptr;
			on_btn_close_clicked();
		}
	}
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
	case PushEvt::PUSH_ERR_NET_DISCONNECT:
		emit update_event(2);
		break;
	case PushEvt::PUSH_EVT_PUSH_BEGIN:
		emit update_event(5);
		break;
	case PushEvt::PUSH_ERR_CAMERA_OCCUPY:
		emit update_event(3);
		break;
	default:
		break;
	}
	if (eventId == TXE_STATUS_UPLOAD_EVENT && !m_pushBegin)
	{
		return;
	}

	if (eventId == TXE_STATUS_UPLOAD_EVENT || eventId == TXE_STATUS_DOWNLOAD_EVENT)
	{
		Json::Value msg;
		if (index == 2)
		{
			msg["cmd"] = "pushStatusEvent";
		}
		else
			msg["cmd"] = "playStatusEvent";

		Json::Value data;
		if (index == 2)
		{
			data["id"] = m_pushID.toStdString();
		}
		else
			data["id"] = m_playID.toStdString();

		data["eventID"] = eventId;

		Json::Value param;
		for (int i = 0; i < paramCount; ++i)
		{
			param[paramKeys[i]] = paramValues[i];
		}
		data["params"] = param;
		msg["data"] = data;
		Json::FastWriter writer;
		std::string jsonStr = writer.write(msg);
		send(jsonStr.c_str(), jsonStr.size(), 1000);
		return;
	}
	{
		Json::Value data;
		if (index == 2)
		{
			data["id"] = m_pushID.toStdString();
		}
		else
			data["id"] = m_playID.toStdString();
		
		data["eventID"] = eventId;

		Json::Value param;
		for (int i = 0; i < paramCount; ++i)
		{
			param[paramKeys[i]] = paramValues[i];
		}
		data["params"] = param;

		Json::Value msg;
		if (index == 2)
		{
			msg["cmd"] = "pushNotifyEvent";
		}
		else
			msg["cmd"] = "playNotifyEvent";

		msg["data"] = data;

		Json::FastWriter writer;
		std::string jsonStr = writer.write(msg);
		send(jsonStr.c_str(),jsonStr.size(),1000);
	}
}

void DialogPushPlay::send(QString data, size_t dataSize, DWORD timeout)
{
	emit dispatch([this, data, dataSize, timeout] {
		if (m_activeConnect)
		{
			m_activeConnect->send(data.toStdString().c_str(), dataSize, timeout);
		}
	});
}

void DialogPushPlay::onClose(IPCConnection* connection)
{
	// todo Server崩溃了，大事情
	// 注意，不要在onClose回调中释放IPCConnection，否则引起死锁
	
	emit dispatch([this, connection] {

		if (NULL != connection)
		{
			if (connection == m_activeConnect)
			{
				m_activeConnect = nullptr;
			}
			connection->close();
		}

		for (auto iter = m_setIpc.begin(); iter != m_setIpc.end(); iter++)
		{
			if (connection == *iter)
			{
				delete *iter;
				m_setIpc.erase(iter);
				break;
			}
		}
	});
}

DWORD DialogPushPlay::onRecv(IPCConnection * connection, const void * data, size_t dataSize)
{
	QString text = reinterpret_cast<const char*>(data);
	LINFO(L"onRecv: %s\n", text.toStdWString().c_str());
	emit dispatch([this, text, dataSize, connection] {

		m_activeConnect = connection;
		onRecvCmd(text.toStdString());
	});
	return ERROR_SUCCESS;
}

void DialogPushPlay::onLog(LogLevel level, const char * content)
{
	// 建议打印log，方便事后定位问题
	LINFO(L"level: %d, content: %s\n", level, QString::fromStdString(content).toStdWString().c_str());
}

IPCConnection * DialogPushPlay::onCreateBegin()
{
	LINFO(L"onCreateBegin\n");
	return new IPCConnection(this);
}

void DialogPushPlay::onCreateEnd(bool success, IPCConnection * connection)
{
	emit dispatch([this, success, connection] {
		LINFO(L"onCreateEnd\n");

		if (true == success)
		{
			m_setIpc.insert(connection);
		}
		else
		{
			if (NULL != connection)
			{
				delete connection;
			}
		}
	});
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

void DialogPushPlay::on_startPush(QString id, QString url)
{
	on_stopPush();
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

	m_pushID = id;

	m_pusher.setRenderMode(TXE_RENDER_MODE_ADAPT);
	m_pusher.setAutoAdjustStrategy(TXE_AUTO_ADJUST_REALTIME_VIDEOCHAT_STRATEGY);
	m_pusher.startPreview((HWND)ui.widget_video_push->winId(), RECT{ 0, 0, ui.widget_video_push->width(), ui.widget_video_push->height() }, 0);
	m_pusher.startPush(url.toLocal8Bit());
	m_pusher.startAudioCapture();
	m_pushing = true;
	//ui.widget_video_push->setUpdatesEnabled(false);
}

void DialogPushPlay::on_startPlay(QString id, QString url)
{
	on_stopPlay();
	if (url.isEmpty() || !url.toLower().contains("rtmp://"))
	{
		DialogMessage::exec(QStringLiteral("请输入合法的RTMP地址!"), DialogMessage::OK);
		return;
	}
	m_playID = id;

	m_player.setRenderYMirror(false);
	m_player.setRenderMode(TXE_RENDER_MODE_ADAPT);
	m_player.setRenderFrame((HWND)ui.widget_video_play->winId(), RECT{ 0, 0, ui.widget_video_play->width(), ui.widget_video_play->height() });
	m_player.startPlay(url.toLocal8Bit(), PLAY_TYPE_LIVE_RTMP_ACC);
	m_playing = true;
	//ui.widget_video_play->setUpdatesEnabled(false);
}

void DialogPushPlay::on_stopPush()
{
	m_pusher.stopAudioCapture();
	m_pusher.stopPreview();
	m_pusher.stopPush();

	ui.widget_video_push->update();
	ui.widget_video_push->setUpdatesEnabled(true);
	m_pushing = false;
	m_pushID = "";
	m_pushBegin = false;
}

void DialogPushPlay::on_stopPlay()
{
	m_player.stopPlay();
	m_player.closeRenderFrame();
	ui.widget_video_play->update();
	ui.widget_video_play->setUpdatesEnabled(true);
	m_playing = false;
	m_playID = "";
}

void DialogPushPlay::on_setTitle(QString title)
{
	ui.label_title->setText(title);
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
		on_stopPlay();
	}
	break;
	case 2:
	{
		DialogMessage::exec(QStringLiteral("推流连接失败!"), DialogMessage::OK);
		on_stopPush();
	}
	break;
	case 3:
	{
		DialogMessage::exec(QStringLiteral("摄像头已被占用!"), DialogMessage::OK);
		on_stopPush();
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

	Json::Value msg;
	msg["cmd"] = "quit";

	Json::FastWriter writer;
	std::string jsonStr = writer.write(msg);
	send(jsonStr.c_str(), jsonStr.size(), 1000);

	this->hide();
	QApplication::exit(0);
}