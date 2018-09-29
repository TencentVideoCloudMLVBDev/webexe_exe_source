#include "TXShareFrameChoose.h"
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QApplication>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QFileDialog>
#include <QtGui/QClipboard>
#include <QtCore/QDateTime>
#include <QtGui/QPainter>
#include <QtGui/QScreen>
#include <QtGui/QCursor>
#include <QtCore/QMutex>
#include <QtWidgets/QMenu>
#include <QtGui/QPen>
#ifndef QT_NO_DEBUG
#include <QtCore/QDebug>
#endif
#include <windows.h>
#include "log.h"

bool getCurrentWindowFromCursor(QRect &out_rect, HWND& hFocusWnd)
{
	HWND hwnd = nullptr;
	POINT pt;
	// 获得当前鼠标位置
	::GetCursorPos(&pt);
	// 获得当前位置桌面上的子窗口
	hwnd = ::ChildWindowFromPointEx(::GetDesktopWindow(), pt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
	// 如果是desktop的窗口，则不能选择。
	wchar_t str[MAX_PATH], title[MAX_PATH];
	::GetClassName(hwnd, str, MAX_PATH);
	std::wstring class_name = str;

	if (class_name.compare(L"WorkerW") == 0)	return false;  //过滤掉桌面工作窗口。
	if (class_name.compare(L"Shell_TrayWnd") == 0)	return false; //滤掉任务栏本身
	if (class_name.compare(L"Progman") == 0)	return false; //滤掉任务栏本身
	if (class_name.size() == 0)	return false; //过滤掉标题为空的窗口
	//::GetWindowText(hwnd, title, MAX_PATH);
	long lRet = GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (lRet == 9999)
		return false; //滤掉任务栏本身
	if (lRet == 9998)
		return false; //滤掉任务栏本身

#ifndef QT_NO_DEBUG
	qDebug() << "class_name:" << QString::fromStdWString(str) << ";  wnd_name:" << QString::fromStdWString(title);
#endif

	hFocusWnd = hwnd;
	if (hwnd != NULL) {
		/*
		HWND temp_hwnd;
		temp_hwnd = hwnd;
		//是否支持子窗口。

		while (true) {
			::GetCursorPos(&pt);
			::ScreenToClient(temp_hwnd, &pt);
			temp_hwnd = ::ChildWindowFromPointEx(temp_hwnd,
				pt, CWP_SKIPINVISIBLE);
			if (temp_hwnd == NULL || temp_hwnd == hwnd)
				break;
			hwnd = temp_hwnd;
		}
		hFocusWnd = hwnd;
		*/
		RECT temp_window;
		::GetWindowRect(hwnd, &temp_window);
		out_rect.setRect(temp_window.left, temp_window.top,
			temp_window.right - temp_window.left,
			temp_window.bottom - temp_window.top);
		return true;
	}
	return false;
}

/// 鼠标按钮图片的十六进制数据
static const unsigned char uc_mouse_image[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,  0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52
    ,0x00, 0x00, 0x00, 0x1D, 0x00, 0x00, 0x00, 0x2D,  0x08, 0x06, 0x00, 0x00, 0x00, 0x52, 0xE9, 0x60
    ,0xA2, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59,  0x73, 0x00, 0x00, 0x0B, 0x13, 0x00, 0x00, 0x0B
    ,0x13, 0x01, 0x00, 0x9A, 0x9C, 0x18, 0x00, 0x00,  0x01, 0x40, 0x49, 0x44, 0x41, 0x54, 0x58, 0x85
    ,0xED, 0xD5, 0x21, 0x6E, 0xC3, 0x30, 0x14, 0xC6,  0xF1, 0xFF, 0x9B, 0xC6, 0x36, 0x30, 0x38, 0xA9
    ,0x05, 0x01, 0x05, 0x81, 0x05, 0x03, 0x39, 0xCA,  0x60, 0x8F, 0xD2, 0x03, 0xEC, 0x10, 0x3B, 0x46
    ,0xC1, 0xC0, 0xC6, 0x0A, 0x3B, 0x96, 0xB1, 0x80,  0x82, 0xC1, 0x56, 0x2A, 0xFF, 0x06, 0xE2, 0x36
    ,0x75, 0x9A, 0xB4, 0xCA, 0xEC, 0x4E, 0x9A, 0xE4,  0x2F, 0xB2, 0x42, 0x22, 0xFF, 0xF2, 0xFC, 0x9C
    ,0x18, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52, 0x52,  0x52, 0x52, 0x52, 0xFE, 0x55, 0xE4, 0xC6, 0xA0
    ,0xDC, 0xC4, 0x71, 0x87, 0xC1, 0xC1, 0x68, 0x01,  0xCC, 0x06, 0xC2, 0x51, 0xD0, 0x29, 0xB0, 0x18
    ,0x00, 0xDF, 0xC6, 0x40, 0x33, 0x37, 0x84, 0x30,  0x4C, 0x80, 0x85, 0xCE, 0x7B, 0x2E, 0x2A, 0x91
    ,0x84, 0x24, 0xBE, 0x25, 0xDE, 0x25, 0x5E, 0x2F,  0x6E, 0xAE, 0xD0, 0x37, 0x92, 0x10, 0xF0, 0x09
    ,0x54, 0x40, 0xE9, 0xEE, 0x15, 0xC6, 0xA2, 0x77,  0xFE, 0xE0, 0xE5, 0x85, 0x8F, 0x16, 0x58, 0xDF
    ,0x35, 0x06, 0x5B, 0xD3, 0xB9, 0xD4, 0x11, 0xD0,  0xA5, 0x8F, 0xDE, 0x57, 0x75, 0x83, 0x73, 0x50
    ,0x06, 0xF6, 0x72, 0x0A, 0x47, 0x40, 0x57, 0x0D,  0x38, 0xDE, 0xC0, 0x04, 0x6F, 0x68, 0x05, 0x36
    ,0xF5, 0xE1, 0x08, 0x3D, 0xCD, 0xEA, 0xEA, 0x5A,  0xD8, 0xBE, 0x5A, 0x46, 0xB0, 0x05, 0x1E, 0xAC
    ,0xF1, 0xC2, 0xD1, 0xCC, 0x01, 0x6D, 0x74, 0x02,  0xDB, 0x3B, 0xBF, 0xD3, 0x73, 0x07, 0x87, 0x2F
    ,0xEF, 0x53, 0x07, 0x38, 0x82, 0x2F, 0xF6, 0xFB,  0xB8, 0x81, 0x73, 0x41, 0x69, 0x28, 0x3A, 0x7A
    ,0x5C, 0xDD, 0x73, 0xCF, 0x3A, 0x86, 0xA3, 0x05,  0x87, 0xEA, 0xCC, 0x60, 0xA1, 0x06, 0x75, 0x89
    ,0xFE, 0x77, 0x92, 0x76, 0x68, 0x23, 0xEF, 0x88,  0xD3, 0x4C, 0xA8, 0x10, 0x7A, 0xD4, 0xEF, 0x8E
    ,0xBE, 0x8B, 0x68, 0x79, 0x3A, 0xB1, 0x72, 0xE1,  0xAE, 0xBC, 0x13, 0x0D, 0xDE, 0xBD, 0x3D, 0xF3
    ,0x08, 0x15, 0xD4, 0xDF, 0x4C, 0x06, 0x36, 0xF7,  0x9E, 0x09, 0xED, 0xE9, 0x99, 0x97, 0x3E, 0x42
    ,0xFF, 0x30, 0x42, 0x4B, 0xA1, 0x8D, 0xD8, 0xE9,  0x2A, 0xBD, 0xED, 0x41, 0x25, 0x2A, 0x89, 0x37
    ,0x1F, 0xBD, 0xEA, 0x61, 0x8B, 0x5F, 0xDD, 0xC1,  0xFA, 0x01, 0xD8, 0xA3, 0x8F, 0xFB, 0xCA, 0x70
    ,0x16, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,  0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

bool TXScreen::isInit_ = false;

TXShareFrameChoose::TXShareFrameChoose(QWidget *parent) : QWidget(parent),
    isLeftPressed_ (false), 
	backgroundScreen_(nullptr),
    originPainting_(nullptr), 
	screenTool_(nullptr),
	m_curChooseHwnd(nullptr),
	m_pCB(nullptr),
	m_bChooseWnd(false)
{
	connect(this, SIGNAL(clickWnd()), this, SLOT(clickWndSlot()), Qt::QueuedConnection);
}

TXShareFrameChoose::~TXShareFrameChoose(void)
{
	LINFO(L"TXShareFrameChoose::~TXShareFrameChoose()");
	disconnect(this, SIGNAL(clickWnd()), this, SLOT(clickWndSlot()));
}

void TXShareFrameChoose::start(TXShareFrameChooseCB *cb)
{
	m_pCB = cb;
	/// 初始化鼠标
	initCursor();
	/// 截取屏幕信息
	initGlobalScreen();
	/// 全屏窗口
	showFullScreen();
	/// 窗口与显示屏对齐
	setGeometry(getScreenRect());
	/// 霸道置顶
	onEgoistic();
	/// 开启鼠标实时追踪
	setMouseTracking(true);
	/// 更新鼠标的位置
	emit cursorPosChange(cursor().pos().x(), cursor().pos().y());
	/// 更新鼠标区域窗口
	updateMouse();
	/// 展示窗口
	show();
}

void TXShareFrameChoose::stop()
{
	m_pCB = nullptr;
	destroyScreen();
	close();
}

void TXShareFrameChoose::hideEvent(QHideEvent *) {
    //isActivity_ = false;
}


void TXShareFrameChoose::closeEvent(QCloseEvent *) {
    //isActivity_ = false;
}

void TXShareFrameChoose::mouseDoubleClickEvent(QMouseEvent *) {
    //emit doubleClick();
}
/**
 * 功能：获得当前屏幕的大小
 */
const QRect &TXShareFrameChoose::getScreenRect(void) {
    if (!desktopRect_.isEmpty()) {
        return desktopRect_;
    }

	int nWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int nHeight = ::GetSystemMetrics(SM_CYSCREEN);
	desktopRect_.setRect(0, 0, nWidth, nHeight);
	/*
	//先不兼容多个屏幕。分辨率太大。
	QScreen *screen = QGuiApplication::primaryScreen();
#ifndef QT_NO_DEBUG
	qDebug() << screen->availableGeometry();
#endif
	desktopRect_ = screen->availableGeometry();
	*/

    /// 兼容多个屏幕的问题
	//desktopRect_ = QRect(0, 0, 1920, 1080);
    //desktopRect_ = QRect(QApplication::desktop()->pos(), QApplication::desktop()->size());
    return desktopRect_;
}

std::shared_ptr<QPixmap> TXShareFrameChoose::initGlobalScreen(void) {
    /*if (backgroundScreen_.get() != nullptr) {
        return backgroundScreen_;
    }*/
    /// 获得屏幕原画
    std::shared_ptr<QPixmap> temp_screen = getGlobalScreen();

    /// 制作暗色屏幕背景
    QPixmap temp_dim_pix(temp_screen->width(), temp_screen->height());
    temp_dim_pix.fill((QColor(0, 0, 0, 160)));
    backgroundScreen_.reset(new QPixmap(*temp_screen));
    QPainter p(backgroundScreen_.get());
    p.drawPixmap(0, 0, temp_dim_pix);

    return backgroundScreen_;
}

/*
 * 获得屏幕的原画
 * 返回：QPixmap* 指针
 */
std::shared_ptr<QPixmap> TXShareFrameChoose::getGlobalScreen(void) {
    //if (originPainting_.get() == nullptr) {
        /// 截取当前桌面，作为截屏的背景图
        QScreen *screen = QGuiApplication::primaryScreen();
        const QRect& temp_rect = getScreenRect();
        originPainting_.reset(new QPixmap(screen->grabWindow(0, temp_rect.x(),
                            temp_rect.y(), temp_rect.width(),
                            temp_rect.height())));
   // }
    return originPainting_;
}

void TXShareFrameChoose::clickWndSlot(void)
{
	LINFO(L"TXShareFrameChoose::clickWndSlot,m_curChooseHwnd[%d]", m_curChooseHwnd);
	if (screenTool_.get() == nullptr)
	{
		if (m_pCB)
			m_pCB->cancelChoose();
		return;
	}
	QRect rect(0,0,0,0);
	rect = screenTool_->getWndQRect();
	if (m_bChooseWnd)
	{
		if (m_pCB)
		{
			m_pCB->chooseParam(m_curChooseHwnd, rect);
		}
		//emit chooseParam(m_curChooseHwnd, rect);
	}
	else
	{
		if (m_pCB)
		{
			m_pCB->chooseParam(nullptr, rect);
		}
		//emit chooseParam(nullptr, rect);
	}
	destroyScreen();
	close();
}

void TXShareFrameChoose::onEgoistic(void)
{
    /// 窗口置顶
#ifdef Q_OS_WIN32
    SetWindowPos((HWND)this->winId(),HWND_TOPMOST,this->pos().x(),this->pos().y(),this->width(),this->height(),SWP_SHOWWINDOW);
#else
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
#endif
}

/*
 * 初始化鼠标
 * 参数：_ico 鼠标图片的资源文件
 */
void TXShareFrameChoose::initCursor(const QString& ico) {
    QPixmap pixmap;
    if (ico.isEmpty()) {
        pixmap.loadFromData(uc_mouse_image, sizeof(uc_mouse_image));
    }
    else {
        pixmap.load(ico);
    }
    QCursor cursor;
    cursor = QCursor(pixmap, 15, 23);
    setCursor(cursor);
}

std::shared_ptr<TXScreen> TXShareFrameChoose::createScreen(const QPoint &pos) {
    if (screenTool_.get() == nullptr) {
        /// 创建截图器
        screenTool_.reset(new TXScreen(originPainting_, pos, this));
        /// 建立信号连接
        connect (this, SIGNAL(cursorPosChange(int,int)), screenTool_.get(),SLOT(onMouseChange(int,int)));
        /// 建立主界面双击保存信号关联
        /// 获得截图器当前起始位置
        startPoint_ = pos;
    }
    return screenTool_;
}

void TXShareFrameChoose::destroyScreen() {
    if (screenTool_.get() != nullptr) {
        /// 清理工具
        screenTool_.reset();
        screenTool_ = nullptr;
        update();
        return;
    }
}

void TXShareFrameChoose::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
		isLeftPressed_ = true;
        createScreen(e->pos());
        return ;
    }
}

void TXShareFrameChoose::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::RightButton) {
        if (screenTool_.get() != nullptr) {
            /// 更新当前鼠标选中的窗口
            updateMouse();
            return destroyScreen();
        }
		if (m_pCB)
		{
			m_pCB->cancelChoose();
		}
		LINFO(L"TXShareFrameChoose::mouseReleaseEvent,cancelChoose()");
		//emit cancelChoose();
        close();
        return ;
    }
    else if (isLeftPressed_ == true && e->button() == Qt::LeftButton) {
        /// 选择窗口选区
		isLeftPressed_ = false;

        if (startPoint_ == e->pos() && !windowRect_.isEmpty()) 
		{
            screenTool_->setGeometry(windowRect_);
            screenTool_->show();
            windowRect_ = {};
			m_bChooseWnd = true;
        }
        /// 断开鼠标移动的信号
        disconnect (this, SIGNAL(cursorPosChange(int,int)), screenTool_.get(),SLOT(onMouseChange(int,int)));
       
		//在此处获取事件和返回结果
		LINFO(L"TXShareFrameChoose::mouseReleaseEvent,emit clickWnd()");
		emit clickWnd();
    }
    QWidget::mouseReleaseEvent(e);
}

void TXShareFrameChoose::mouseMoveEvent(QMouseEvent *e) {
    emit cursorPosChange(e->x(), e->y());
    if (isLeftPressed_) {
        windowRect_ = {};
        update();
    }
    else if (isLeftPressed_ == false && false == TXScreen::state()){
        /// 霸道置顶
        onEgoistic();
        /// 更新当前鼠标选中的窗口
        updateMouse();
    }
    QWidget::mouseMoveEvent(e);
}

void TXShareFrameChoose::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    /// 画全屏图
    painter.drawPixmap(0,0,desktopRect_.width(), desktopRect_.height(), *backgroundScreen_);

    if (!windowRect_.isEmpty()) {
        /// 绘制选区
        QPen pen = painter.pen();
        pen.setColor(QColor(0,175,255));
        pen.setWidth(6);
		//pen.setStyle(Qt::DashDotLine); //Qt::PenStyle
        painter.setPen(pen);
        painter.drawPixmap(QPoint(windowRect_.x(),windowRect_.y()), *originPainting_, windowRect_);
		painter.drawRect(windowRect_.x(), windowRect_.y(), windowRect_.width(), windowRect_.height());

#ifndef QT_NO_DEBUG
		qDebug() << "TXChooseScreenArea::paintEvent::" << "windowRect_:" << windowRect_.left() << "," << windowRect_.right() << "," << windowRect_.top() << "," << windowRect_.bottom();
#endif
    }
}

void TXShareFrameChoose::updateMouse(void) {
    /// 获取当前鼠标选中的窗口
    ::EnableWindow((HWND)winId(), FALSE);
    /// @marker: 只更新一次,可以修复用户误操作导致的查找窗口与识别界面窗口不一致.
	HWND hChildHwnd = nullptr;
	windowRect_ = getScreenRect();
    bool bRet = getCurrentWindowFromCursor(windowRect_, hChildHwnd);
    QPoint temp_pt = mapFromGlobal(QPoint(windowRect_.x(), windowRect_.y()));
    windowRect_ = QRect(temp_pt.x(), temp_pt.y(), windowRect_.width(), windowRect_.height());
    ::EnableWindow((HWND)winId(), TRUE);
	m_curChooseHwnd = hChildHwnd;
    //emit findChildWind(windowRect_);
    update();
}

void TXShareFrameChoose::keyPressEvent(QKeyEvent *e) {
    /// Esc 键退出截图;
    if (e->key() == Qt::Key_Escape) {
		if (screenTool_.get() != nullptr) 
			destroyScreen();
		if (m_pCB)
		{
			m_pCB->cancelChoose();
		}
		LINFO(L"TXShareFrameChoose::keyPressEvent,cancelChoose()");
		//emit cancelChoose();
        close();
    }
    else {
        e->ignore();
    }
}

///////////////////////////////////////////////////////////

TXScreen::TXScreen(std::shared_ptr<QPixmap> originPainting, QPoint pos, QWidget *parent)
    : QWidget(parent), direction_(NONE), originPoint_(pos),
      isPressed_(false), originPainting_(originPainting) {
	menu_ = nullptr;

    /// 双击即完成
    connect(this, SIGNAL(doubleClick()), this, SLOT(onChooseWndOrArea()));
    /// 开启鼠标实时追踪
    setMouseTracking(true);
    /// 默认隐藏
    hide();
}

TXScreen::~TXScreen()
{
	isInit_ = false;
	LINFO(L"TXScreen::~TXScreen()");
}

QRect TXScreen::getWndQRect()
{
	return currentRect_;
}

void TXScreen::contextMenuEvent(QContextMenuEvent *) {
    /// 在鼠标位置弹射出菜单栏
	if (menu_)
		menu_->exec(cursor().pos());
}

void TXScreen::mouseDoubleClickEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        emit doubleClick();
        e->accept();
    }
}

void TXScreen::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        isPressed_ = true;
        if(direction_ != NONE) {
            this->mouseGrabber();
        }
		//emit doubleClick();
        /// @bug :这里可能存在问题, 不应当使用globalPos
        movePos_ = e->globalPos() - pos();
    }
}

void TXScreen::mouseReleaseEvent(QMouseEvent * e) {
    if (e->button() == Qt::LeftButton) {
        isPressed_ = false;
        if(direction_ != NONE) {
            setCursor(QCursor(Qt::SizeAllCursor));
        }

    }
}

void TXScreen::mouseMoveEvent(QMouseEvent * e) {
	
    QPoint gloPoint = mapToParent(e->pos());
    // left upper
    QPoint pt_lu = mapToParent(rect().topLeft());
    // left lower
    QPoint pt_ll = mapToParent(rect().bottomLeft());
    // right lower
    QPoint pt_rl = mapToParent(rect().bottomRight());
    // right upper
    QPoint pt_ru = mapToParent(rect().topRight());

    currentRect_ = geometry();
#ifndef QT_NO_DEBUG
	qDebug() << "onMouseChange::" << "currentRect_:" << currentRect_.left() << "," << currentRect_.right() << "," << currentRect_.top() << "," << currentRect_.bottom();
#endif
}

void TXScreen::moveEvent(QMoveEvent *) {
    emit postionChange(x(), y());
}

void TXScreen::resizeEvent(QResizeEvent *) {
    listMarker_.clear();

    /// 重新计算八个锚点
    // 角点
    listMarker_.push_back(QPoint(0, 0));
    listMarker_.push_back(QPoint(width(), 0));
    listMarker_.push_back(QPoint(0, height()));
    listMarker_.push_back(QPoint(width(), height()));

    // 中点
    listMarker_.push_back(QPoint((width() >> 1), 0));
    listMarker_.push_back(QPoint((width() >> 1), height()));
    listMarker_.push_back(QPoint(0, (height() >> 1)));
    listMarker_.push_back(QPoint(width(), (height() >> 1)));

    emit sizeChange(width(), height());
}

void TXScreen::showEvent(QShowEvent *) {
    isInit_ = true;
}

void TXScreen::hideEvent(QHideEvent *) {
    currentRect_ = {};
    movePos_ = {};
    originPoint_ = {};
    isInit_ = false;
}

void TXScreen::enterEvent(QEvent *e) {
    //setCursor(Qt::SizeAllCursor);
    QWidget::enterEvent(e);
}

void TXScreen::leaveEvent(QEvent *e) {
    //setCursor(Qt::ArrowCursor);
    QWidget::leaveEvent(e);
}

void TXScreen::closeEvent(QCloseEvent *)
{
    isInit_ = false;
}

void TXScreen::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    /// 绘制截屏编辑窗口
    painter.drawPixmap(QPoint(0,0), *originPainting_, currentRect_);

    /// 绘制边框线
    QPen pen(QColor(0,174,255), 6);
    painter.setPen(pen);
    painter.drawRect(rect());

}

void TXScreen::onChooseWndOrArea(void) {
    /// 把图片放入剪切板
    //QClipboard *board = QApplication::clipboard();
    //board->setPixmap(originPainting_->copy(currentRect_));
    /// 退出当前截图工具
	QRect rect(currentRect_);
	quitScreenshot();
#ifndef QT_NO_DEBUG
	qDebug() << "onChooseWndOrArea::" << "currentRect_:" << rect.left() << "," << rect.right() << "," << rect.top() << "," << rect.bottom();
#endif
}

void TXScreen::quitScreenshot(void) {
    close();
    parentWidget()->close();
}

void TXScreen::onMouseChange(int x, int y) {
    show();
    if (x < 0 || y < 0) {
        return;
    }
    const int& rx = (x >= originPoint_.x()) ? originPoint_.x() : x;
    const int& ry = (y >= originPoint_.y()) ? originPoint_.y() : y;
    const int& rw = abs(x - originPoint_.x());
    const int& rh = abs(y - originPoint_.y());

    /// 改变大小
    currentRect_ = QRect(rx, ry, rw, rh);
#ifndef QT_NO_DEBUG
	qDebug() << "onMouseChange::" << "currentRect_:" << currentRect_.left() << "," << currentRect_.right() << "," << currentRect_.top() << "," << currentRect_.bottom();
#endif
    this->setGeometry(currentRect_);
    /// 改变大小后更新父窗口，防止父窗口未及时刷新而导致的问题
    parentWidget()->update();
}

