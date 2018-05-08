#include <QWidget>
#include <QPainter>

class CaptureScreen : public QWidget
{
    Q_OBJECT

public:
	CaptureScreen(QWidget *parent = 0);
	~CaptureScreen();

Q_SIGNALS:
	void signalSelectRect(QRect captureRect);

private:
	void initWindow();
	void loadBackgroundPixmap();

	virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent * event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void paintEvent(QPaintEvent *event);
	QRect getRect(const QPoint &beginPoint, const QPoint &endPoint);

private:
	bool m_isMousePress;
	QPixmap m_loadPixmap, m_capturePixmap;
	int m_screenwidth;
	int m_screenheight;
	QPoint m_beginPoint, m_endPoint;
	QPainter m_painter;
	QRect m_captureRect;
};