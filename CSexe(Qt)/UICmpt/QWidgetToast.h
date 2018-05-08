#pragma once
#include <QWidget>
#include <QLabel>
#include <QPaintEvent>
#include <QTimer>

class QWidgetToast : public QWidget
{
	Q_OBJECT
public:
	explicit QWidgetToast(QWidget *parent = 0);
	~QWidgetToast();

	//默认显示3秒，无渐变后隐藏效果。以下两个接口可以更改设置，但需要在setText之前使用。
	//显示时间可设置为[1000,10000]毫秒
	void setDuration(int nMSecond);
	void setCloseOut(bool closeOut);

	void setText(const QString & text);

protected:
	void paintEvent(QPaintEvent *e);
	void showEvent(QShowEvent *e);
signals:

private slots :
	void onTimerStayOut();
	void onTimerCloseOut();//渐变后隐藏
private:
	QLabel* m_pLabel;
	QTimer * m_pTimer;
	QTimer * m_pCloseTimer;
	qreal m_dTransparent;
	int m_nParentWidth;
	int m_nParentHeight;
	int m_nMSecond;
	bool m_bCloseOut;
};