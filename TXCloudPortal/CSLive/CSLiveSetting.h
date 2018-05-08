#pragma once

#include <QDialog>
#include "ui_CSLiveSetting.h"

class CSLiveSetting : public QDialog
{
	Q_OBJECT

public:
	CSLiveSetting(QWidget *parent = Q_NULLPTR);
	~CSLiveSetting();

protected:
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);

protected slots:
	void on_btn_close_clicked();
	void on_btn_ok_clicked();
	void on_btn_cancel_clicked();

private:
	Ui::CSLiveSetting ui;
	bool m_pressed;
	QPoint m_point;
};
