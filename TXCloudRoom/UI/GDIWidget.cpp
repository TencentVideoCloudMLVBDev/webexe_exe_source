#include "GDIWidget.h"
#include <windows.h>
#include <gdiplus.h>
#include <stdio.h>
#include "libyuv.h"

//using namespace Gdiplus;

extern QWidget * mainWindow;

GDIWidget::GDIWidget(QWidget *parent)
{
	render_menu_ = new QMenu(this);
	pActEnterMain = new QAction(QStringLiteral("设为主画面"), render_menu_);
	pActStopLinkMic = new QAction(QStringLiteral("终止连麦"), render_menu_);
	pActStopCamera = new QAction(QStringLiteral("关闭摄像头"), render_menu_);
	pActStopMic = new QAction(QStringLiteral("关闭麦克风"), render_menu_);

	pActExitMain = new QAction(QStringLiteral("退出主画面"), render_menu_);
	pActOpenCamera = new QAction(QStringLiteral("打开摄像头"), render_menu_);
	pActOpenMic = new QAction(QStringLiteral("打开麦克风"), render_menu_);

	render_menu_->addAction(pActEnterMain);
	render_menu_->addAction(pActExitMain);
	render_menu_->addAction(pActStopCamera);
	render_menu_->addAction(pActOpenCamera);
	render_menu_->addAction(pActStopMic);
	render_menu_->addAction(pActOpenMic);
	render_menu_->addAction(pActStopLinkMic);

	render_menu_->setStyleSheet(".QMenu{background-color:#444444;\
		color: rgb(255,255,255);\
		font: 9pt \"微软雅黑\";}");
	connect(pActEnterMain, &QAction::triggered, this, [this] { emit doubleClicked(); m_menuInfo.fullScreen = true; });
	connect(pActStopLinkMic, &QAction::triggered, this, [this] { emit actLinkMic(); });
	connect(pActStopCamera, &QAction::triggered, this, [this] { emit actCamera(false); m_menuInfo.camera = false; });
	connect(pActStopMic, &QAction::triggered, this, [this] { emit actMic(false); m_menuInfo.mic = false; });
	connect(pActExitMain, &QAction::triggered, this, [this] { emit escPressed(); m_menuInfo.fullScreen = false; });
	connect(pActOpenCamera, &QAction::triggered, this, [this] { emit actCamera(true); m_menuInfo.camera = true; });
	connect(pActOpenMic, &QAction::triggered, this, [this] { emit actMic(true); m_menuInfo.mic = true; });
	connect(this, SIGNAL(dispatch(txfunction)), this, SLOT(handle(txfunction)), Qt::QueuedConnection);
}

void GDIWidget::enterFullScreen()
{
	if (!isFullScreen())
	{
		m_pParentWidget = parentWidget();
		m_Rect = geometry();
		if (m_pMainWidget)
		{
			this->setParent(m_pMainWidget);
			this->setWindowFlags(Qt::Window);
		}
		else
		{
			this->setParent(NULL);
		}
		this->showFullScreen();
	}
}

void GDIWidget::exitFullScreen()
{
	if (isFullScreen())
	{
		this->setParent(m_pParentWidget);
		this->setGeometry(m_Rect);
		this->setWindowFlags(Qt::SubWindow);
		this->showNormal();
	}
}

void GDIWidget::setMenuInfo(MenuInfo & menuInfo)
{
	m_menuInfo = menuInfo;
}

void GDIWidget::updateMenuInfo()
{
	QList<QAction *> actionList = render_menu_->actions();
	for (int i = 0; i < actionList.size(); i++)
	{
		actionList[i]->setVisible(true);
	}

	//if (!m_bMainDis)
	//{
		//if (m_menuInfo.fullScreen)
			render_menu_->actions()[0]->setVisible(false);
		//else
			render_menu_->actions()[1]->setVisible(false);
	//}
	//else
	//{
	//	render_menu_->actions()[0]->setVisible(false);
	//	render_menu_->actions()[1]->setVisible(false);
	//}

	if (m_menuInfo.mainDis)
	{
		//if (!m_menuInfo.camera)
			render_menu_->actions()[2]->setVisible(false);
		//else
			render_menu_->actions()[3]->setVisible(false);

		//if (!m_menuInfo.mic)
			render_menu_->actions()[4]->setVisible(false);
		//else
			render_menu_->actions()[5]->setVisible(false);
		render_menu_->actions()[6]->setVisible(false);
	}
	else
	{
		render_menu_->actions()[2]->setVisible(false);
		render_menu_->actions()[3]->setVisible(false);
		render_menu_->actions()[4]->setVisible(false);
		render_menu_->actions()[5]->setVisible(false);
		if (m_menuInfo.linkMic)
			render_menu_->actions()[6]->setVisible(true);
		else
			render_menu_->actions()[6]->setVisible(false);
	}
}

void GDIWidget::setFullScreen(bool fullScreen)
{
	m_menuInfo.fullScreen = fullScreen;
}

std::string GDIWidget::getId()
{
	return m_identifier;
}

void GDIWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
	if (isFullScreen())
	{
		m_menuInfo.fullScreen = false;
		emit escPressed();
	}
	else
	{
		m_menuInfo.fullScreen = true;
		emit doubleClicked();
		//emit applyFullScreen(this);
	}
}

void GDIWidget::mousePressEvent(QMouseEvent * event)
{
	if (event->button() == Qt::RightButton)
	{
		updateMenuInfo();
		render_menu_->exec(QCursor::pos());
	}
}

void GDIWidget::keyPressEvent(QKeyEvent * event)
{
	switch (event->key())
	{
	case Qt::Key_Escape:
		if (isFullScreen())
		{
			exitFullScreen();
		}
		else
		{
			emit escPressed();
		}
		break;
	default:
		QWidget::keyPressEvent(event);
	}
}

GDIWidget::~GDIWidget()
{
	if (argbBuf)
	{
		delete[] argbBuf;
	}
}

void GDIWidget::calAdaptPos(int & x, int & y, int & imgWidth, int & imgHeight)
{
	int screenWidth = this->width();
	int screenHeight = this->height();
	
	float imgRate = imgWidth*1.0 / imgHeight;
	float screenRate = screenWidth*1.0 / screenHeight;

	if (screenRate > imgRate)
	{
		x = (1 - imgRate / screenRate) / 2 * screenWidth + 0.5;
		y = 0;
		imgWidth = imgRate / screenRate *screenWidth + 0.5;
		imgHeight = screenHeight;
	}
	else
	{
		x = 0;
		y = (1 - screenRate / imgRate) / 2 * screenHeight + 0.5;
		imgWidth = screenWidth;
		imgHeight = screenRate / imgRate* screenHeight + 0.5;
	}
}

int GDIWidget::inputDataToARGB(unsigned char * yuvBuf, int width, int height)
{
	if (mInputWidth != width || mInputHeight != height)
	{
		if (argbBuf)
		{
			delete[] argbBuf;
		}
		argbBuf = new unsigned char[width * height * 4];
		mInputWidth = width;
		mInputHeight = height;
	}

	unsigned char* src_y = yuvBuf;
	int y_stride = width;

	unsigned char* src_u = yuvBuf + width * height;
	int u_stride = width / 2;

	unsigned char* src_v = yuvBuf + width * height * 5 / 4;
	int v_stride = width / 2;

	int rgba_stride = width * 4;

	libyuv::I420ToARGB(src_y, y_stride,
		src_u, u_stride,
		src_v, v_stride,
		argbBuf,
		rgba_stride,
		width, height);

	return 0;
}

void GDIWidget::displayFrame(const unsigned char * data, unsigned int width, unsigned int height)
{
	emit dispatch([this, data, width, height] {
		//inputDataToARGB((uint8_t*)data, width, height);

		//int x = 0, y = 0, showWidth = width, showHeight = height;
		//calAdaptPos(x, y, showWidth, showHeight);

		//Gdiplus::Graphics graphics((HWND)this->winId());
		//Gdiplus::Bitmap bitmap(width, height, width * 4, PixelFormat32bppARGB, (BYTE*)argbBuf);

		//Image *pImage = (Image *)&bitmap;

		//int screenWidth = this->width();
		//int screenHeight = this->height();

		//Gdiplus::SolidBrush black(Gdiplus::Color::Black);
		//graphics.DrawImage(pImage, x, y, showWidth, showHeight);
		//if (x != 0)
		//{
		//	graphics.FillRectangle(&black, 0, 0, x, showHeight);
		//	graphics.FillRectangle(&black, x + showWidth, 0, screenWidth - x - showWidth, showHeight);
		//}
		//else if (y != 0)
		//{
		//	graphics.FillRectangle(&black, 0, 0, showWidth, y);
		//	graphics.FillRectangle(&black, 0, y + showHeight, showWidth, screenHeight - y - showHeight);
		//}
	});
}

void GDIWidget::handle(txfunction func)
{
	func();
}
