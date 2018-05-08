#ifndef GDIWIDGET_H
#define GDIWIDGET_H

#include <QMutex>
#include <QString>
#include "commonType.h"
#include <qevent.h>
#include <qmenu.h>
#include <memory>

typedef std::function<void(void)> txfunction;

class GDIWidget : public QWidget
{
	Q_OBJECT
public:
	explicit GDIWidget(QWidget *parent = nullptr);
	~GDIWidget();

public:
	void    displayFrame(const unsigned char * data, unsigned int width, unsigned int height);

	void	enterFullScreen();//È«ÆÁ
	void	exitFullScreen();//ÍË³öÈ«ÆÁ

	void    setMenuInfo(MenuInfo & menuInfo);
	void    updateMenuInfo();
	void    setFullScreen(bool fullScreen);

	std::string getId();
	void setId(const std::string &id)
	{
		m_identifier = id;
	}

signals:
	void    doubleClicked();
	void    escPressed();
	void    actLinkMic();
	void    actCamera(bool open);
	void    actMic(bool open);

	void    dispatch(txfunction func);

private slots:
	void handle(txfunction func);

protected:
	void	mouseDoubleClickEvent(QMouseEvent * event) override;
	void    mousePressEvent(QMouseEvent * event) override;
	void	keyPressEvent(QKeyEvent *event) override;

private:
	std::string	m_identifier;

	QWidget*	m_pParentWidget; 
	QWidget*	m_pMainWidget;
	QRect		m_Rect;

	QMenu *  render_menu_;
	QAction* pActEnterMain;
	QAction* pActStopLinkMic;
	QAction* pActStopCamera;
	QAction* pActStopMic;

	QAction* pActExitMain;
	QAction* pActOpenCamera;
	QAction* pActOpenMic;

	MenuInfo m_menuInfo;

	unsigned char* argbBuf = NULL;
	int mInputWidth = 0;
	int mInputHeight = 0;
	void calAdaptPos(int & x, int & y, int & imgWidth, int & imgHeight);
	int inputDataToARGB(unsigned char* yuvBuf, int width, int height);
};

#endif // RENDERWIDGET_H