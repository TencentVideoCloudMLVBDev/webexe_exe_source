#include "RTCShareVideo.h"
#include <QtWidgets/QScrollBar>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QGraphicsDropShadowEffect>
#include "RTCRoom.h"
#include <QtWidgets/QDesktopWidget>
#include "log.h"
#include "Base.h"

#define MAX_WIDTH 300
#define MIN_WIDTH 200 

RTCShareVideo::RTCShareVideo(QWidget *parent)
    : QDialog(parent)
    , m_pressed(false)
    , m_point()
{
    ui.setupUi(this);

	setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

	QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
	shadowEffect->setOffset(0, 0);
	shadowEffect->setColor(Qt::gray);
	shadowEffect->setBlurRadius(5);
	ui.widgetMain->setGraphicsEffect(shadowEffect);

	scrollArea_camera = new QScrollArea(ui.widget_camera);
	scrollArea_camera->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea_camera->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea_camera->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	scrollArea_camera->setWidgetResizable(true);

	scrollArea_camera->setContentsMargins(QMargins(0, 0, 0, 0));
	ui.hLayout_camera->addWidget(scrollArea_camera);
	hCameraLayout = new QVBoxLayout(scrollArea_camera);
	hCameraLayout->setMargin(0);
	hCameraLayout->setAlignment(Qt::AlignTop);
	hCameraLayout->setSpacing(15);
	scrollArea_camera->show();

	QWidget* widget = new QWidget(scrollArea_camera);
	widget->setLayout(hCameraLayout);

	scrollArea_camera->setWidget(widget);
	widget->show();

	const QString scrollBarStyle =
		R"(QScrollBar{
    background: transparent;
	 width: 7px;
}

QScrollBar::handle {
    background-color: #dbdbdb;
    border-radius: 3px;
}

QScrollBar::handle:hover {
    background-color: #dfdfdf;
}

QScrollBar::handle:pressed {
    background-color: #cccccc;
}

QScrollBar::add-line, QScrollBar::sub-line {
    background: transparent;
    height: 0px;
    width: 0px;
}

QScrollBar::add-page, QScrollBar::sub-page {
    background: transparent;
}

QScrollBar::up-arrow, QScrollBar::down-arrow {
    background: transparent;
    height: 0px;
    width: 0px;
})";

	scrollArea_camera->verticalScrollBar()->setStyleSheet(scrollBarStyle);
	scrollArea_camera->setParent(ui.widget_camera);

	QDesktopWidget* desktopWidget = QApplication::desktop();
	m_deskRect = desktopWidget->availableGeometry();

	setWidth(MIN_WIDTH);
}

RTCShareVideo::~RTCShareVideo()
{

}

void RTCShareVideo::setUserInfo(std::string userName, std::string userID, bool bAdd, bool bStart)
{
	LINFO(L"setuserinfo, userid: %s, add:%d, start:%d", Ansi2Wide(userID).c_str(), bAdd, bStart);

	if (bAdd)
	{
		m_videoCount++;
		RTCVideoWidget * cameraWidget;
		bool idle = false;
		for (int i = 0; i < m_vCameraWidgets.size(); i++)
		{
			if (m_vCameraWidgets[i]->idle == true)
			{
				idle = true;
				m_vCameraWidgets[i]->idle = false;
				m_vCameraWidgets[i]->userID = userID.c_str();
				cameraWidget = m_vCameraWidgets[i]->cameraWidget;
				break;
			}
		}
		if (!idle)
		{
			cameraWidget = new RTCVideoWidget(scrollArea_camera);
			RTCCameraWidget * pusherCameraWidget = new RTCCameraWidget;
			pusherCameraWidget->cameraWidget = cameraWidget;
			pusherCameraWidget->idle = false;
			pusherCameraWidget->userID = userID.c_str();
			hCameraLayout->addWidget(cameraWidget);
			m_vCameraWidgets.push_back(pusherCameraWidget);
		}

		if (m_bMax)
		{
			cameraWidget->setFixedSize(QSize(MAX_WIDTH, MAX_WIDTH + 22));
		}
		else
			cameraWidget->setFixedSize(QSize(MIN_WIDTH, MIN_WIDTH + 22));

		cameraWidget->show();
		cameraWidget->setUserName(userName);
		cameraWidget->setUserID(userID);
		if (bStart)
		{
			LINFO(L"startVideo");
			cameraWidget->startVideo(userID);
		}
	}
	else
	{
		m_videoCount--;
		for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end(); iter++)
		{
			if ((*iter)->userID == userID)
			{
				(*iter)->cameraWidget->stopVideo();
				hCameraLayout->removeWidget((*iter)->cameraWidget);
				delete (*iter)->cameraWidget;
				delete (*iter);
				m_vCameraWidgets.erase(iter);
				ui.widget_camera->update();
				break;
			}
		}
		initCameraWidget();
	}

	setWidth(m_width);
}

void RTCShareVideo::updateUI()
{
	::SetWindowPos((HWND)this->winId(), HWND_TOPMOST, m_deskRect.width() - this->width() - 20, 20, this->width(), this->height(), SWP_SHOWWINDOW);
	for (auto iter = m_vCameraWidgets.begin(); iter != m_vCameraWidgets.end(); iter++)
	{
		if (!(*iter)->idle)
		{
			(*iter)->cameraWidget->updatePreview();
		}
	}
}

void RTCShareVideo::setWidth(int width)
{
	m_width = width;
	
	int cameraHeight = m_videoCount > 0 ? (width + 22) * m_videoCount + 15 * (m_videoCount - 1) : 0;
	this->setFixedSize(QSize(width, cameraHeight + 20));
	ui.widgetMain->setFixedSize(QSize(width, cameraHeight + 20));
	ui.widget_title->setFixedSize(QSize(width, 20));
	ui.widget_camera->setFixedSize(QSize(width, cameraHeight));

	for (int i = 0; i < m_vCameraWidgets.size(); i++)
	{
		m_vCameraWidgets[i]->cameraWidget->setFixedSize(QSize(width, width + 22));
		if (m_vCameraWidgets[i]->idle == false)
		{
			m_vCameraWidgets[i]->cameraWidget->show();
		}
		else
			m_vCameraWidgets[i]->cameraWidget->hide();
	}
}

void RTCShareVideo::setCameraSize(int cameraSize)
{
	m_cameraSize = cameraSize;
	initCameraWidget();
	setWidth(m_width);
}

void RTCShareVideo::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_pressed = true;
        m_point = event->pos();
    }
}

void RTCShareVideo::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed)
    {
        move(event->pos() - m_point + pos());
    }
}

void RTCShareVideo::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    m_pressed = false;
}

void RTCShareVideo::on_btn_max_clicked()
{
	if (m_bMax)
	{
		m_bMax = false;
		
		const QString maxBtnStyle =
			R"(QPushButton{
		border: none;
		image: url(:/RoomService/max-button.png);
		background-position: center;
		}
		)";
		ui.btn_max->setStyleSheet(maxBtnStyle);

		setWidth(MIN_WIDTH);
		updateUI();
	}
	else
	{
		m_bMax = true;

		const QString restoreBtnStyle =
			R"(QPushButton{
		border: none;
		image: url(:/RoomService/restore-button.png);
		background-position: center;
		}
		)";
		ui.btn_max->setStyleSheet(restoreBtnStyle);

		hide();
		setWidth(MAX_WIDTH);
		show();
		updateUI();
	}
}

void RTCShareVideo::initCameraWidget()
{
	int createSize = m_cameraSize - m_vCameraWidgets.size();
	if (createSize <= 0)
	{
		return;
	}

	for (int i = 0; i < createSize; i++)
	{
		RTCVideoWidget * cameraWidget = new RTCVideoWidget(scrollArea_camera);
		RTCCameraWidget * pusherCameraWidget = new RTCCameraWidget;
		pusherCameraWidget->cameraWidget = cameraWidget;
		pusherCameraWidget->idle = true;
		pusherCameraWidget->userID = "";
		cameraWidget->hide();
		hCameraLayout->addWidget(cameraWidget);
		m_vCameraWidgets.push_back(pusherCameraWidget);
	}
}

void RTCShareVideo::on_btn_close_clicked()
{
	done(0);
}