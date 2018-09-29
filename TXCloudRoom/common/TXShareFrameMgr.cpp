
#include "TXShareFrameMgr.h"

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include <QtGui/QPen>
#ifndef QT_NO_DEBUG
#include <QtCore/QDebug>
#endif
#include <windows.h>
#include "log.h"

#define T_RECTWIDTH(rect) (rect.right - rect.left)
#define T_RECTHEIGHT(rect) (rect.bottom - rect.top)

TXShareFrameMgr::TXShareFrameMgr(TXShareFrameCallback* callback, QObject *parent) : QObject(parent)
{
	m_pTXShareFrameCallback = callback;
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);
	//m_pTXShareFrameChoose = new TXShareFrameChoose(nullptr);
}

TXShareFrameMgr::~TXShareFrameMgr(void)
{
	//disconnect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);
	if (m_pTXShareFrameChoose)
	{
		delete m_pTXShareFrameChoose;
		m_pTXShareFrameChoose = nullptr;
	}
	stopTrack();
}

void TXShareFrameMgr::fullShareFrame()
{
	m_hTrackHwnd = nullptr;
	m_shareRect = TXShareFrameMgr::getPrimaryScreenRect();

	if (m_pTXShareFrameCallback)
		m_pTXShareFrameCallback->onSwitch(m_hTrackHwnd, m_shareRect, false);
	//返回结果给上层处理。
	startTrack(m_hTrackHwnd, m_shareRect);

	LINFO(L"TXShareFrameMgr::fullShareFrame");
}

void TXShareFrameMgr::areaShareFrame()
{
	//stopTrack(); 如果是从新选择器，则重新选择窗口后，才stop原来的逻辑。
	//connect(TXShareFrameChoose::Instance(), SIGNAL(chooseParam(HWND, QRect)), this, SLOT(chooseParam(HWND, QRect)));
	//connect(TXShareFrameChoose::Instance(), SIGNAL(cancelChoose()), this, SLOT(cancelChoose()));
	if (m_pTXShareFrameChoose == nullptr)
		m_pTXShareFrameChoose = new TXShareFrameChoose(nullptr);
	if (m_pTXShareFrameChoose)
	{
		m_pTXShareFrameChoose->start((TXShareFrameChooseCB*)this);
	}
	LINFO(L"TXShareFrameMgr::areaShareFrame");
}

void TXShareFrameMgr::stopShareFrame()
{
	stopTrack();
}

void TXShareFrameMgr::reTrackFrame(HWND hwnd, QRect rect)
{
	startTrack(hwnd, rect);
}

bool TXShareFrameMgr::IsFollowWnd(HWND hwnd)
{
	bool bFollowWnd = false;
	if (GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED)
		bFollowWnd = true;
	//如果是本应用窗口,先默认是可以采集的：

	wchar_t str[MAX_PATH], title[MAX_PATH];
	::GetClassName(hwnd, str, MAX_PATH);
	::GetWindowText(hwnd, title, MAX_PATH);
	std::wstring class_name = str;
	std::wstring wnd_name = title;
	if (wnd_name.compare(L"RoomDemo") == 0)
		bFollowWnd = false;
	std::wstring::size_type pos = wnd_name.find(L"Chrome");
	if (pos != std::wstring::npos)
		bFollowWnd = true;
	return bFollowWnd;
}

void TXShareFrameMgr::startTrack(HWND hwnd, QRect rect)
{
	stopTrack();
	//处理描边逻辑。
	m_hTrackHwnd = hwnd;
	m_shareRect = rect;
	trackTimer_ = new QTimer(this);
	connect(trackTimer_, SIGNAL(timeout()), this, SLOT(TrackTimeout()));//timeoutslot()为自定义槽
	trackTimer_->start(50);

	if (m_pTXShareToolWnd == nullptr)
	{
		m_pTXShareToolWnd = new TXShareToolWnd(m_hTrackHwnd, nullptr);
		connect(m_pTXShareToolWnd, SIGNAL(onClose()), this, SLOT(onClose()));
		connect(m_pTXShareToolWnd, SIGNAL(onSwitch()), this, SLOT(onSwitch()));
	}
}

void TXShareFrameMgr::chooseParam(HWND hwnd, QRect rect)
{
	//disconnect(TXShareFrameChoose::Instance(), SIGNAL(chooseParam(HWND, QRect)), this, SLOT(chooseParam(HWND, QRect)));
	//disconnect(TXShareFrameChoose::Instance(), SIGNAL(cancelChoose()), this, SLOT(cancelChoose()));
	//TXShareFrameChoose::destroy();
	LINFO(L"TXShareFrameMgr::chooseParam:hwnd[%d], rect[%d,%d,%d,%d]", hwnd, rect.left(), rect.right(), rect.width(), rect.height());
	if (m_pTXShareFrameChoose)
	{
		m_pTXShareFrameChoose->stop();
	}

	//返回结果给上层处理。
	bool bFollow = IsFollowWnd(hwnd);
	if (m_pTXShareFrameCallback)
		m_pTXShareFrameCallback->onSwitch(hwnd, rect, bFollow);

	startTrack(hwnd, rect);

	emit dispatch([this] {
		if (m_pTXShareFrameChoose)
		{
			if (m_pTXShareFrameChoose)
				delete m_pTXShareFrameChoose;
			m_pTXShareFrameChoose = nullptr;
		}
	});
}

void TXShareFrameMgr::cancelChoose()
{
	//disconnect(TXShareFrameChoose::Instance(), SIGNAL(chooseParam(HWND, QRect)), this, SLOT(chooseParam(HWND, QRect)));
	//disconnect(TXShareFrameChoose::Instance(), SIGNAL(cancelChoose()), this, SLOT(cancelChoose()));
	LINFO(L"TXShareFrameMgr::cancelChoose");
	if (m_pTXShareFrameChoose) {
		m_pTXShareFrameChoose->stop();
	}

	if (m_hTrackHwnd == nullptr && m_pTXShareTrackWnd)
		m_pTXShareTrackWnd->show();

	emit dispatch([this] {
		if (m_pTXShareFrameChoose)
		{
			if (m_pTXShareFrameChoose)
				delete m_pTXShareFrameChoose;
			m_pTXShareFrameChoose = nullptr;
		}
	});
}

void TXShareFrameMgr::onClose()
{
	stopShareFrame();
	if (m_pTXShareFrameCallback)
		m_pTXShareFrameCallback->onClose();
}

void TXShareFrameMgr::onSwitch()
{
	//避免顶层描边透明窗口遮挡了其他窗口的选择。
	if (m_hTrackHwnd == nullptr && m_pTXShareTrackWnd)
		m_pTXShareTrackWnd->hide();
	areaShareFrame();
}

void TXShareFrameMgr::handle(txfunction func)
{
	func();
}

void TXShareFrameMgr::TrackWnd()
{
	BOOL bVisible = TRUE;
	if (m_hTrackHwnd && !IsWindow(m_hTrackHwnd))
	{
		bVisible = FALSE;
	}
	if (m_hTrackHwnd && !IsWindowVisible(m_hTrackHwnd))
	{
		bVisible = FALSE;
	}
	if (m_hTrackHwnd && IsIconic(m_hTrackHwnd))
	{
		bVisible = FALSE;
	}

	RECT rcWindow = { 0 };
	::GetWindowRect(m_hTrackHwnd, &rcWindow);
	if (T_RECTWIDTH(rcWindow) < 16 || T_RECTHEIGHT(rcWindow) < 16)
	{
		bVisible = FALSE;
	}
	if (!bVisible)
	{
		if (m_pTXShareTrackWnd)
			delete m_pTXShareTrackWnd;
		m_pTXShareTrackWnd = nullptr;
		return;
	}

	QRect tempRect;
	tempRect.setRect(rcWindow.left, rcWindow.top, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top);

	if (m_pTXShareTrackWnd == nullptr)
	{
		m_pTXShareTrackWnd = new TXShareTrackWnd(m_hTrackHwnd, m_shareRect, nullptr);
	}
	if (tempRect != m_hwndOleRect)
	{
		m_pTXShareTrackWnd->updatePosition();
	}
	m_hwndOleRect = tempRect;
}

void TXShareFrameMgr::TrackArea()
{
	BOOL bVisible = TRUE;
	if (m_shareRect.width() < 16 || m_shareRect.height() < 16)
		bVisible = FALSE;

	if (!bVisible)
	{
		if (m_pTXShareTrackWnd)
			delete m_pTXShareTrackWnd;
		m_pTXShareTrackWnd = nullptr;
		return;
	}
	QRect tempRect = m_shareRect;
	if (m_pTXShareTrackWnd == nullptr)
	{
		m_pTXShareTrackWnd = new TXShareTrackWnd(m_hTrackHwnd, m_shareRect, nullptr);
	}
	if (tempRect != m_hwndOleRect)
	{
		m_pTXShareTrackWnd->updatePosition();
	}
	m_hwndOleRect = tempRect;
}

QRect TXShareFrameMgr::getPrimaryScreenRect(void)
{
	/*
	QScreen *screen = QGuiApplication::primaryScreen();
#ifndef QT_NO_DEBUG
	qDebug() << screen->availableGeometry();
#endif
	QRect mm = screen->availableGeometry();
	*/
	
	int nWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int nHeight = ::GetSystemMetrics(SM_CYSCREEN);
	QRect mm;
	mm.setRect(0, 0, nWidth, nHeight);
	return mm;
}

void TXShareFrameMgr::stopTrack()
{
	if (trackTimer_)
	{
		trackTimer_->stop();
		delete trackTimer_;
		trackTimer_ = nullptr;
	}
	if (m_pTXShareToolWnd)
	{
		disconnect(m_pTXShareToolWnd, SIGNAL(onClose()), this, SLOT(onClose()));
		disconnect(m_pTXShareToolWnd, SIGNAL(onSwitch()), this, SLOT(onSwitch()));
		delete m_pTXShareToolWnd;
		m_pTXShareToolWnd = nullptr;
	}
	if (m_pTXShareTrackWnd)
	{
		delete m_pTXShareTrackWnd;
		m_pTXShareTrackWnd = nullptr;
	}
	m_shareRect = { 0,0,0,0 };
	m_hwndOleRect = { 0,0,0,0 };
	m_hTrackHwnd = nullptr;
}

void TXShareFrameMgr::TrackTimeout()
{
	if (m_hTrackHwnd)
	{
		TrackWnd();
	}
	else
	{
		TrackArea();
	}
}
///////////////////////////////////////////////////////////
TXShareTrackWnd::TXShareTrackWnd(HWND hWnd, QRect rect, QWidget *parent)
	: QWidget(parent)
{
	//showFullScreen();
	QRect desktopRect_ = QRect(0, 0, 1920, 1080);
	//setGeometry(desktopRect_);
	/// 开启鼠标实时追踪
	
	//setWindowOpacity(0.5);
	setWindowOpacity(1);
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint | Qt::Tool);
	setAttribute(Qt::WA_TranslucentBackground);
	//setWindowTitle("TXShareTrackWnd_TXRoom_exe");
	//::SetWindowText((HWND)winId(), L"TXShareTrackWnd_TXRoom_exe");
	SetWindowLongPtr((HWND)winId(), GWL_USERDATA, 9999);
	/// 默认隐藏
	m_hTrackHwnd = hWnd;
	m_shareRect = rect;
	updatePosition();
	//::EnableWindow((HWND)winId(), TRUE);
	if (hWnd != nullptr)
	{
		if (::GetWindowLong(m_hTrackHwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
		{
			setTopmostWnd(true);
		}
		//::SetWindowLong(m_hTrackHwnd, GWL_HWNDPARENT, winId());
		::SetWindowLong((HWND)winId(), GWL_HWNDPARENT, (LONG)m_hTrackHwnd);
		show();
		::SetForegroundWindow(m_hTrackHwnd);
	}
	else
	{
		setTopmostWnd(true);
		show();
	}
	
}

TXShareTrackWnd::~TXShareTrackWnd()
{
	if (m_hTrackHwnd)
		::SetWindowLong(m_hTrackHwnd, GWL_HWNDPARENT, 0);
}

void TXShareTrackWnd::updatePosition()
{
	if (m_hTrackHwnd)
	{
		RECT temp_window;
		::GetWindowRect(m_hTrackHwnd, &temp_window);
		if (::IsZoomed(m_hTrackHwnd) == 0)
		{
			currentRect_.setRect(temp_window.left - 6, temp_window.top - 6,
				temp_window.right - temp_window.left + 12,
				temp_window.bottom - temp_window.top + 12);
			this->setGeometry(currentRect_);
			/*
			if ((::GetWindowLong(m_hTrackHwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) == false)
			{
				if (::GetWindowLong((HWND)winId(), GWL_EXSTYLE) & WS_EX_TOPMOST)
				{
					setTopmostWnd(true);
					::SetForegroundWindow(m_hTrackHwnd);
				}
			}*/
		}
		else
		{
			currentRect_.setRect(temp_window.left + 6, temp_window.top + 6,
				temp_window.right - temp_window.left - 12,
				temp_window.bottom - temp_window.top - 12);
			this->setGeometry(currentRect_);

			//如果是全屏窗口，则需要置顶才能看得见窗口。
			/*
			if ((::GetWindowLong((HWND)winId(), GWL_EXSTYLE) & WS_EX_TOPMOST) == false)
			{ 
				::SetForegroundWindow((HWND)winId());
				setTopmostWnd(true);
			}*/
		}
	}
	else
	{
		currentRect_.setRect(m_shareRect.left(), m_shareRect.top(),
			m_shareRect.right() - m_shareRect.left(),
			m_shareRect.bottom() - m_shareRect.top());
		this->setGeometry(currentRect_);
	}
	update();
}

void TXShareTrackWnd::paintEvent(QPaintEvent *) {
	QPainter painter(this);

	//QPixmap pixmap = QPixmap("./temp4.png").scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
	//painter.drawPixmap(this->rect(), pixmap);
	/// 绘制边框线
	//QPen pen(QColor(0, 174, 255), 16);
	QPen pen(QColor(255, 130, 65), 16);
	painter.setPen(pen);
	painter.drawRect(rect());
	
#ifndef QT_NO_DEBUG
	qDebug() << "TXScreen::paintEvent::" << "TXScreen.rect():" << rect().left() << "," << rect().right() << "," << rect().top() << "," << rect().bottom();
#endif

	/// 绘制八个点
	//改变点的宽度
	/*
	pen.setWidth(6);
	pen.setColor(Qt::red);
	painter.setPen(pen);
	painter.drawPoints(listMarker_);
	*/
}

void TXShareTrackWnd::setTopmostWnd(bool bTop)
{
	if (bTop)
	{
		/// 窗口置顶
#ifdef Q_OS_WIN32
		SetWindowPos((HWND)this->winId(), HWND_TOPMOST, this->pos().x(), this->pos().y(), this->width(), this->height(), SWP_SHOWWINDOW);
#else
		Qt::WindowFlags flags = windowFlags();
		flags |= Qt::WindowStaysOnTopHint;
		setWindowFlags(flags);
#endif
	}
	else
	{
		/// 窗口置顶
#ifdef Q_OS_WIN32
		SetWindowPos((HWND)this->winId(), HWND_NOTOPMOST, this->pos().x(), this->pos().y(), this->width(), this->height(), SWP_SHOWWINDOW);
#else
		Qt::WindowFlags flags = windowFlags();
		flags |= Qt::WindowStaysOnTopHint;
		setWindowFlags(flags);
#endif
	}
}

///////////////////////////////////////////////////////////
TXShareToolWnd::TXShareToolWnd(HWND hwnd, QWidget *parent)
	: QWidget(parent),
	btnStopShare(new QPushButton(this)),
	btnSwitchArea(new QPushButton(this)),
	tipText(new QLabel(this))
{
	QSize toolSize = QSize(210, 110);

	//目前只支持主屏幕分享设置。
	QRect primarySRect = TXShareFrameMgr::getPrimaryScreenRect();
	int x = primarySRect.width() / 2 - toolSize.width() / 2;
	QRect toolRect = QRect(x, 2, toolSize.width(), toolSize.height());
	setGeometry(toolRect);
	//setWindowOpacity(0.8);
	//setWindowOpacity(1);
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinimizeButtonHint | Qt::Tool | Qt::WindowStaysOnTopHint);
	setAttribute(Qt::WA_TranslucentBackground);

	QString titleText;
	if (hwnd == nullptr)
	{
		titleText = QStringLiteral("正在分享屏幕");
	}
	else
	{
		titleText = QStringLiteral("正在分享窗口");
	}
	m_hTrackHwnd = hwnd;

	SetWindowLongPtr((HWND)winId(), GWL_USERDATA, 9998);
	//QFont font("Microsoft YaHei", 16, 75); //第一个属性是字体（微软雅黑），第二个是大小，第三个是加粗（权重是75） 
	//tipText->setFont(font);
	//tipText->setGeometry(10, 15, 280, 30);
	//tipText->setText(titleText);
	//tipText->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	//QPalette pe;
	//pe.setColor(QPalette::WindowText, Qt::white);
	//tipText->setPalette(pe);
	QString tipStyle =
		R"(
	border: 0px;
	color: #dedede;
	font: 9pt "Microsoft YaHei";
)";

		QString btnStyle =
		R"(
QPushButton {
border-radius: 15px;
	border: 1px solid #dedede;
	color: #dedede;
	font: 9pt "Microsoft YaHei";
}

QPushButton:hover {
	color: #cdcdcd;
	border: 1px solid #cdcdcd;
}

QPushButton:pressed {
	color: #cdcdcd;
	border: 1px solid #cdcdcd;
}
)";
	tipText->setGeometry(70, 20, 80, 20);
	tipText->setStyleSheet(tipStyle);
	tipText->setText(titleText);

	btnSwitchArea->setGeometry(20, 60, 80, 30);
	btnSwitchArea->setObjectName("rechooseShare");
	btnSwitchArea->show();
	btnSwitchArea->setText(QStringLiteral("切换应用"));
	btnSwitchArea->setStyleSheet(btnStyle);

	btnStopShare->setGeometry(110, 60, 80, 30);
	btnStopShare->setObjectName("quitShare");
	btnStopShare->show();
	btnStopShare->setText(QStringLiteral("停止分享"));
	btnStopShare->setStyleSheet(btnStyle);

	connect(btnSwitchArea.get(), SIGNAL(clicked()), this, SLOT(onBtnSwitchClick()));
	connect(btnStopShare.get(), SIGNAL(clicked()), this, SLOT(onBtnQuitClick()));
	show();
}

TXShareToolWnd::~TXShareToolWnd()
{
	disconnect(btnSwitchArea.get(), SIGNAL(clicked()), this, SLOT(onBtnSwitchClick()));
	disconnect(btnStopShare.get(), SIGNAL(clicked()), this, SLOT(onBtnQuitClick()));
}

void TXShareToolWnd::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.fillRect(this->rect(), QColor(20, 20, 20, 180));  //QColor最后一个参数80代表背景的透明度
}

void TXShareToolWnd::onBtnSwitchClick()
{
	::SetForegroundWindow(m_hTrackHwnd);
	emit onSwitch();
}

void TXShareToolWnd::onBtnQuitClick()
{
	emit onClose();
}
