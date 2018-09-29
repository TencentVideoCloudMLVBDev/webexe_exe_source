
#ifndef TXShareFrameMgr_H
#define TXShareFrameMgr_H

#include <QtCore/QRect>
#include <QtWidgets/QWidget>
#include <memory>
#include <functional>
#include "TXShareFrameChoose.h"
class TXShareFrameCallback
{
public:
	virtual ~TXShareFrameCallback() {};
	virtual void onSwitch(HWND, QRect, bool) = 0;
	virtual void onClose() = 0;
};

typedef std::function<void(void)>     txfunction;
class QTimer;
class QLabel;
class QPushButton;
class TXShareToolWnd;
class TXShareTrackWnd;
class TXShareFrameMgr : public QObject, public TXShareFrameChooseCB {
    Q_OBJECT

public:
    explicit TXShareFrameMgr(TXShareFrameCallback* callback, QObject *parent = 0);
    ~TXShareFrameMgr(void);
	void fullShareFrame();
	void areaShareFrame();
	void stopShareFrame();
	void reTrackFrame(HWND hwnd, QRect rect);
	static bool IsFollowWnd(HWND hwnd);
protected:
	virtual void chooseParam(HWND hwnd, QRect rect);
	virtual void cancelChoose();

signals:
	void dispatch(txfunction func);
protected slots:
	void TrackTimeout();
	void startTrack(HWND hwnd, QRect rect);
	void onClose();
	void onSwitch();
	void handle(txfunction func);
protected:
	void TrackWnd();
	void TrackArea();
	void stopTrack();
public:
	static QRect getPrimaryScreenRect(void);
protected:
    QTimer                      *trackTimer_ = nullptr;
	HWND					    m_hTrackHwnd = nullptr;
	QRect						m_shareRect = { 0,0,0,0 };
	QRect						m_hwndOleRect = {0,0,0,0};
	TXShareTrackWnd				*m_pTXShareTrackWnd = nullptr;  //跟随分享区域的窗口。
	TXShareFrameCallback        *m_pTXShareFrameCallback = nullptr;
	TXShareToolWnd				*m_pTXShareToolWnd = nullptr;
	TXShareFrameChoose			*m_pTXShareFrameChoose = nullptr;
};

/**
* @class : TXTrackWnd
* @brief : 窗口描边器
* @note  : 窗口分享后，需要给窗口描边。
*/
class TXShareTrackWnd : public QWidget {
	Q_OBJECT
public:

	explicit TXShareTrackWnd(HWND hWnd, QRect rect, QWidget *parent = 0);
	~TXShareTrackWnd();
	void updatePosition();
protected:

	virtual void paintEvent(QPaintEvent *);
	void setTopmostWnd(bool bTop);
private:
	QRect			currentRect_;
	QRect			m_shareRect = { 0,0,0,0 };
	HWND			m_hTrackHwnd = nullptr;
};


/**
* @class : TXTrackWnd
* @brief : 窗口描边器
* @note  : 窗口分享后，需要给窗口描边。
*/
class TXShareToolWnd : public QWidget {
	Q_OBJECT
signals :
	void onClose();
	void onSwitch();
public:
	explicit TXShareToolWnd(HWND hwnd, QWidget *parent = 0);
	~TXShareToolWnd();
protected:
	virtual void paintEvent(QPaintEvent *);
protected slots:
	void onBtnQuitClick();
	void onBtnSwitchClick();
protected:
	std::shared_ptr<QPushButton> btnStopShare;
	std::shared_ptr<QPushButton> btnSwitchArea;
	std::shared_ptr<QLabel>   tipText;
	HWND			m_hTrackHwnd = nullptr;
private:
};

#endif /// TXShareFrameMgr_H
